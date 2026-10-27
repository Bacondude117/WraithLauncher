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

#include "ui/pages/BasePage.h"

class MinecraftInstance;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTreeWidget;

namespace WraithServer {
class FriendServer;
}

/** "Friend Server" instance page: turn the instance into a local dedicated server and control it. */
class ServerDashboardPage : public QWidget, public BasePage {
    Q_OBJECT
   public:
    explicit ServerDashboardPage(MinecraftInstance* instance, QWidget* parent = nullptr);

    QString displayName() const override { return tr("Friend Server"); }
    QIcon icon() const override { return QIcon::fromTheme("launch"); }
    QString id() const override { return "friend_server"; }
    void openedImpl() override;

   private:
    void updateState();
    void reloadMods();
    void appendConsole(const QString& line);
    QString joinHint() const;

   private:
    WraithServer::FriendServer* m_server = nullptr;
    QLabel* m_status = nullptr;
    QLabel* m_join = nullptr;
    QPushButton* m_setupButton = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_stopButton = nullptr;
    QPushButton* m_folderButton = nullptr;
    QSpinBox* m_port = nullptr;
    QSpinBox* m_memory = nullptr;
    QCheckBox* m_eula = nullptr;
    QTreeWidget* m_mods = nullptr;
    QLabel* m_modsHint = nullptr;
    QPlainTextEdit* m_console = nullptr;
    QLineEdit* m_command = nullptr;
    bool m_loadingMods = false;
};
