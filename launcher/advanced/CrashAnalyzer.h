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
 * Local log and crash analysis: log line levels for the log viewer, and known crash causes explained in plain language
 * with a suggestion. Nothing leaves the computer and nothing is changed; results are suggestions.
 */
namespace Wraith::Advanced {

enum class LogLevel { Unknown, Debug, Info, Warn, Error, Fatal };

/** The level of one log line ("[12:00:00] [Render thread/WARN]: ...", "[12:00:00 ERROR]", "Exception in thread"...). */
LogLevel lineLevel(const QString& line, LogLevel previous = LogLevel::Unknown);
QString levelName(LogLevel level);

struct CrashFinding {
    QString id;           ///< stable key, e.g. "out-of-memory"
    QString title;        ///< plain-language headline
    QString explanation;  ///< what it means
    QString suggestion;   ///< what to try; never applied automatically
    QString evidence;     ///< the line that gave it away
    QStringList mods;     ///< mods involved, when known
    bool memory = false;  ///< offers memory settings
    bool java = false;    ///< offers Java settings
    bool rollback = true; ///< a snapshot rollback may help
};

/** Analyse a crash report and/or game log. Most specific findings first. */
QList<CrashFinding> analyzeCrash(const QString& text);

}  // namespace Wraith::Advanced
