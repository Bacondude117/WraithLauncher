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
#include "UnifiedPage.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSplitter>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "Application.h"
#include "UnifiedModel.h"
#include "ui/dialogs/NewInstanceDialog.h"
#include "ui/widgets/PackCardDelegate.h"

using namespace Unified;

UnifiedPage::UnifiedPage(NewInstanceDialog* dialog, QWidget* parent) : QWidget(parent), m_dialog(dialog), m_model(new UnifiedModel(this))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setClearButtonEnabled(true);
    layout->addWidget(m_searchEdit);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    m_view = new QListView(splitter);
    m_view->setModel(m_model);
    m_view->setMouseTracking(true);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setUniformItemSizes(true);
    m_view->setItemDelegate(new PackCardDelegate({ .author = UnifiedModel::AuthorRole,
                                                   .summary = UnifiedModel::SummaryRole,
                                                   .sources = UnifiedModel::SourcesRole,
                                                   .downloads = UnifiedModel::DownloadsRole },
                                                 m_view));

    auto* detailsPane = new QWidget(splitter);
    auto* detailsLayout = new QVBoxLayout(detailsPane);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    m_details = new QTextBrowser(detailsPane);
    m_details->setOpenExternalLinks(true);
    detailsLayout->addWidget(m_details);
    m_installButtons = new QHBoxLayout();
    detailsLayout->addLayout(m_installButtons);

    splitter->addWidget(m_view);
    splitter->addWidget(detailsPane);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    m_searchTimer.setSingleShot(true);
    connect(&m_searchTimer, &QTimer::timeout, this, &UnifiedPage::triggerSearch);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this] { m_searchTimer.start(400); });
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &UnifiedPage::triggerSearch);
    connect(m_model->search(), &UnifiedSearch::resultsChanged, this, &UnifiedPage::updateStatus);
    connect(m_model, &QAbstractItemModel::modelReset, this, &UnifiedPage::updateDetails);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this, &UnifiedPage::updateDetails);

    retranslate();
    updateDetails();
}

QIcon UnifiedPage::icon() const
{
    return APPLICATION->logo();
}

void UnifiedPage::retranslate()
{
    m_searchEdit->setPlaceholderText(tr("Search modpacks on every source at once..."));
    updateStatus();
    updateDetails();
}

void UnifiedPage::openedImpl()
{
    BasePage::openedImpl();
    // Nothing is selected for installation on this page; installs happen on the per-source pages.
    m_dialog->setSuggestedPack();
    if (!m_searchedOnce)
        triggerSearch();
}

void UnifiedPage::setSearchTerm(QString term)
{
    m_searchEdit->setText(term);
    triggerSearch();
}

QString UnifiedPage::getSerachTerm() const
{
    return m_searchEdit->text();
}

QString UnifiedPage::pageIdFor(Source source)
{
    switch (source) {
        case Source::Modrinth:
            return "modrinth";
        case Source::CurseForge:
            return "flame";
        case Source::FTB:
            return "ftb";
        case Source::ATLauncher:
            return "atl";
        case Source::Technic:
            return "technic";
    }
    return {};
}

void UnifiedPage::triggerSearch()
{
    m_searchTimer.stop();
    m_searchedOnce = true;
    m_model->search()->search(m_searchEdit->text());
}

void UnifiedPage::updateStatus()
{
    const auto* search = m_model->search();
    QStringList parts;

    if (search->isRunning()) {
        QStringList pending;
        for (auto source : search->pendingSources())
            pending.append(sourceName(source));
        parts.append(tr("Searching %1...").arg(pending.join(", ")));
    } else if (m_searchedOnce) {
        parts.append(tr("%n modpack(s) found.", "", static_cast<int>(search->packs().size())));
    }

    for (auto it = search->errors().constBegin(); it != search->errors().constEnd(); ++it)
        parts.append(tr("Couldn't reach %1.").arg(sourceName(it.key())));

    if (!UnifiedSearch::enabledSources().contains(Source::CurseForge))
        parts.append(tr("CurseForge is off because no API key is configured (see SETUP_KEYS.md)."));

    m_status->setText(parts.join(' '));
}

void UnifiedPage::updateDetails()
{
    // clear old install buttons
    while (auto* item = m_installButtons->takeAt(0)) {
        delete item->widget();
        delete item;
    }

    const Pack* pack = m_model->pack(m_view->currentIndex());
    if (!pack) {
        m_details->setHtml(tr("<p>Select a modpack to see where it's available.</p>"
                              "<p>Packs found on several sources are merged into one card. "
                              "The per-source pages are still available under <b>Advanced</b>.</p>"));
        return;
    }

    QString html = QString("<h2>%1</h2>").arg(pack->name.toHtmlEscaped());
    if (!pack->author.isEmpty())
        html += QString("<p>%1</p>").arg(tr("by %1").arg(pack->author.toHtmlEscaped()));
    if (!pack->summary.isEmpty())
        html += QString("<p>%1</p>").arg(pack->summary.toHtmlEscaped());
    if (pack->downloads > 0)
        html += QString("<p>%1</p>").arg(PackCardDelegate::formatDownloads(pack->downloads));

    QStringList links;
    for (const auto& hit : pack->hits) {
        const QString name = sourceName(hit.source);
        links.append(hit.websiteUrl.isEmpty() ? name : QString("<a href=\"%1\">%2</a>").arg(hit.websiteUrl.toHtmlEscaped(), name));
    }
    html += QString("<p>%1 %2</p>").arg(tr("Available on:"), links.join(" &middot; "));
    m_details->setHtml(html);

    for (const auto& hit : pack->hits) {
        auto* button = new QPushButton(tr("Install from %1").arg(sourceName(hit.source)));
        button->setToolTip(tr("Open the %1 page with this pack to pick a version and install it.").arg(sourceName(hit.source)));
        const Source source = hit.source;
        connect(button, &QPushButton::clicked, this, [this, source] { installFrom(source); });
        m_installButtons->addWidget(button);
    }
}

void UnifiedPage::installFrom(Source source)
{
    const Pack* pack = m_model->pack(m_view->currentIndex());
    const SourceHit* hit = pack ? pack->hit(source) : nullptr;
    if (!hit || !m_container)
        return;

    const QString pageId = pageIdFor(source);
    // Technic's page resolves "#slug" to exactly one pack; the others filter by name.
    const QString term = source == Source::Technic ? "#" + hit->id : hit->name;
    if (auto* page = dynamic_cast<ModpackProviderBasePage*>(m_container->getPage(pageId)))
        page->setSearchTerm(term);
    m_container->selectPage(pageId);
}
