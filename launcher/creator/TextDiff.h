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

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

/** Line diffs for the config diff (modpack creator tools). Pure logic, unit tested. */
namespace Wraith::Creator {

struct DiffLine {
    enum class Kind { Same, Added, Removed };
    Kind kind;
    QString text;
    int oldLine = 0;  ///< 1-based, 0 for added lines
    int newLine = 0;  ///< 1-based, 0 for removed lines
};

/** The full line diff (longest common subsequence). Returns nothing for inputs over `maxLines` lines. */
QList<DiffLine> diffLines(const QStringList& before, const QStringList& after, int maxLines = 5000);

/** A unified diff ("@@ -a,b +c,d @@" hunks) with `context` lines around changes. */
QString unifiedDiff(const QString& before, const QString& after, const QString& name, int context = 3);

/** true when the data looks like text (no NUL bytes in the first 8 KB). */
bool looksLikeText(const QByteArray& data);

}  // namespace Wraith::Creator
