// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Wraith Launcher contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "SnapshotStore.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

#include "archive/ArchiveWriter.h"

namespace Wraith::Snapshots {

namespace {
// Folders inside the game folder that are never part of a snapshot: logs, generated caches and big outputs.
const QStringList s_excludedGameFolders = { "logs", "crash-reports", "screenshots", ".cache", ".fabric", ".quilt", ".mixin.out",
                                            "webcache", "backups", "debug" };
// Folders inside the instance folder that are never part of a snapshot.
const QStringList s_excludedRootFolders = { ".wraith", "wraith-server", "natives" };

QString relativeSlash(const QDir& root, const QString& path)
{
    return QDir::fromNativeSeparators(root.relativeFilePath(path));
}

QJsonObject toJson(const Snapshot& s)
{
    QJsonObject files;
    for (auto it = s.files.begin(); it != s.files.end(); ++it)
        files.insert(it.key(), QJsonObject{ { "sha1", it->hash }, { "size", it->size } });
    return { { "formatVersion", 1 },
             { "id", s.id },
             { "created", s.created.toString(Qt::ISODateWithMs) },
             { "label", s.label },
             { "trigger", s.trigger },
             { "automatic", s.automatic },
             { "pinned", s.pinned },
             { "includesWorlds", s.includesWorlds },
             { "skipped", QJsonArray::fromStringList(s.skipped) },
             { "files", files } };
}

Snapshot fromJson(const QJsonObject& obj)
{
    Snapshot s;
    s.id = obj.value("id").toString();
    s.created = QDateTime::fromString(obj.value("created").toString(), Qt::ISODateWithMs);
    s.label = obj.value("label").toString();
    s.trigger = obj.value("trigger").toString();
    s.automatic = obj.value("automatic").toBool();
    s.pinned = obj.value("pinned").toBool();
    s.includesWorlds = obj.value("includesWorlds").toBool();
    for (const auto& v : obj.value("skipped").toArray())
        s.skipped.append(v.toString());
    const auto files = obj.value("files").toObject();
    for (auto it = files.begin(); it != files.end(); ++it) {
        const auto f = it.value().toObject();
        s.files.insert(it.key(), FileEntry{ f.value("sha1").toString(), f.value("size").toInteger() });
    }
    return s;
}
}  // namespace

qint64 Snapshot::totalSize() const
{
    qint64 total = 0;
    for (const auto& f : files)
        total += f.size;
    return total;
}

Store::Store(QString instanceRoot, QString gameRoot) : m_root(QDir(instanceRoot).absolutePath())
{
    m_gameDir = QDir::fromNativeSeparators(QDir(m_root).relativeFilePath(QDir(gameRoot).absolutePath()));
    if (m_gameDir == ".")
        m_gameDir.clear();
}

QString Store::storeDir() const
{
    return QDir(m_root).filePath(".wraith/snapshots");
}

QString Store::objectPath(const QString& hash) const
{
    return QDir(storeDir()).filePath(QString("objects/%1/%2").arg(hash.left(2), hash));
}

QString Store::snapshotPath(const QString& id) const
{
    return QDir(storeDir()).filePath(id + ".json");
}

bool Store::inScope(const QString& relativePath, bool includeWorlds) const
{
    const QString first = relativePath.section('/', 0, 0);
    if (s_excludedRootFolders.contains(first))
        return false;
    const QString prefix = m_gameDir.isEmpty() ? QString() : m_gameDir + "/";
    if (!prefix.isEmpty() && !relativePath.startsWith(prefix))
        return true;  // instance files outside the game folder: instance.cfg, mmc-pack.json, patches/, jarmods/ ...
    const QString inGame = relativePath.mid(prefix.size());
    const QString top = inGame.section('/', 0, 0);
    if (inGame.contains('/') && s_excludedGameFolders.contains(top))
        return false;
    if (top == "saves" && inGame.contains('/'))
        return includeWorlds;
    return true;
}

void Store::walk(const QString& dir, bool includeWorlds, QStringList& out) const
{
    const QDir root(m_root);
    const auto entries = QDir(dir).entryInfoList(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const auto& entry : entries) {
        const QString relative = relativeSlash(root, entry.absoluteFilePath());
        if (entry.isDir()) {
            // shared folders are links to a folder other instances use too; they are not this instance's to restore
            if (entry.isJunction() || entry.isSymLink())
                continue;
            // test with a file name inside the folder, so excluded folders are skipped without walking them
            if (inScope(relative + "/x", includeWorlds))
                walk(entry.absoluteFilePath(), includeWorlds, out);
        } else if (inScope(relative, includeWorlds)) {
            out.append(relative);
        }
    }
}

void Store::loadHashCache()
{
    if (m_hashCacheLoaded)
        return;
    m_hashCacheLoaded = true;
    QFile file(QDir(storeDir()).filePath("hashcache.json"));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto obj = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        const auto e = it.value().toObject();
        m_hashCache.insert(it.key(), { e.value("size").toInteger(), e.value("mtime").toInteger(), e.value("sha1").toString() });
    }
}

