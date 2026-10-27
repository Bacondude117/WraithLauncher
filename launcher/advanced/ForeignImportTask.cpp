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

#include "ForeignImportTask.h"

#include <QDir>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

ForeignImportTask::ForeignImportTask(Wraith::Advanced::ForeignInstance source) : m_source(std::move(source)) {}

const QStringList& ForeignImportTask::sharedGameFolderWhitelist()
{
    static const QStringList s_list = { "mods",         "config",      "saves",       "resourcepacks", "shaderpacks",
                                        "options.txt",  "servers.dat", "schematics",  "defaultconfigs" };
    return s_list;
}

const QStringList& ForeignImportTask::blacklist()
{
    // accounts and tokens, logs, other launchers' own metadata, and game files the launcher downloads itself
    static const QStringList s_list = { "launcher_profiles.json", "launcher_accounts.json", "launcher_accounts_microsoft_store.json",
                                        "logs", "crash-reports", "minecraftinstance.json", "instance.json", "profile.json",
                                        "natives", "libraries", "versions", "assets", "bin", ".cache", ".mixin.out", "webcache",
                                        ".curseclient", "Install" };
    return s_list;
}

void ForeignImportTask::executeTask()
{
    setStatus(tr("Copying %1 from %2").arg(m_source.name, m_source.launcher));
    const QString staging = m_stagingPath;
    const auto source = m_source;

    // copying can take a while (worlds, big packs), so it runs off the GUI thread
    auto future = QtConcurrent::run([staging, source]() -> QString {
        if (source.prismFormat) {
            FS::copy copy(source.path, staging);
            if (!copy())
                return tr("couldn't copy %1").arg(source.path);
            return {};
        }
        const QDir from(source.gameDir);
        const QString target = FS::PathCombine(staging, "minecraft");
        FS::ensureFolderPathExists(target);
        const bool shared = source.launcher == "Minecraft Launcher";
        for (const auto& entry : from.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
            const QString name = entry.fileName();
            if (shared ? !sharedGameFolderWhitelist().contains(name) : blacklist().contains(name))
                continue;
            FS::copy copy(entry.absoluteFilePath(), FS::PathCombine(target, name));
            if (!copy())
                return tr("couldn't copy %1").arg(entry.absoluteFilePath());
        }
        return {};
    });

    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
        const QString error = watcher->result();
        watcher->deleteLater();
        if (!error.isEmpty()) {
            emitFailed(error);
            return;
        }
        // describe the instance: Prism-format copies keep theirs and only get the new name
        MinecraftInstance instance(m_globalSettings, std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")),
                                   m_stagingPath);
        {
            const SettingsObject::Lock lock(instance.settings());
            if (!m_source.prismFormat) {
                auto* components = instance.getPackProfile();
                components->buildingFromScratch();
                components->setComponentVersion("net.minecraft", m_source.minecraft, true);
                static const QHash<QString, QString> s_uids = { { "fabric", "net.fabricmc.fabric-loader" },
                                                                { "quilt", "org.quiltmc.quilt-loader" },
                                                                { "forge", "net.minecraftforge" },
                                                                { "neoforge", "net.neoforged" } };
                if (s_uids.contains(m_source.loader) && !m_source.loaderVersion.isEmpty())
                    components->setComponentVersion(s_uids.value(m_source.loader), m_source.loaderVersion);
                components->saveNow();
                instance.setIconKey(m_instIcon);
            }
            instance.setName(name());
        }
        emitSucceeded();
    });
    watcher->setFuture(future);
}
