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

#include "SecretStore.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace Wraith::SecretStore {

namespace {
const QByteArray s_header = "WRAITH-PROTECTED-V1:DPAPI\n";

#ifdef Q_OS_WIN
// "Entropy" only separates Wraith's blobs from other DPAPI users; the secret is the Windows login.
const QByteArray s_entropy = "io.github.wraithlauncher.accounts";

std::optional<QByteArray> dpapi(const QByteArray& input, bool encrypt)
{
    DATA_BLOB in{ static_cast<DWORD>(input.size()), reinterpret_cast<BYTE*>(const_cast<char*>(input.data())) };
    DATA_BLOB entropy{ static_cast<DWORD>(s_entropy.size()), reinterpret_cast<BYTE*>(const_cast<char*>(s_entropy.data())) };
    DATA_BLOB out{ 0, nullptr };
    const BOOL ok = encrypt ? CryptProtectData(&in, L"Wraith Launcher accounts", &entropy, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out)
                            : CryptUnprotectData(&in, nullptr, &entropy, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
    if (!ok)
        return std::nullopt;
    QByteArray result(reinterpret_cast<const char*>(out.pbData), static_cast<qsizetype>(out.cbData));
    SecureZeroMemory(out.pbData, out.cbData);
    LocalFree(out.pbData);
    return result;
}
#endif
}  // namespace

bool available()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QByteArray protect(const QByteArray& plain)
{
#ifdef Q_OS_WIN
    if (auto encrypted = dpapi(plain, true))
        return s_header + encrypted->toBase64() + '\n';
    qWarning() << "SecretStore: CryptProtectData failed, error" << GetLastError();
#endif
    return plain;
}

bool isProtected(const QByteArray& data)
{
    return data.startsWith(s_header);
}

std::optional<QByteArray> unprotect(const QByteArray& data)
{
    if (!isProtected(data))
        return data;
#ifdef Q_OS_WIN
    return dpapi(QByteArray::fromBase64(data.mid(s_header.size()).trimmed()), false);
#else
    return std::nullopt;
#endif
}

}  // namespace Wraith::SecretStore
