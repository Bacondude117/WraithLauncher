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

#include "TextDiff.h"

#include <QByteArray>
#include <vector>

namespace Wraith::Creator {

QList<DiffLine> diffLines(const QStringList& before, const QStringList& after, int maxLines)
{
    const int n = static_cast<int>(before.size());
    const int m = static_cast<int>(after.size());
    if (n > maxLines || m > maxLines)
        return {};
    // skip the common start and end, then LCS on the middle
    int start = 0;
    while (start < n && start < m && before[start] == after[start])
        start++;
    int endB = n;
    int endA = m;
    while (endB > start && endA > start && before[endB - 1] == after[endA - 1]) {
        endB--;
        endA--;
    }
    const int rows = endB - start;
    const int cols = endA - start;
    std::vector<int> lcs(static_cast<size_t>(rows + 1) * static_cast<size_t>(cols + 1), 0);
    auto at = [&](int i, int j) -> int& { return lcs[static_cast<size_t>(i) * static_cast<size_t>(cols + 1) + static_cast<size_t>(j)]; };
    for (int i = rows - 1; i >= 0; i--)
        for (int j = cols - 1; j >= 0; j--)
            at(i, j) = before[start + i] == after[start + j] ? at(i + 1, j + 1) + 1 : std::max(at(i + 1, j), at(i, j + 1));

    QList<DiffLine> out;
    for (int i = 0; i < start; i++)
        out.append({ DiffLine::Kind::Same, before[i], i + 1, i + 1 });
    int i = 0;
    int j = 0;
    while (i < rows || j < cols) {
        if (i < rows && j < cols && before[start + i] == after[start + j]) {
            out.append({ DiffLine::Kind::Same, before[start + i], start + i + 1, start + j + 1 });
            i++;
            j++;
        } else if (i < rows && (j == cols || at(i + 1, j) >= at(i, j + 1))) {
            // removals before additions, like unified diffs show them
            out.append({ DiffLine::Kind::Removed, before[start + i], start + i + 1, 0 });
            i++;
        } else {
            out.append({ DiffLine::Kind::Added, after[start + j], 0, start + j + 1 });
            j++;
        }
    }
    for (int k = 0; k < n - endB; k++)
        out.append({ DiffLine::Kind::Same, before[endB + k], endB + k + 1, endA + k + 1 });
    return out;
}

QString unifiedDiff(const QString& before, const QString& after, const QString& name, int context)
{
    const QStringList a = before.split('\n');
    const QStringList b = after.split('\n');
    const auto lines = diffLines(a, b);
    if (lines.isEmpty() && (a.size() > 5000 || b.size() > 5000))
        return QString("%1: too large to compare line by line\n").arg(name);

    QString out = QString("--- %1 (before)\n+++ %1 (after)\n").arg(name);
    const int count = static_cast<int>(lines.size());
    int index = 0;
    bool any = false;
    while (index < count) {
        // find the next change
        while (index < count && lines[index].kind == DiffLine::Kind::Same)
            index++;
        if (index >= count)
            break;
        any = true;
        int hunkStart = std::max(0, index - context);
        int hunkEnd = index;
        // extend while changes are within 2*context of each other
        int lastChange = index;
        for (int k = index; k < count; k++) {
            if (lines[k].kind != DiffLine::Kind::Same)
                lastChange = k;
            else if (k - lastChange > 2 * context)
                break;
            hunkEnd = k;
        }
        hunkEnd = std::min(count - 1, lastChange + context);
        int oldStart = 0, oldCount = 0, newStart = 0, newCount = 0;
        QString body;
        for (int k = hunkStart; k <= hunkEnd; k++) {
            const auto& l = lines[k];
            if (l.kind != DiffLine::Kind::Added) {
                if (!oldStart)
                    oldStart = l.oldLine;
                oldCount++;
            }
            if (l.kind != DiffLine::Kind::Removed) {
                if (!newStart)
                    newStart = l.newLine;
                newCount++;
            }
            body += (l.kind == DiffLine::Kind::Same ? " " : l.kind == DiffLine::Kind::Added ? "+" : "-") + l.text + "\n";
        }
        out += QString("@@ -%1,%2 +%3,%4 @@\n").arg(oldStart).arg(oldCount).arg(newStart).arg(newCount) + body;
        index = hunkEnd + 1;
    }
    if (!any)
        out += "(no differences)\n";
    return out;
}

bool looksLikeText(const QByteArray& data)
{
    return !data.left(8192).contains('\0');
}

}  // namespace Wraith::Creator
