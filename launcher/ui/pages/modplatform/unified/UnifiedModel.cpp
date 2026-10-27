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
#include "UnifiedModel.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>

#include "Application.h"

namespace Unified {

UnifiedModel::UnifiedModel(QObject* parent) : QAbstractListModel(parent)
{
    connect(&m_search, &UnifiedSearch::resultsChanged, this, &UnifiedModel::resultsChanged);
}

int UnifiedModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_packs.size());
}

const Pack* UnifiedModel::pack(const QModelIndex& index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_packs.size())
        return nullptr;
    return &m_packs[index.row()];
}

QVariant UnifiedModel::data(const QModelIndex& index, int role) const
{
    const Pack* p = pack(index);
    if (!p)
        return {};

    switch (role) {
        case Qt::DisplayRole:
            return p->name;
        case Qt::ToolTipRole:
            return p->summary;
        case Qt::DecorationRole: {
            if (p->iconUrl.isEmpty())
                return APPLICATION->logo();
            if (auto it = m_icons.constFind(p->iconUrl); it != m_icons.constEnd())
                return *it;
            requestIcon(p->iconUrl);
            return APPLICATION->logo();
        }
        case AuthorRole:
            return p->author;
        case SummaryRole:
            return p->summary;
        case SourcesRole: {
            QStringList sources;
            for (const auto& hit : p->hits)
                sources.append(sourceName(hit.source));
            return sources;
        }
        case DownloadsRole:
            return p->downloads;
        default:
            return {};
    }
}

void UnifiedModel::resultsChanged()
{
    // Results are re-ranked as each source answers, so a full reset is the honest signal here.
    beginResetModel();
    m_packs = m_search.packs();
    endResetModel();
}

void UnifiedModel::requestIcon(const QString& url) const
{
    if (m_requestedIcons.contains(url))
        return;
    m_requestedIcons.insert(url);

    QNetworkRequest request{ QUrl(url) };
    request.setHeader(QNetworkRequest::UserAgentHeader, APPLICATION->getUserAgent());
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto* reply = APPLICATION->network()->get(request);
    auto* self = const_cast<UnifiedModel*>(this);
    connect(reply, &QNetworkReply::finished, self, [self, reply, url] {
        reply->deleteLater();
        QPixmap pixmap;
        if (reply->error() != QNetworkReply::NoError || !pixmap.loadFromData(reply->readAll()))
            return;
        self->m_icons.insert(url, QIcon(pixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        for (int row = 0; row < self->m_packs.size(); row++) {
            if (self->m_packs[row].iconUrl == url)
                emit self->dataChanged(self->index(row), self->index(row), { Qt::DecorationRole });
        }
    });
}

}  // namespace Unified
