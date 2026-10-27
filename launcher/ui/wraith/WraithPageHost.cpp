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
#include "WraithPageHost.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "WraithStyle.h"

namespace Wraith {

PageHost::PageHost(QWidget* parent) : QWidget(parent), m_group(new QButtonGroup(this))
{
    setObjectName("wraithHost");
    setAttribute(Qt::WA_StyledBackground);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    m_header = new QWidget(this);
    m_pills = new QHBoxLayout(m_header);
    m_pills->setContentsMargins(0, 0, 0, 0);
    m_pills->setSpacing(8);
    m_pills->addStretch(1);
    layout->addWidget(m_header);
    m_header->hide();

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);
    m_group->setExclusive(true);
}

void PageHost::addPage(BasePage* page, const QString& label)
{
    auto* widget = dynamic_cast<QWidget*>(page);
    if (!widget)
        return;
    widget->setParent(m_stack);
    modernizePage(widget);
    m_stack->addWidget(widget);
    page->setParentContainer(this);
    page->updateExtraInfo = [](const QString&, const QString&) {};
    m_pages.append(page);

    QString text = label.isEmpty() ? page->displayName() : label;
    auto* button = new QPushButton(text.replace('&', "&&"), m_header);  // a literal "&", not a mnemonic
    button->setObjectName("wraithPill");
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    m_group->addButton(button);
    m_pills->insertWidget(m_pills->count() - 1, button);
    m_buttons.insert(page, button);
    connect(button, &QPushButton::clicked, this, [this, page] { show(page); });

    refreshContainer();
    if (!m_last && page->shouldDisplay()) {
        // the first page is shown once the host is activated
        m_last = page;
        m_stack->setCurrentWidget(widget);
        button->setChecked(true);
    }
}

void PageHost::addHeaderWidget(QWidget* widget)
{
    widget->setParent(m_header);
    m_pills->addWidget(widget);
    m_header->show();
}

void PageHost::show(BasePage* page)
{
    if (!page)
        return;
    if (m_current == page) {
        m_buttons.value(page)->setChecked(true);
        return;
    }
    if (m_current)
        m_current->closed();
    m_current = page;
    m_stack->setCurrentWidget(dynamic_cast<QWidget*>(page));
    m_buttons.value(page)->setChecked(true);
    m_opened.insert(page);
    page->opened();
    emit pageChanged(page->id());
}

void PageHost::replacePage(BasePage* old, BasePage* fresh)
{
    const int index = m_pages.indexOf(old);
    auto* oldWidget = dynamic_cast<QWidget*>(old);
    auto* freshWidget = dynamic_cast<QWidget*>(fresh);
    if (index < 0 || !oldWidget || !freshWidget)
        return;
    const bool wasCurrent = m_current == old;
    if (wasCurrent)
        old->closed();
    freshWidget->setParent(m_stack);
    modernizePage(freshWidget);
    m_stack->insertWidget(m_stack->indexOf(oldWidget), freshWidget);
    fresh->setParentContainer(this);
    fresh->updateExtraInfo = [](const QString&, const QString&) {};
    m_pages[index] = fresh;
    auto* button = m_buttons.take(old);
    m_buttons.insert(fresh, button);
    disconnect(button, &QPushButton::clicked, this, nullptr);
    connect(button, &QPushButton::clicked, this, [this, fresh] { show(fresh); });
    m_opened.remove(old);
    if (m_last == old)
        m_last = fresh;
    m_stack->setCurrentWidget(freshWidget);
    if (wasCurrent) {
        m_current = fresh;
        m_opened.insert(fresh);
        fresh->opened();
    }
    oldWidget->deleteLater();
}

bool PageHost::selectPage(QString pageId)
{
    if (auto* page = getPage(pageId); page && page->shouldDisplay()) {
        show(page);
        return true;
    }
    return false;
}

BasePage* PageHost::getPage(QString pageId)
{
    for (auto* page : m_pages) {
        if (page->id() == pageId)
            return page;
    }
    return nullptr;
}

void PageHost::refreshContainer()
{
    int visible = 0;
    for (auto* page : m_pages) {
        const bool display = page->shouldDisplay();
        m_buttons.value(page)->setVisible(display);
        visible += display ? 1 : 0;
    }
    // a single page needs no switcher, unless a header widget was added
    const bool hasExtras = m_pills->count() > m_pages.size() + 1;
    m_header->setVisible(visible > 1 || hasExtras);
    if (m_current && !m_current->shouldDisplay()) {
        for (auto* page : m_pages) {
            if (page->shouldDisplay()) {
                show(page);
                break;
            }
        }
    }
}

bool PageHost::applyAll()
{
    // pages that were never shown hold no edits; applying them could overwrite changes made elsewhere since
    for (auto* page : m_pages) {
        if (m_opened.contains(page) && !page->apply())
            return false;
    }
    return true;
}

void PageHost::deactivate()
{
    if (m_current) {
        m_last = m_current;
        m_current->closed();
    }
    m_current = nullptr;
}

void PageHost::activate()
{
    if (m_current)
        return;
    if (m_last && m_last->shouldDisplay()) {
        show(m_last);
        return;
    }
    for (auto* page : m_pages) {
        if (page->shouldDisplay()) {
            show(page);
            return;
        }
    }
}

}  // namespace Wraith
