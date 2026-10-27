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

#include "SnapshotDiff.h"

#include <QHash>
#include <QRegularExpression>

#include <algorithm>

namespace Wraith::Snapshots {

QString areaName(Area area)
{
    switch (area) {
        case Area::Components:
            return "Minecraft and loader";
        case Area::Mods:
            return "Mods";
        case Area::Config:
            return "Config";
        case Area::ResourcePacks:
            return "Resource packs";
        case Area::ShaderPacks:
            return "Shader packs";
        case Area::Worlds:
            return "Worlds";
        case Area::Other:
            return "Other files";
    }
    return {};
}

Area areaOf(const QString& path, const QString& gameDir)
{
    if (path == "mmc-pack.json" || path.startsWith("patches/") || path.startsWith("jarmods/"))
        return Area::Components;
    const QString prefix = gameDir.isEmpty() ? QString() : gameDir + "/";
    if (!prefix.isEmpty() && !path.startsWith(prefix))
        return Area::Other;
    const QString top = path.mid(prefix.size()).section('/', 0, 0);
    if (top == "mods" || top == "coremods")
        return Area::Mods;
    if (top == "config" || top == "defaultconfigs" || top == "kubejs" || top == "scripts")
        return Area::Config;
    if (top == "resourcepacks" || top == "texturepacks")
        return Area::ResourcePacks;
    if (top == "shaderpacks")
        return Area::ShaderPacks;
    if (top == "saves")
        return Area::Worlds;
    return Area::Other;
}

QString displayName(const QString& path)
{
    QString name = path.section('/', -1);
    if (name.endsWith(".disabled"))
        name.chop(9);
    static const QRegularExpression s_extension(R"(\.(jar|zip|litemod)$)", QRegularExpression::CaseInsensitiveOption);
    name.remove(s_extension);
    return name;
}

QString modStem(const QString& fileName)
{
    static const QRegularExpression s_split("[-_ ]");
    static const QRegularExpression s_version(R"(^(v?\d|mc\d|\d))", QRegularExpression::CaseInsensitiveOption);
    QStringList kept;
    for (const auto& token : displayName(fileName).split(s_split, Qt::SkipEmptyParts)) {
        if (s_version.match(token).hasMatch())
            break;
        kept.append(token.toLower());
    }
    return kept.isEmpty() ? displayName(fileName).toLower() : kept.join('-');
}

namespace {
bool isModFile(const QString& path, const QString& gameDir)
{
    if (areaOf(path, gameDir) != Area::Mods)
        return false;
    // metadata (packwiz .index) and config-like files in the mods folder aren't mods
    const QString inMods = path.section('/', gameDir.isEmpty() ? 1 : 2);
    return !inMods.contains('/');
}

QString withoutDisabled(const QString& path)
{
    return path.endsWith(".disabled") ? path.chopped(9) : path;
}
}  // namespace

QList<ModChange> modChanges(const Diff& diff, const QString& gameDir)
{
    QList<ModChange> result;
    QStringList added;
    QStringList removed;
    for (const auto& p : diff.added) {
        if (isModFile(p, gameDir))
            added.append(p);
    }
    for (const auto& p : diff.removed) {
        if (isModFile(p, gameDir))
            removed.append(p);
    }

    // enabled/disabled: the same file with or without ".disabled"
    for (auto it = removed.begin(); it != removed.end();) {
        const auto match = std::find_if(added.begin(), added.end(), [&](const QString& a) { return withoutDisabled(a) == withoutDisabled(*it); });
        if (match != added.end()) {
            const bool nowDisabled = match->endsWith(".disabled");
            result.append({ nowDisabled ? ModChange::Kind::Disabled : ModChange::Kind::Enabled, displayName(*it), *it, *match });
            added.erase(match);
            it = removed.erase(it);
        } else {
            ++it;
        }
    }
    // updated: a removed and an added file of the same mod (same name stem)
    for (auto it = removed.begin(); it != removed.end();) {
        const QString stem = modStem(*it);
        const auto match = std::find_if(added.begin(), added.end(), [&](const QString& a) { return modStem(a) == stem; });
        if (match != added.end()) {
            result.append({ ModChange::Kind::Updated, displayName(*match), *it, *match });
            added.erase(match);
            it = removed.erase(it);
        } else {
            ++it;
        }
    }
    for (const auto& p : added)
        result.append({ ModChange::Kind::Added, displayName(p), {}, p });
    for (const auto& p : removed)
        result.append({ ModChange::Kind::Removed, displayName(p), p, {} });
    std::sort(result.begin(), result.end(), [](const ModChange& a, const ModChange& b) {
        if (a.kind != b.kind)
            return a.kind < b.kind;
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
    return result;
}

QList<FileChange> groupedChanges(const Diff& diff, const QString& gameDir)
{
    QList<FileChange> changes;
    for (const auto& p : diff.added)
        changes.append({ areaOf(p, gameDir), p, "added" });
    for (const auto& p : diff.removed)
        changes.append({ areaOf(p, gameDir), p, "removed" });
    for (const auto& p : diff.changed)
        changes.append({ areaOf(p, gameDir), p, "changed" });
    std::sort(changes.begin(), changes.end(), [](const FileChange& a, const FileChange& b) {
        if (a.area != b.area)
            return a.area < b.area;
        return a.path.compare(b.path, Qt::CaseInsensitive) < 0;
    });
    return changes;
}

QString markdownChangelog(const QString& title, const Diff& diff, const QString& gameDir)
{
    QString out = QString("# %1\n").arg(title);
    if (diff.isEmpty())
        return out + "\nNo changes.\n";

    const auto mods = modChanges(diff, gameDir);
    auto section = [&](ModChange::Kind kind, const QString& heading) {
        QStringList lines;
        for (const auto& m : mods) {
            if (m.kind != kind)
                continue;
            if (kind == ModChange::Kind::Updated)
                lines.append(QString("- %1 (was %2)").arg(m.name, displayName(m.oldFile)));
            else
                lines.append(QString("- %1").arg(m.name));
        }
        if (!lines.isEmpty())
            out += QString("\n## %1\n%2\n").arg(heading, lines.join('\n'));
    };
    section(ModChange::Kind::Added, "Added mods");
    section(ModChange::Kind::Updated, "Updated mods");
    section(ModChange::Kind::Removed, "Removed mods");
    section(ModChange::Kind::Enabled, "Enabled mods");
    section(ModChange::Kind::Disabled, "Disabled mods");

    // everything else, counted per area
    QMap<int, int> counts;
    for (const auto& c : groupedChanges(diff, gameDir)) {
        if (c.area == Area::Mods && isModFile(c.path, gameDir))
            continue;
        counts[static_cast<int>(c.area)]++;
    }
    if (!counts.isEmpty()) {
        out += "\n## Other changes\n";
        for (auto it = counts.begin(); it != counts.end(); ++it)
            out += QString("- %1: %2 file(s) changed\n").arg(areaName(static_cast<Area>(it.key()))).arg(it.value());
    }
    return out;
}

}  // namespace Wraith::Snapshots
