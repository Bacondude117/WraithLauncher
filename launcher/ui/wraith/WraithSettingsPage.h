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

#include <QWidget>

#include "WraithShell.h"

namespace Wraith {

/** The Settings page: categories with simple controls first and the original settings pages under Advanced. */
class SettingsPage : public QWidget, public ShellPage {
    Q_OBJECT
   public:
    SettingsPage(Shell* shell, QWidget* parent);

    void selectCategory(const QString& category);

   private:
    Shell* m_shell;
};

}  // namespace Wraith
