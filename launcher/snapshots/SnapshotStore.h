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

#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QString>
#include <QStringList>
#include <functional>
#include <optional>

#include "Result.h"

/**
 * Instance snapshots: point-in-time copies of an instance's files that can be compared, restored and exported.
 *
 * Storage is plain files inside the instance, so it stays with the instance and needs no special tools:
 *   <instance>/.wraith/snapshots/objects/<ab>/<sha1>   file contents, stored once however many snapshots use them
 *   <instance>/.wraith/snapshots/<id>.json             one snapshot: metadata + relative path -> sha1, size
 *   <instance>/.wraith/snapshots/hashcache.json        path/size/mtime -> sha1, so unchanged files aren't re-read
 *
 * Worlds (saves/), logs, crash reports, screenshots, caches and Wraith's own folders are left out by default; worlds
 * can be included per snapshot. This is pure file logic with no UI, so it is unit tested (tests/Snapshot_test.cpp).
 */
namespace Wraith::Snapshots {

struct FileEntry {
    QString hash;  ///< SHA-1, hex
    qint64 size = 0;
    bool operator==(const FileEntry& other) const { return hash == other.hash; }
};
using FileMap = QMap<QString, FileEntry>;  ///< relative path ('/' separators) -> content

struct Snapshot {
    QString id;
    QDateTime created;
    QString label;    ///< user-given name, may be empty
    QString trigger;  ///< what caused it: "Manual", "Before a mod change", ...
    bool automatic = false;
    bool pinned = false;
    bool includesWorlds = false;
    FileMap files;
    QStringList skipped;  ///< in-scope files that were too large to keep

    qint64 totalSize() const;
};

/** Differences going from one file set to another. */
struct Diff {
    QStringList added;    ///< only in the target
    QStringList removed;  ///< only in the source
    QStringList changed;  ///< in both, different content
    bool isEmpty() const { return added.isEmpty() && removed.isEmpty() && changed.isEmpty(); }
    int count() const { return static_cast<int>(added.size() + removed.size() + changed.size()); }
};

struct Options {
    QString label;
    QString trigger;
    bool automatic = false;
    bool includeWorlds = false;
};

using Progress = std::function<void(int done, int total)>;

class Store {
   public:
    static constexpr qint64 s_maxFileSize = 512LL * 1024 * 1024;

    /** instanceRoot: the instance folder; gameRoot: its Minecraft folder (".minecraft" or "minecraft" inside it). */
    Store(QString instanceRoot, QString gameRoot);

    QString storeDir() const;

    /** All snapshots, newest first. */
    QList<Snapshot> list() const;
    std::optional<Snapshot> get(const QString& id) const;

    Result<Snapshot> create(const Options& options, const Progress& progress = {});
    bool setPinned(const QString& id, bool pinned);
    bool setLabel(const QString& id, const QString& label);
    /** Delete one snapshot and the stored files only it used. */
    bool remove(const QString& id);

    /**
     * Keep at most `ringSize` unpinned snapshots by removing the oldest unpinned ones (0 = keep all).
     * Pinned snapshots are never removed. Returns the removed ids.
     */
    QStringList prune(int ringSize);

    /** The instance's current files, in the same scope a snapshot uses. */
    FileMap scanCurrent(bool includeWorlds, const Progress& progress = {}, QStringList* skipped = nullptr);

    static Diff diff(const FileMap& from, const FileMap& to);
    /**
     * What restoring would do: added = brought back, removed = deleted, changed = replaced.
     * instance.cfg (playtime, settings) is never rolled back.
     */
    Diff restorePreview(const Snapshot& snapshot);
    /** Make the instance's in-scope files match the snapshot. The caller confirms first (see the preview). */
    Result<> restore(const Snapshot& snapshot, const Progress& progress = {});

    Result<> exportToFolder(const Snapshot& snapshot, const QString& folder) const;
    Result<> exportToZip(const Snapshot& snapshot, const QString& zipPath) const;

    /** The stored contents of one file of a snapshot (by its hash). */
    QByteArray contents(const QString& hash) const;

    /** Bytes used by stored file contents (shared between snapshots, so less than the sum of their sizes). */
    qint64 storageBytes() const;

    /** Whether a path relative to the instance belongs in a snapshot. */
    bool inScope(const QString& relativePath, bool includeWorlds) const;

   private:
    QString objectPath(const QString& hash) const;
    QString snapshotPath(const QString& id) const;
    bool save(const Snapshot& snapshot) const;
    void collectGarbage();
    void walk(const QString& dir, bool includeWorlds, QStringList& out) const;
    QString hashFile(const QString& relative, const QFileInfo& info);
    void loadHashCache();
    void saveHashCache() const;

    QString m_root;
    QString m_gameDir;  ///< game folder relative to the instance root, e.g. ".minecraft"
    struct CachedHash {
        qint64 size;
        qint64 mtime;
        QString hash;
    };
    QHash<QString, CachedHash> m_hashCache;
    bool m_hashCacheLoaded = false;
};

}  // namespace Wraith::Snapshots
