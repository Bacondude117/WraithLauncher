// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher (fork of Prism Launcher)
 *  Copyright (C) 2026 Wraith Launcher Contributors
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
#include "ServerModFilter.h"

#include <QDir>
#include <QHash>
#include <QObject>
#include <QRegularExpression>

#include <algorithm>

#include "modplatform/packwiz/Packwiz.h"

namespace WraithServer {

namespace {
/**
 * Mods that are purely client-side (rendering, HUD, input, menus). Used only when a mod has no side
 * metadata. Kept conservative: a mod that is wrongly included usually just loads harmlessly on Fabric,
 * while a wrongly excluded gameplay mod breaks the server, so only clear-cut client mods are listed.
 */
const QStringList& knownClientOnly()
{
    static const QStringList s_list = {
        "sodium",          "iris",          "oculus",           "embeddium",       "rubidium",     "optifine",
        "optifabric",      "modmenu",       "betterf3",         "entityculling",   "dynamicfps",   "dynamic-fps",
        "fpsreducer",      "notenoughanimations", "ambientsounds", "presencefootsteps", "controlling", "mousetweaks",
        "zoomify",         "okzoomer",      "ok_zoomer",        "continuity",      "indium",       "lambdynamiclights",
        "sodiumextra",     "sodium-extra",  "reeses-sodium-options", "reesessodiumoptions", "betterbeds", "cullleaves",
        "enhancedblockentities", "immediatelyfast", "xaerominimap", "xaeros_minimap", "xaeroworldmap", "xaeros_worldmap",
        "journeymap",      "blur",          "legendarytooltips", "itemborders",   "borderlessmining", "fullbrightnesstoggle",
        "chatheads",       "screenshot-to-clipboard", "skinlayers3d", "3dskinlayers", "physicsmod", "fancymenu",
        "drippyloadingscreen", "loadingscreentips", "torohealth", "neat",        "inventoryhud", "betterthirdperson",
    };
    return s_list;
}

/** "Sodium-Fabric-0.5.8+mc1.20.1.jar" -> "sodiumfabric058mc1201": lowercase, letters/digits/-/_ only */
QString simplify(const QString& text)
{
    QString out;
    for (const QChar c : text.toLower()) {
        if (c.isLetterOrNumber() || c == '-' || c == '_')
            out.append(c);
    }
    return out;
}
}  // namespace

QString describe(ModReason reason)
{
    switch (reason) {
        case ModReason::MetadataBoth:
            return QObject::tr("Runs on client and server");
        case ModReason::MetadataServer:
            return QObject::tr("Server-side mod");
        case ModReason::MetadataClientOnly:
            return QObject::tr("Client-only (from mod metadata)");
        case ModReason::KnownClientOnly:
            return QObject::tr("Client-only (known client mod)");
        case ModReason::NoInformation:
            return QObject::tr("No side information; included to be safe");
        case ModReason::UserIncluded:
            return QObject::tr("Included by you");
        case ModReason::UserExcluded:
            return QObject::tr("Excluded by you");
    }
    return {};
}

bool isKnownClientOnly(const QString& fileNameOrSlug)
{
    QString base = fileNameOrSlug;
    if (base.endsWith(".disabled", Qt::CaseInsensitive))
        base.chop(9);
    if (base.endsWith(".jar", Qt::CaseInsensitive))
        base.chop(4);
    const QString simple = simplify(base);
    // The mod id is the leading part of the file name, before the version: split on -, _ or + boundaries
    // followed by a digit or a loader/version word, then compare against the list.
    static const QRegularExpression s_versionPart(R"([-_+](?=(\d|v\d|mc\d|fabric|forge|neoforge|quilt|universal)))");
    const QString lead = simplify(base.toLower().split(s_versionPart).first());
    const QString leadNoSeparators = QString(lead).remove('-').remove('_');
    for (const auto& known : knownClientOnly()) {
        const QString knownNoSeparators = QString(known).remove('-').remove('_');
        if (lead == known || leadNoSeparators == knownNoSeparators || simple == known)
            return true;
    }
    return false;
}

ModEntry classify(const QString& fileName,
                  const QString& name,
                  std::optional<ModPlatform::SideType> side,
                  const QSet<QString>& userIncluded,
                  const QSet<QString>& userExcluded)
{
    ModEntry entry;
    entry.fileName = fileName;
    entry.name = name.isEmpty() ? fileName : name;

    if (userExcluded.contains(fileName)) {
        entry.include = false;
        entry.reason = ModReason::UserExcluded;
    } else if (userIncluded.contains(fileName)) {
        entry.include = true;
        entry.reason = ModReason::UserIncluded;
    } else if (side.has_value() && *side == ModPlatform::SideType::ClientSide) {
        entry.include = false;
        entry.reason = ModReason::MetadataClientOnly;
    } else if (side.has_value() && *side == ModPlatform::SideType::ServerSide) {
        entry.reason = ModReason::MetadataServer;
    } else if (side.has_value() && *side == ModPlatform::SideType::UniversalSide) {
        entry.reason = ModReason::MetadataBoth;
    } else if (isKnownClientOnly(fileName)) {
        entry.include = false;
        entry.reason = ModReason::KnownClientOnly;
    } else {
        entry.reason = ModReason::NoInformation;
    }
    return entry;
}

QList<ModEntry> classifyModsFolder(const QDir& modsDir, const QSet<QString>& userIncluded, const QSet<QString>& userExcluded)
{
    // Map file name -> metadata, from packwiz-style .index entries written when mods are installed via the launcher.
    QHash<QString, Packwiz::V1::Mod> metadata;
    const QDir indexDir(modsDir.filePath(".index"));
    for (const auto& tomlName : indexDir.entryList({ "*.pw.toml" }, QDir::Files)) {
        const QString slug = tomlName.chopped(QString(".pw.toml").size());
        auto mod = Packwiz::V1::getIndexForMod(indexDir, slug);
        if (!mod.filename.isEmpty())
            metadata.insert(mod.filename, mod);
    }

    QList<ModEntry> entries;
    for (const auto& fileName : modsDir.entryList({ "*.jar" }, QDir::Files, QDir::Name | QDir::IgnoreCase)) {
        std::optional<ModPlatform::SideType> side;
        QString name;
        if (auto it = metadata.constFind(fileName); it != metadata.constEnd()) {
            name = it->name;
            if (it->side != ModPlatform::SideType::NoSide)
                side = it->side;
        }
        entries.append(classify(fileName, name, side, userIncluded, userExcluded));
    }
    std::stable_sort(entries.begin(), entries.end(),
                     [](const ModEntry& a, const ModEntry& b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });
    return entries;
}

}  // namespace WraithServer
