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

/**
 * Java argument presets (garbage collector choices) with plain-language explanations, and memory recommendations.
 * Pure logic, unit tested. Applying a preset is always the user's choice; custom arguments are never replaced silently.
 */
namespace Wraith::Advanced {

struct JvmPreset {
    QString id;
    QString name;
    QString summary;      ///< one line
    QString explanation;  ///< when to use it, and the trade-off
    QString args;         ///< extra JVM arguments (memory is set separately)
    int minJava = 8;
};

QList<JvmPreset> jvmPresets();
/** The preset whose arguments are exactly `args` (whitespace-insensitive); "" when none ("custom" or empty). */
QString presetIdFor(const QString& args);
/** The highest Java major a preset needs that `javaMajor` can't run; empty when it is fine. */
QString presetProblem(const JvmPreset& preset, int javaMajor);

struct MemoryAdvice {
    int recommendedMiB = 0;
    QString reason;
    QString warning;  ///< about the current setting, if any
};

/**
 * A memory suggestion from the mod count, Minecraft version and the computer's RAM, plus a warning about the current
 * maximum (`currentMaxMiB`) when it is too low for the pack or leaves too little for the system.
 */
MemoryAdvice recommendMemory(int modCount, const QString& minecraftVersion, qint64 systemRamMiB, int currentMaxMiB);

}  // namespace Wraith::Advanced
