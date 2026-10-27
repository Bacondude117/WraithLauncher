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
#include <optional>

/**
 * Encrypts files that hold login tokens (accounts.json) so they are never stored in plain text.
 *
 * Windows: DPAPI (CryptProtectData), bound to the current Windows user; nothing to configure, no key file.
 * Other platforms: not available yet (see VALUES_AUDIT.md); data is stored as before, in a file only the
 * owner can read.
 *
 * Protected data starts with a short text header so the format is recognisable and plain files from Prism
 * (or older Wraith builds) are still read.
 */
namespace Wraith::SecretStore {

/** true when this platform can protect data. */
bool available();

/** Protect `plain`. Returns `plain` unchanged where protection isn't available. */
QByteArray protect(const QByteArray& plain);

/** true when `data` was produced by protect() (and isn't plain text). */
bool isProtected(const QByteArray& data);

/**
 * Undo protect(). Plain (unprotected) input is returned as it is. nullopt when protected data can't be
 * decrypted, for example after copying the file to another user account or computer.
 */
std::optional<QByteArray> unprotect(const QByteArray& data);

}  // namespace Wraith::SecretStore
