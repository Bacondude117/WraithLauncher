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

#include <QString>
#include <QStringList>

#include "Result.h"

/**
 * Optional shared folders: an instance's resource packs, shader packs, screenshots or schematics can point at one
 * folder shared by every instance that opts in (<data>/shared/<folder>).
 *
 * The instance folder becomes a link (a directory junction on Windows, which needs no admin rights; a symlink
 * elsewhere). Sharing never deletes or overwrites: the instance's files move into the shared folder unless a file of the
 * same name is already there, and anything left over is kept in "<folder>.before-sharing". Unsharing copies the shared
 * files back into a normal folder.
 */
namespace Wraith::Advanced::SharedFolders {

/** Folders that can be shared. Worlds aren't: one world in several Minecraft versions gets corrupted. */
const QStringList& shareableFolders();

bool isShared(const QString& gameRoot, const QString& folder, const QString& sharedRoot);

/** Share gameRoot/folder through sharedRoot/folder. Returns a note for the user (what moved or was kept). */
Result<QString> share(const QString& gameRoot, const QString& folder, const QString& sharedRoot);

/** Make gameRoot/folder a normal folder again, with a copy of the shared files. */
Result<> unshare(const QString& gameRoot, const QString& folder);

}  // namespace Wraith::Advanced::SharedFolders
