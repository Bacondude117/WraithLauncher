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

#include <QHash>
#include <QWidget>

#include "WraithShell.h"

class BasePage;
class MinecraftInstance;
class QQuickWidget;
class QStackedWidget;

namespace Wraith {

class PageHost;

/**
 * Full-page view of one instance: a header (cover, icon, name, badges, Play) with tabs. Content and Worlds have
 * simple Wraith lists with the original pages one click away; the other tabs host the original pages directly.
 */
class InstancePage : public QWidget, public ShellPage {
    Q_OBJECT
   public:
    InstancePage(Shell* shell, MinecraftInstance* instance, QWidget* parent);
    ~InstancePage() override;

    /** A tab name ("content", "worlds", ...) or a Prism page id ("console", "mods", "version", ...). */
    void selectTab(const QString& tabOrPage);
    void scriptCall(const QString& function, const QString& argument);

    void pageShown() override;
    bool pageLeaving() override;

   private slots:
    void onTabSelected(const QString& tab);
    void onContentAction(const QString& action, const QString& pageId);

   private:
    QWidget* buildContentTab(const QList<BasePage*>& pages);
    QWidget* buildWorldsTab(BasePage* page);
    QWidget* buildSnapshotsTab();
    QWidget* buildPerformanceCard(QWidget* parent);
    QWidget* buildLogsTab(const QList<BasePage*>& pages);
    QWidget* buildSharedFoldersCard(QWidget* parent);
    /** A tab that is one Qt Quick view with a C++ model (set as its "model" property). */
    QWidget* buildQuickTab(const QString& key, const QString& qml, QObject* model);
    QWidget* buildSettingsTab(const QList<BasePage*>& pages);
    PageHost* hostTab(const QString& tab, const QList<BasePage*>& pages);
    void showAdvanced(QStackedWidget* stack, PageHost* host, const QString& pageId);
    void showTab(const QString& tab);
    void refreshSettingsPage();

    Shell* m_shell;
    MinecraftInstance* m_instance;
    QQuickWidget* m_header = nullptr;
    QStackedWidget* m_tabs = nullptr;
    QHash<QString, QWidget*> m_tabWidgets;
    QHash<QString, PageHost*> m_hosts;  // by tab
    QHash<QString, QStackedWidget*> m_simpleStacks;  // tabs with a simple view and an advanced view
    QString m_tab;
};

}  // namespace Wraith
