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

#include "ServerFileSync.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSet>

#include <algorithm>

namespace WraithServer {

FileSync::FileSync(QString targetRoot, QString manifestPath) : m_root(std::move(targetRoot)), m_manifestPath(std::move(manifestPath))
{
    load();
}

QString FileSync::fileHash(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return QString::fromLatin1(hash.result().toHex());
}

void FileSync::load()
{
    QFile file(m_manifestPath);
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto obj = QJsonDocument::fromJson(file.readAll()).object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        m_manifest.insert(it.key(), it.value().toString());
}

void FileSync::save() const
{
    QJsonObject obj;
    for (auto it = m_manifest.begin(); it != m_manifest.end(); ++it)
        obj.insert(it.key(), it.value());
    QSaveFile file(m_manifestPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(obj).toJson());
        file.commit();
    }
}

FileSync::Result FileSync::sync(const QList<Item>& items, const QStringList& prefixes)
{
    Result result;
    const QDir root(m_root);
    QSet<QString> wanted;

    for (const auto& item : items) {
        wanted.insert(item.relative);
        const QString target = root.filePath(item.relative);
        const QString sourceHash = fileHash(item.source);
        if (QFile::exists(target)) {
            const QString targetHash = fileHash(target);
            if (targetHash == sourceHash) {
                m_manifest.insert(item.relative, sourceHash);
                result.unchanged++;
                continue;
            }
            // Only replace a file Wraith wrote and nobody changed since.
            if (m_manifest.value(item.relative) != targetHash) {
                result.keptEdited.append(item.relative);
                continue;
            }
            QFile::remove(target);
        }
        QDir().mkpath(QFileInfo(target).absolutePath());
        if (QFile::copy(item.source, target)) {
            m_manifest.insert(item.relative, sourceHash);
            result.copied++;
        } else {
            result.failed.append(item.relative);
        }
    }

    // Files Wraith copied before that are no longer wanted: remove them only if they are untouched.
    for (const auto& relative : m_manifest.keys()) {
        if (wanted.contains(relative))
            continue;
        const bool managed = std::any_of(prefixes.begin(), prefixes.end(), [&relative](const QString& p) { return relative.startsWith(p); });
        if (!managed)
            continue;
        const QString target = root.filePath(relative);
        if (!QFile::exists(target)) {
            m_manifest.remove(relative);
        } else if (fileHash(target) == m_manifest.value(relative)) {
            if (QFile::remove(target))
                result.removed++;
            m_manifest.remove(relative);
        } else {
            result.keptEdited.append(relative);
            m_manifest.remove(relative);  // it is the user's file now
        }
    }

    save();
    return result;
}

}  // namespace WraithServer
