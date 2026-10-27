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
#include "AdvancedSourcesPage.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AdvancedSourcesPage::AdvancedSourcesPage(QStringList pageIds, QWidget* parent) : QWidget(parent), m_pageIds(std::move(pageIds))
{
    auto* layout = new QVBoxLayout(this);
    auto* intro = new QLabel(tr("<p>The pages listed below this one search a single modpack source each. They offer each "
                                "source's own filters and sorting, plus installers for sources that need extra steps.</p>"
                                "<p>For everyday searching, <b>Browse Modpacks</b> searches all of them at once.</p>"),
                             this);
    intro->setWordWrap(true);
    layout->addWidget(intro);
    m_buttons = new QVBoxLayout();
    layout->addLayout(m_buttons);
    layout->addStretch(1);
}

void AdvancedSourcesPage::openedImpl()
{
    // (Re)build the shortcut buttons; which pages exist depends on configuration (e.g. CurseForge key).
    while (auto* item = m_buttons->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (!m_container)
        return;
    for (const auto& pageId : m_pageIds) {
        BasePage* page = m_container->getPage(pageId);
        if (!page || !page->shouldDisplay())
            continue;
        auto* button = new QPushButton(page->icon(), page->displayName(), this);
        connect(button, &QPushButton::clicked, this, [this, pageId] { m_container->selectPage(pageId); });
        m_buttons->addWidget(button);
    }
}
