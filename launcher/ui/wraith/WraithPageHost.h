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
#include <QSet>
#include <QWidget>

#include "ui/pages/BasePage.h"
#include "ui/pages/BasePageContainer.h"

class QButtonGroup;
class QHBoxLayout;
class QPushButton;
class QStackedWidget;

namespace Wraith {

/**
 * Shows original Prism pages (BasePage) inside the shell: a row of pills to switch between them when there are
 * several, and the page itself, restyled. It is their BasePageContainer, like PageContainer is in Prism's dialogs.
 */
class PageHost : public QWidget, public BasePageContainer {
    Q_OBJECT
   public:
    explicit PageHost(QWidget* parent = nullptr);

    /** Takes ownership of the page widget. */
    void addPage(BasePage* page, const QString& label = QString());
    /** Extra widget at the right end of the pill row (e.g. "Back to simple view"). */
    void addHeaderWidget(QWidget* widget);
    /** Swaps in a freshly constructed page (so it reloads its settings); the old one is deleted. */
    void replacePage(BasePage* old, BasePage* fresh);
    /** Whether the page has been shown since it was added (only those can hold unsaved edits). */
    bool wasOpened(BasePage* page) const { return m_opened.contains(page); }
    QList<BasePage*> pages() const { return m_pages; }
    bool isEmpty() const { return m_pages.isEmpty(); }

    bool applyAll();
    /** Pages are told when they become visible or hidden, as in Prism's page dialogs. */
    void activate();
    void deactivate();

    // BasePageContainer
    bool selectPage(QString pageId) override;
    BasePage* selectedPage() const override { return m_current; }
    BasePage* getPage(QString pageId) override;
    void refreshContainer() override;
    bool requestClose() override { return true; }

   signals:
    void pageChanged(const QString& pageId);

   private:
    void show(BasePage* page);

    QWidget* m_header;
    QHBoxLayout* m_pills;
    QButtonGroup* m_group;
    QStackedWidget* m_stack;
    QList<BasePage*> m_pages;
    QHash<BasePage*, QPushButton*> m_buttons;
    BasePage* m_current = nullptr;
    BasePage* m_last = nullptr;
    QSet<BasePage*> m_opened;
};

}  // namespace Wraith