void Store::saveHashCache() const
{
    QJsonObject obj;
    for (auto it = m_hashCache.begin(); it != m_hashCache.end(); ++it)
        obj.insert(it.key(), QJsonObject{ { "size", it->size }, { "mtime", it->mtime }, { "sha1", it->hash } });
    QDir().mkpath(storeDir());
    QSaveFile file(QDir(storeDir()).filePath("hashcache.json"));
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

QString Store::hashFile(const QString& relative, const QFileInfo& info)
{
    const qint64 mtime = info.lastModified().toMSecsSinceEpoch();
    auto cached = m_hashCache.constFind(relative);
    if (cached != m_hashCache.constEnd() && cached->size == info.size() && cached->mtime == mtime)
        return cached->hash;
    QFile file(info.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    const QString hex = QString::fromLatin1(hash.result().toHex());
    m_hashCache.insert(relative, { info.size(), mtime, hex });
    return hex;
}

FileMap Store::scanCurrent(bool includeWorlds, const Progress& progress, QStringList* skipped)
{
    loadHashCache();
    QStringList paths;
    walk(m_root, includeWorlds, paths);
    FileMap files;
    const QDir root(m_root);
    for (int i = 0; i < paths.size(); i++) {
        const QFileInfo info(root.filePath(paths[i]));
        if (info.size() > s_maxFileSize) {
            if (skipped)
                skipped->append(paths[i]);
        } else if (const QString hash = hashFile(paths[i], info); !hash.isEmpty()) {
            files.insert(paths[i], { hash, info.size() });
        }
        if (progress)
            progress(i + 1, static_cast<int>(paths.size()) * 2);
    }
    saveHashCache();
    return files;
}

Result<Snapshot> Store::create(const Options& options, const Progress& progress)
{
    Snapshot snapshot;
    snapshot.created = QDateTime::currentDateTime();
    snapshot.id = snapshot.created.toUTC().toString("yyyyMMdd-HHmmss-zzz");
    while (QFile::exists(snapshotPath(snapshot.id)))
        snapshot.id += "x";
    snapshot.label = options.label;
    snapshot.trigger = options.trigger.isEmpty() ? QString("Manual") : options.trigger;
    snapshot.automatic = options.automatic;
    snapshot.includesWorlds = options.includeWorlds;
    snapshot.files = scanCurrent(options.includeWorlds, progress, &snapshot.skipped);

    // store contents not stored yet (written to a temporary name first, so a crash never leaves a half object)
    const QDir root(m_root);
    int done = 0;
    const int total = static_cast<int>(snapshot.files.size());
    for (auto it = snapshot.files.begin(); it != snapshot.files.end(); ++it) {
        const QString object = objectPath(it->hash);
        if (!QFile::exists(object)) {
            QDir().mkpath(QFileInfo(object).absolutePath());
            const QString temp = object + ".part";
            QFile::remove(temp);
            if (!QFile::copy(root.filePath(it.key()), temp) || !QFile::rename(temp, object)) {
                QFile::remove(temp);
                return std::unexpected(QString("couldn't store %1").arg(it.key()));
            }
        }
        if (progress)
            progress(total + ++done, total * 2);
    }
    if (!save(snapshot))
        return std::unexpected(QString("couldn't write %1").arg(snapshotPath(snapshot.id)));
    return snapshot;
}

bool Store::save(const Snapshot& snapshot) const
{
    QDir().mkpath(storeDir());
    QSaveFile file(snapshotPath(snapshot.id));
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(toJson(snapshot)).toJson());
    return file.commit();
}

QList<Snapshot> Store::list() const
{
    QList<Snapshot> snapshots;
    const auto files = QDir(storeDir()).entryInfoList({ "*.json" }, QDir::Files);
    for (const auto& info : files) {
        if (info.fileName() == "hashcache.json")
            continue;
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly))
            continue;
        auto snapshot = fromJson(QJsonDocument::fromJson(file.readAll()).object());
        if (!snapshot.id.isEmpty())
            snapshots.append(snapshot);
    }
    std::sort(snapshots.begin(), snapshots.end(), [](const Snapshot& a, const Snapshot& b) { return a.id > b.id; });
    return snapshots;
}

std::optional<Snapshot> Store::get(const QString& id) const
{
    QFile file(snapshotPath(id));
    if (!file.open(QIODevice::ReadOnly))
        return std::nullopt;
    auto snapshot = fromJson(QJsonDocument::fromJson(file.readAll()).object());
    if (snapshot.id.isEmpty())
        return std::nullopt;
    return snapshot;
}

bool Store::setPinned(const QString& id, bool pinned)
{
    auto snapshot = get(id);
    if (!snapshot)
        return false;
    snapshot->pinned = pinned;
    return save(*snapshot);
}

