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

#include <QList>
#include <QString>

#include "snapshots/SnapshotStore.h"

/**
 * Turns a file-level diff into something people read: changes grouped by area (mods, config, ...), mods paired into
 * "updated" / "enabled" / "disabled", and a Markdown changelog. Pure logic, unit tested.
 */
namespace Wraith::Snapshots {

enum class Area { Components, Mods, Config, ResourcePacks, ShaderPacks, Worlds, Other };
QString areaName(Area area);

/** Which area a path (relative to the instance, '/' separators) belongs to. gameDir is e.g. ".minecraft". */
Area areaOf(const QString& path, const QString& gameDir);

struct ModChange {
    enum class Kind { Added, Removed, Updated, Enabled, Disabled };
    Kind kind;
    QString name;  ///< readable name, from the file name
    QString oldFile;
    QString newFile;
};

/** A file name without folders, ".disabled" and the archive extension. */
QString displayName(const QString& path);
/** The mod's identity for pairing versions: "sodium-fabric-0.5.8+mc1.20.1.jar" -> "sodium-fabric". */
QString modStem(const QString& fileName);

/** Mod changes in the mods folder, with removed+added versions of the same mod paired as "Updated". */
QList<ModChange> modChanges(const Diff& diff, const QString& gameDir);

struct FileChange {
    Area area;
    QString path;
    QString kind;  ///< "added", "removed" or "changed"
};
QList<FileChange> groupedChanges(const Diff& diff, const QString& gameDir);

/** A Markdown changelog from `from` to `to` (title, mods added/updated/removed, then other areas as counts). */
QString markdownChangelog(const QString& title, const Diff& diff, const QString& gameDir);

}  // namespace Wraith::Snapshots
