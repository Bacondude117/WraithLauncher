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

// Wraith Launcher's Modrinth-style main window layout, kept out of MainWindow.cpp to ease rebases on Prism.
// Nothing is removed: the main toolbar (with every original action) becomes a left navigation rail,
// a library header with search goes above the instance grid, and the Launch button is styled as a Play button.
// All of it is undone when another theme is selected.

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "Application.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"
#include "ui/dialogs/FriendServersDialog.h"
#include "ui/instanceview/InstanceProxyModel.h"
#include "ui/instanceview/InstanceView.h"
#include "ui/themes/WraithTheme.h"

namespace {
void repolish(QWidget* widget)
{
    if (!widget)
        return;
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}
}  // namespace

void MainWindow::setupWraithLayout()
{
    m_classicToolButtonStyle = ui->mainToolBar->toolButtonStyle();
    m_classicIconSize = ui->mainToolBar->iconSize();

    // library header: title, search, primary action
    m_wraithHeader = new QWidget(m_libraryPane);
    m_wraithHeader->setObjectName("wraithHeader");
    auto* header = new QHBoxLayout(m_wraithHeader);
    header->setContentsMargins(18, 14, 18, 10);
    auto* title = new QLabel(tr("Library"), m_wraithHeader);
    title->setObjectName("wraithTitle");
    header->addWidget(title);
    header->addStretch(1);
    m_wraithSearch = new QLineEdit(m_wraithHeader);
    m_wraithSearch->setObjectName("wraithSearch");
    m_wraithSearch->setPlaceholderText(tr("Search instances..."));
    m_wraithSearch->setClearButtonEnabled(true);
    m_wraithSearch->setFixedWidth(280);
    header->addWidget(m_wraithSearch);
    auto* newInstance = new QPushButton(QIcon::fromTheme("new"), tr("New instance"), m_wraithHeader);
    newInstance->setObjectName("wraithAccentButton");
    header->addWidget(newInstance);
    qobject_cast<QVBoxLayout*>(m_libraryPane->layout())->insertWidget(0, m_wraithHeader);

    proxymodel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    connect(m_wraithSearch, &QLineEdit::textChanged, proxymodel, &QSortFilterProxyModel::setFilterFixedString);
    connect(newInstance, &QPushButton::clicked, this, [this] { addInstance(); });

    // navigation rail entries (inserted ahead of the original toolbar actions)
    auto* library = new QAction(QIcon::fromTheme("minecraft"), tr("Library"), this);
    library->setToolTip(tr("Library: your instances"));
    library->setCheckable(true);
    library->setChecked(true);
    connect(library, &QAction::triggered, this, [this, library] {
        library->setChecked(true);
        view->setFocus();
    });
    auto* browse = new QAction(QIcon::fromTheme("centralmods"), tr("Browse"), this);
    browse->setToolTip(tr("Browse modpacks from every source"));
    connect(browse, &QAction::triggered, this, [this] { addInstance({}, {}, "unified"); });
    auto* servers = new QAction(QIcon::fromTheme("server"), tr("Servers"), this);
    servers->setToolTip(tr("Friend servers"));
    connect(servers, &QAction::triggered, this, [this] {
        FriendServersDialog dialog(this);
        dialog.exec();
    });
    ui->mainToolBar->insertAction(ui->actionAddInstance, library);
    ui->mainToolBar->insertAction(ui->actionAddInstance, browse);
    ui->mainToolBar->insertAction(ui->actionAddInstance, servers);
    auto* separator = ui->mainToolBar->insertSeparator(ui->actionAddInstance);
    m_wraithRailActions = { library, browse, servers, separator };

    // follow theme changes live
    connect(APPLICATION->settings()->getSetting("ApplicationTheme").get(), &Setting::SettingChanged, this,
            [this] { applyWraithLayout(WraithTheme::isActive()); }, Qt::QueuedConnection);

    applyWraithLayout(WraithTheme::isActive());
}

void MainWindow::applyWraithLayout(bool enabled)
{
    m_wraithHeader->setVisible(enabled);
    for (auto* action : m_wraithRailActions)
        action->setVisible(enabled);
    if (!enabled)
        m_wraithSearch->clear();

    // Move the main toolbar between the top edge (classic) and the left edge (navigation rail).
    // A different object name keeps the two layouts' saved toolbar positions apart.
    removeToolBar(ui->mainToolBar);
    if (enabled) {
        ui->mainToolBar->setObjectName("wraithNavRail");
        addToolBar(Qt::LeftToolBarArea, ui->mainToolBar);
        ui->mainToolBar->setOrientation(Qt::Vertical);
        ui->mainToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        ui->mainToolBar->setIconSize(QSize(26, 26));
        ui->mainToolBar->setMovable(false);
    } else {
        ui->mainToolBar->setObjectName("mainToolBar");
        addToolBar(Qt::TopToolBarArea, ui->mainToolBar);
        ui->mainToolBar->setOrientation(Qt::Horizontal);
        ui->mainToolBar->setToolButtonStyle(m_classicToolButtonStyle);
        ui->mainToolBar->setIconSize(m_classicIconSize);
        ui->mainToolBar->setMovable(!APPLICATION->settings()->get("ToolbarsLocked").toBool());
    }
    updateMainToolBar();  // respects "menu bar instead of toolbar"
    repolish(ui->mainToolBar);

    // the Launch button becomes a prominent Play button (WideBar makes its own buttons, so look it up by action)
    ui->instanceToolBar->setObjectName(enabled ? "wraithSidePanel" : "instanceToolBar");
    for (auto* button : ui->instanceToolBar->findChildren<QToolButton*>()) {
        if (button->defaultAction() == ui->actionLaunchInstance || button->text() == ui->actionLaunchInstance->text()) {
            button->setObjectName(enabled ? "wraithPlayButton" : QString());
            button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    }
    repolish(ui->instanceToolBar);
    for (auto* child : ui->instanceToolBar->findChildren<QWidget*>())
        repolish(child);

    // tile size depends on the theme
    view->doItemsLayout();
    view->viewport()->update();
}
