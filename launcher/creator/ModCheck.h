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
#include <QMap>
#include <QString>
#include <QStringList>
#include <optional>

/**
 * Local dependency and conflict checker for an instance's mods (modpack creator tools).
 *
 * Reads the metadata each mod ships (fabric.mod.json, quilt.mod.json, META-INF/mods.toml,
 * META-INF/neoforge.mods.toml), including mods bundled inside other mods, and reports missing or disabled
 * dependencies, declared incompatibilities, duplicates, mods for another loader and Minecraft version mismatches.
 * Everything runs on the computer; results are suggestions and nothing is changed automatically.
 */
namespace Wraith::Creator {

struct ModInfo {
    QString file;     ///< file name in the mods folder
    QString id;
    QString name;
    QString version;  ///< empty when the mod doesn't state a usable one
    QString loader;   ///< "fabric", "quilt", "forge" or "neoforge"
    bool enabled = true;
    QStringList provides;            ///< extra ids (aliases, bundled mods)
    QMap<QString, QString> depends;  ///< required: id -> version range ("" = any)
    QMap<QString, QString> breaks;   ///< incompatible: id -> version range ("" = any)
};

struct Issue {
    enum class Severity { Error, Warning, Info };
    Severity severity = Severity::Warning;
    QString title;
    QString detail;
    QString suggestion;  ///< what the user could do; never applied automatically
    QStringList files;
};

/** Read one mod file's metadata (all of its mods, if the jar holds several). Empty when nothing is recognised. */
QList<ModInfo> readModFile(const QString& path);

/**
 * Check a set of mods. `loader` is the instance's mod loader ("fabric", "quilt", "forge", "neoforge" or "" for
 * none) and `minecraft` its Minecraft version.
 */
QList<Issue> checkMods(const QList<ModInfo>& mods, const QString& loader, const QString& minecraft);

/**
 * Whether `version` satisfies a Fabric/Quilt style range (">=1.2 <2", "~1.20.1", "1.20.x", "*", "a || b") or a
 * Maven/Forge range ("[1.0,2.0)", "[47,)"). `understood` is false when the range syntax isn't recognised.
 */
bool versionSatisfies(const QString& version, const QString& range, bool* understood = nullptr);

/** Compare two version strings part by part (numbers numerically). <0, 0, >0. */
int compareVersions(const QString& a, const QString& b);

}  // namespace Wraith::Creator
