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
#pragma once

#include <QList>
#include <QSet>
#include <QString>
#include <optional>

#include "modplatform/ModIndex.h"

class QDir;

/** Friend server support: deciding which of an instance's mods belong on a dedicated server. */
namespace WraithServer {

enum class ModReason {
    MetadataBoth,        ///< Modrinth/CurseForge metadata says it runs on both sides
    MetadataServer,      ///< metadata says server-only
    MetadataClientOnly,  ///< metadata says client-only -> excluded
    KnownClientOnly,     ///< no metadata, but on Wraith's list of well-known client-only mods -> excluded
    NoInformation,       ///< no metadata and not a known client mod -> included (safer to include)
    UserIncluded,        ///< user override
    UserExcluded,        ///< user override
};

struct ModEntry {
    QString fileName;  ///< file name in the instance's mods folder
    QString name;      ///< display name (metadata name, else file name)
    bool include = true;
    ModReason reason = ModReason::NoInformation;
};

QString describe(ModReason reason);

/** Matches a mod file name / slug against the built-in list of well-known client-only mods. */
bool isKnownClientOnly(const QString& fileNameOrSlug);

/**
 * Decide whether one mod goes on the server.
 * `side` is the metadata side if the mod has a .index entry. User overrides win over everything.
 */
ModEntry classify(const QString& fileName,
                  const QString& name,
                  std::optional<ModPlatform::SideType> side,
                  const QSet<QString>& userIncluded = {},
                  const QSet<QString>& userExcluded = {});

/** Classify every enabled .jar in modsDir, reading packwiz metadata from modsDir/.index. */
QList<ModEntry> classifyModsFolder(const QDir& modsDir, const QSet<QString>& userIncluded = {}, const QSet<QString>& userExcluded = {});

}  // namespace WraithServer
