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

#include "modplatform/helpers/HashUtils.h"
#include "net/ChecksumValidator.h"

namespace Net {

/**
 * A checksum validator for a published hash, or nullptr when the hash is empty or the algorithm unknown.
 * `algorithm` is a name like "sha1", "sha512" or "md5"; `hexHash` is the hex digest.
 */
inline Validator* makeChecksumValidator(const QString& algorithm, const QString& hexHash)
{
    if (hexHash.isEmpty())
        return nullptr;
    switch (Hashing::algorithmFromString(algorithm.toLower())) {
        case Hashing::Algorithm::Md5:
            return new ChecksumValidator(QCryptographicHash::Md5, hexHash);
        case Hashing::Algorithm::Sha1:
            return new ChecksumValidator(QCryptographicHash::Sha1, hexHash);
        case Hashing::Algorithm::Sha256:
            return new ChecksumValidator(QCryptographicHash::Sha256, hexHash);
        case Hashing::Algorithm::Sha512:
            return new ChecksumValidator(QCryptographicHash::Sha512, hexHash);
        default:
            return nullptr;
    }
}

}  // namespace Net
