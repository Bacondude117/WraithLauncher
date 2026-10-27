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

#include "InstanceTask.h"
#include "advanced/LauncherImport.h"

/**
 * Creates a Wraith instance from another launcher's instance by copying (never moving) its files.
 * Prism-format instances are copied as they are; others get a new instance with the same Minecraft version and loader,
 * and their game files (mods, config, saves, resource packs, ...) are copied in. Launcher-specific files (accounts,
 * logs, the other launcher's metadata, game libraries) are left out.
 */
class ForeignImportTask : public InstanceTask {
    Q_OBJECT
   public:
    explicit ForeignImportTask(Wraith::Advanced::ForeignInstance source);

    /** Files and folders copied from a game folder that holds more than one instance's data (the official launcher). */
    static const QStringList& sharedGameFolderWhitelist();
    /** Names never copied from another launcher's instance folder. */
    static const QStringList& blacklist();

   protected:
    void executeTask() override;

   private:
    Wraith::Advanced::ForeignInstance m_source;
};