bool Store::setLabel(const QString& id, const QString& label)
{
    auto snapshot = get(id);
    if (!snapshot)
        return false;
    snapshot->label = label.trimmed();
    return save(*snapshot);
}

bool Store::remove(const QString& id)
{
    if (!QFile::remove(snapshotPath(id)))
        return false;
    collectGarbage();
    return true;
}

QStringList Store::prune(int ringSize)
{
    QStringList removed;
    if (ringSize <= 0)
        return removed;
    int kept = 0;
    for (const auto& snapshot : list()) {  // newest first
        if (snapshot.pinned)
            continue;
        if (++kept > ringSize && QFile::remove(snapshotPath(snapshot.id)))
            removed.append(snapshot.id);
    }
    if (!removed.isEmpty())
        collectGarbage();
    return removed;
}

void Store::collectGarbage()
{
    QSet<QString> used;
    for (const auto& snapshot : list()) {
        for (const auto& f : snapshot.files)
            used.insert(f.hash);
    }
    const QDir objects(QDir(storeDir()).filePath("objects"));
    for (const auto& bucket : objects.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        for (const auto& object : QDir(bucket.absoluteFilePath()).entryInfoList(QDir::Files)) {
            if (!used.contains(object.fileName()))
                QFile::remove(object.absoluteFilePath());
        }
        QDir().rmdir(bucket.absoluteFilePath());  // only succeeds when empty
    }
}

Diff Store::diff(const FileMap& from, const FileMap& to)
{
    Diff d;
    for (auto it = to.begin(); it != to.end(); ++it) {
        auto old = from.constFind(it.key());
        if (old == from.constEnd())
            d.added.append(it.key());
        else if (!(*old == *it))
            d.changed.append(it.key());
    }
    for (auto it = from.begin(); it != from.end(); ++it) {
        if (!to.contains(it.key()))
            d.removed.append(it.key());
    }
    return d;
}

Diff Store::restorePreview(const Snapshot& snapshot)
{
    Diff d = diff(scanCurrent(snapshot.includesWorlds), snapshot.files);
    // instance.cfg holds the playtime counters and settings the launcher keeps in memory; it is kept in snapshots
    // (for comparing and exporting) but never rolled back, so restoring can't erase playtime or fight the launcher.
    for (auto* list : { &d.added, &d.removed, &d.changed })
        list->removeAll("instance.cfg");
    return d;
}

Result<> Store::restore(const Snapshot& snapshot, const Progress& progress)
{
    // check every stored file first, so a damaged store never leaves the instance half restored
    for (const auto& f : snapshot.files) {
        if (!QFile::exists(objectPath(f.hash)))
            return std::unexpected(QString("the snapshot is incomplete (missing stored file %1)").arg(f.hash));
    }
    const Diff d = restorePreview(snapshot);
    const QDir root(m_root);
    int done = 0;
    const int total = d.count();
    for (const auto& path : d.removed) {
        QFile::remove(root.filePath(path));
        // tidy up folders that became empty (QDir::rmdir refuses non-empty ones)
        QString parent = QFileInfo(root.filePath(path)).absolutePath();
        while (parent.startsWith(m_root + "/") && QDir().rmdir(parent))
            parent = QFileInfo(parent).absolutePath();
        if (progress)
            progress(++done, total);
    }
    for (const auto& path : d.added + d.changed) {
        const QString target = root.filePath(path);
        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile::remove(target);
        if (!QFile::copy(objectPath(snapshot.files.value(path).hash), target))
            return std::unexpected(QString("couldn't restore %1").arg(path));
        QFile(target).setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup |
                                     QFile::ReadOther);
        if (progress)
            progress(++done, total);
    }
    return {};
}

Result<> Store::exportToFolder(const Snapshot& snapshot, const QString& folder) const
{
    const QDir target(folder);
    for (auto it = snapshot.files.begin(); it != snapshot.files.end(); ++it) {
        const QString path = target.filePath(it.key());
        QDir().mkpath(QFileInfo(path).absolutePath());
        if (QFile::exists(path))
            return std::unexpected(QString("%1 already exists").arg(path));
        if (!QFile::copy(objectPath(it->hash), path))
            return std::unexpected(QString("couldn't write %1").arg(path));
    }
    return {};
}

Result<> Store::exportToZip(const Snapshot& snapshot, const QString& zipPath) const
{
    MMCZip::ArchiveWriter zip(zipPath);
    if (auto opened = zip.open(); !opened)
        return opened;
    for (auto it = snapshot.files.begin(); it != snapshot.files.end(); ++it) {
        if (auto added = zip.addFile(objectPath(it->hash), it.key()); !added)
            return added;
    }
    return zip.close();
}

QByteArray Store::contents(const QString& hash) const
{
    QFile file(objectPath(hash));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

qint64 Store::storageBytes() const
{
    qint64 total = 0;
    QDirIterator it(QDir(storeDir()).filePath("objects"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        total += QFileInfo(it.next()).size();
    return total;
}

}  // namespace Wraith::Snapshots
