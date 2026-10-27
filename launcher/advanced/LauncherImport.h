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
#include <QStringList>

/**
 * Finds instances in other launchers' folders so they can be copied into Wraith: Prism, MultiMC and PolyMC (same
 * format, copied as they are), CurseForge app, Modrinth App, ATLauncher, GDLauncher and the official launcher.
 * Other launchers' folders are only read, never changed. Pure logic over folders, unit tested.
 */
namespace Wraith::Advanced {

struct ForeignInstance {
    QString launcher;    ///< "Prism Launcher", "CurseForge", ...
    QString name;
    QString path;        ///< the instance folder in the other launcher
    QString gameDir;     ///< folder with mods/, config/, saves/ (may equal path)
    QString minecraft;   ///< version, e.g. "1.20.1"
    QString loader;      ///< "", "fabric", "quilt", "forge", "neoforge"
    QString loaderVersion;
    bool prismFormat = false;  ///< instance.cfg + mmc-pack.json: copy the whole folder
};

struct LauncherLocation {
    QString launcher;
    QString root;  ///< the launcher's data folder
};

/** Where the supported launchers keep their data on this system (folders that exist). */
QList<LauncherLocation> defaultLocations();

/** Instances found under one launcher's data folder. */
QList<ForeignInstance> scanLauncher(const LauncherLocation& location);

/** "forge-47.2.0" / "fabric-0.15.7-1.20.1" style loader names (CurseForge, GDLauncher) split into loader and version. */
void parseLoaderName(const QString& name, QString* loader, QString* version);

}  // namespace Wraith::Advanced
