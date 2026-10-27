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

// Wraith Launcher's hooks into MainWindow, kept out of MainWindow.cpp to ease rebases on Prism.
// MainWindow stays the controller for every action; when the Wraith shell is active, the classic central widget,
// toolbars, menu bar and status bar are taken out of the window (not deleted) and the shell takes their place.

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QStackedWidget>

#include "Application.h"
#include "InstanceList.h"
#include "InstanceTask.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/wraith/WraithShell.h"

void MainWindow::setupWraithShell()
{
    if (APPLICATION->settings()->get("WraithClassicLayout").toBool())
        return;

    // Removing the toolbars (instead of hiding them) also keeps restoreState() from bringing them back.
    for (QToolBar* bar : std::initializer_list<QToolBar*>{ ui->mainToolBar, ui->instanceToolBar, ui->newsToolBar }) {
        removeToolBar(bar);
        bar->hide();
    }
    ui->menuBar->hide();
    statusBar()->hide();

    // The classic central widget (with the instance view that MainWindow's actions use for the selection) stays
    // alive behind the shell.
    auto* root = new QStackedWidget(this);
    root->addWidget(takeCentralWidget());
    m_wraithShell = new Wraith::Shell(this, root);
    root->addWidget(m_wraithShell);
    root->setCurrentWidget(m_wraithShell);
    setCentralWidget(root);
    setMinimumSize(980, 640);

    // keyboard shortcuts of actions whose toolbars and menus are gone: Ctrl+N, Ctrl+, Ctrl+W, Ctrl+Z (undo delete)
    // and the account switches (Ctrl+1..9, Ctrl+0). Instance shortcuts (Ctrl+K/G/D/I, Delete, F2) live on the
    // instance page, which knows which instance they mean.
    addActions({ ui->actionAddInstance, ui->actionSettings, ui->actionCloseWindow, ui->actionUndoTrashInstance });
    // QKeySequence::Preferences is empty on Windows and Linux; Ctrl+, is what DESIGN.md promises everywhere
    if (!ui->actionSettings->shortcuts().contains(QKeySequence(tr("Ctrl+,"))))
        ui->actionSettings->setShortcuts(ui->actionSettings->shortcuts() << QKeySequence(tr("Ctrl+,")));
    addActions(ui->accountsMenu->actions());
}

void MainWindow::wraithShowInstance(MinecraftInstance* instance, const QString& page)
{
    APPLICATION->showMainWindow();
    m_wraithShell->showInstance(instance, page);
}

void MainWindow::wraithShowSettings(const QString& page)
{
    APPLICATION->showMainWindow();
    m_wraithShell->showSettings(page);
}

void MainWindow::wraithNewInstanceDialog(const QString& group, const QString& pageId, const QString& searchTerm)
{
    // same flow as addInstance(), opened on a chosen page
    const QString groupName = group.isEmpty() ? APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString() : group;
    NewInstanceDialog dialog(groupName, QString(), {}, this);
    if (!pageId.isEmpty())
        dialog.selectPage(pageId, searchTerm);
    if (!dialog.exec())
        return;
    APPLICATION->settings()->set("LastUsedGroupForNewInstance", dialog.instGroup());
    APPLICATION->settings()->set("LastUsedInstDirForNewInstance", dialog.instDir());
    if (InstanceTask* task = dialog.extractTask())
        instanceFromInstanceTask(task);
}

void MainWindow::wraithShowNewInstance()
{
    m_wraithShell->showNewInstance();
}

void MainWindow::wraithLeavePage()
{
    m_wraithShell->leaveCurrentPage();
}

void MainWindow::wraithRenameInstance(MinecraftInstance* instance, const QString& name)
{
    if (!instance)
        return;
    setSelectedInstanceById(instance->id());
    auto* instances = APPLICATION->instances();
    const auto oldName = instance->name();
    if (!instances->setData(instances->index(instances->getInstIndex(instance)), name, Qt::EditRole))
        return;
    handleInstanceRename(oldName, instance->name());
}
