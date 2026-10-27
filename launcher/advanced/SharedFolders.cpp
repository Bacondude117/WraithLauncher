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

#include "SharedFolders.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QProcess>

#include "FileSystem.h"

namespace Wraith::Advanced::SharedFolders {

namespace {
bool isLink(const QFileInfo& info)
{
    return info.isSymLink() || info.isJunction();
}

QString linkTarget(const QFileInfo& info)
{
    return QDir::cleanPath(info.isJunction() ? info.junctionTarget() : info.symLinkTarget());
}

bool makeLink(const QString& link, const QString& target)
{
#ifdef Q_OS_WIN
    // a directory junction: works without admin rights or Developer Mode (unlike symlinks)
    QProcess mklink;
    mklink.start("cmd.exe", { "/c", "mklink", "/J", QDir::toNativeSeparators(link), QDir::toNativeSeparators(target) });
    return mklink.waitForFinished(15000) && mklink.exitCode() == 0 && QFileInfo(link).isJunction();
#else
    return QFile::link(target, link);
#endif
}

bool removeLink(const QString& link)
{
#ifdef Q_OS_WIN
    return QDir().rmdir(link);  // removes the junction itself, never what it points to
#else
    return QFile::remove(link);
#endif
}
}  // namespace

const QStringList& shareableFolders()
{
    static const QStringList s_folders = { "resourcepacks", "shaderpacks", "screenshots", "schematics" };
    return s_folders;
}

bool isShared(const QString& gameRoot, const QString& folder, const QString& sharedRoot)
{
    const QFileInfo info(QDir(gameRoot).filePath(folder));
    if (!isLink(info))
        return false;
    return linkTarget(info).startsWith(QDir::cleanPath(QFileInfo(sharedRoot).absoluteFilePath()), Qt::CaseInsensitive);
}

Result<QString> share(const QString& gameRoot, const QString& folder, const QString& sharedRoot)
{
    if (!shareableFolders().contains(folder))
        return std::unexpected(QObject::tr("%1 can't be shared").arg(folder));
    const QString link = QDir(gameRoot).filePath(folder);
    const QString target = QDir(sharedRoot).filePath(folder);
    if (!QDir().mkpath(target))
        return std::unexpected(QObject::tr("couldn't create %1").arg(target));
    if (isLink(QFileInfo(link)))
        return std::unexpected(QObject::tr("%1 is already a link").arg(link));

    int moved = 0;
    int kept = 0;
    if (QFileInfo(link).isDir()) {
        // move the instance's files into the shared folder, never over a file that is already there
        for (const auto& entry : QDir(link).entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
            const QString destination = QDir(target).filePath(entry.fileName());
            if (!QFileInfo::exists(destination) && QFile::rename(entry.absoluteFilePath(), destination))
                moved++;
            else
                kept++;
        }
        if (kept > 0) {
            QString backup = link + ".before-sharing";
            for (int i = 2; QFileInfo::exists(backup); i++)
                backup = link + QString(".before-sharing-%1").arg(i);
            if (!QDir().rename(link, backup))
                return std::unexpected(QObject::tr("couldn't set %1 aside").arg(link));
        } else if (!QDir().rmdir(link)) {
            return std::unexpected(QObject::tr("couldn't replace %1").arg(link));
        }
    }
    if (!makeLink(link, target)) {
        QDir().mkpath(link);  // never leave the instance without the folder
        return std::unexpected(QObject::tr("couldn't link %1 to %2").arg(link, target));
    }
    QString note = QObject::tr("%1 file(s) moved into the shared folder.").arg(moved);
    if (kept > 0)
        note += ' ' + QObject::tr("%1 already existed there; the instance's copies are kept in %2.before-sharing.").arg(kept).arg(folder);
    return note;
}

Result<> unshare(const QString& gameRoot, const QString& folder)
{
    const QString link = QDir(gameRoot).filePath(folder);
    const QFileInfo info(link);
    if (!isLink(info))
        return {};
    const QString target = linkTarget(info);
    if (!removeLink(link))
        return std::unexpected(QObject::tr("couldn't remove the link %1").arg(link));
    // a normal folder again, with a copy of what was shared (the shared folder keeps its files for other instances)
    FS::copy copy(target, link);
    if (!copy())
        return std::unexpected(QObject::tr("couldn't copy the shared files into %1").arg(link));
    QDir().mkpath(link);
    return {};
}

}  // namespace Wraith::Advanced::SharedFolders
