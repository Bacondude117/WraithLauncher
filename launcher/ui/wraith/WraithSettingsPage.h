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
#include <functional>

#include "WraithShell.h"

class BasePage;
class QButtonGroup;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

namespace Wraith {

class PageHost;

/**
 * The Settings page: a category list; each category has simple controls first and the original global settings
 * pages under "Advanced" (or shown directly, for Accounts and Network). The original pages are created fresh each
 * time Settings opens and applied when it is left, like Prism's settings dialog on OK.
 */
class SettingsPage : public QWidget, public ShellPage {
    Q_OBJECT
   public:
    SettingsPage(Shell* shell, QWidget* parent);

    /** A category key ("general", "java", ...) or a Prism settings page id ("accounts", "java-settings", ...). */
    void selectCategory(const QString& category);

    void pageShown() override;
    bool pageLeaving() override;

   private:
    struct Category {
        QString key;
        QString label;
        QString icon;
        QStringList pageIds;
        bool pagesFirst = false;  // no simple view: the pages are the category
    };

    void build();
    void clear();
    QWidget* buildSimple(const Category& category, PageHost* host, QStackedWidget* stack);
    void showCategory(const QString& key);
    void showAdvanced(const QString& key, const QString& pageId);
    /** Simple controls write settings directly; an advanced page already open is saved first and then reloaded. */
    void write(const QString& categoryKey, const std::function<void()>& change);

    Shell* m_shell;
    QList<Category> m_categories;
    QWidget* m_nav;
    QVBoxLayout* m_navLayout;
    QButtonGroup* m_navGroup;
    QStackedWidget* m_stack;
    QHash<QString, QPushButton*> m_navButtons;
    QHash<QString, QStackedWidget*> m_categoryStacks;
    QHash<QString, PageHost*> m_hosts;
    QString m_current;
    bool m_built = false;
};

}  // namespace Wraith
