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

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

namespace WraithServer {

/**
 * Copies files into a server folder without ever overwriting or deleting something the user edited there.
 *
 * A manifest (relative path -> SHA-1 of the copy Wraith wrote) remembers what Wraith put in the folder. On the
 * next sync a file is only replaced or removed when it still matches that hash. Files the user added or changed
 * are kept and reported.
 */
class FileSync {
   public:
    struct Item {
        QString source;    ///< absolute source path
        QString relative;  ///< path inside the target folder, with '/' separators
    };
    struct Result {
        int copied = 0;
        int unchanged = 0;
        int removed = 0;
        QStringList keptEdited;  ///< target files left alone because the user changed or added them
        QStringList failed;
    };

    explicit FileSync(QString targetRoot, QString manifestPath);

    /**
     * Make the files under the managed `prefixes` (e.g. "mods/", "config/") match `items`, respecting user edits.
     * Saves the manifest afterwards.
     */
    Result sync(const QList<Item>& items, const QStringList& prefixes);

    static QString fileHash(const QString& path);

   private:
    void load();
    void save() const;

    QString m_root;
    QString m_manifestPath;
    QHash<QString, QString> m_manifest;
};

}  // namespace WraithServer
