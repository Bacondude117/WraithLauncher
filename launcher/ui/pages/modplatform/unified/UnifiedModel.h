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

#include <QAbstractListModel>
#include <QHash>
#include <QIcon>
#include <QSet>

#include "modplatform/unified/UnifiedSearch.h"

namespace Unified {

/** List model over UnifiedSearch results; also lazily downloads pack icons. */
class UnifiedModel : public QAbstractListModel {
    Q_OBJECT
   public:
    enum Roles { AuthorRole = Qt::UserRole + 1, SummaryRole, SourcesRole, DownloadsRole };

    explicit UnifiedModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;

    UnifiedSearch* search() { return &m_search; }
    const Pack* pack(const QModelIndex& index) const;

   private:
    void resultsChanged();
    void requestIcon(const QString& url) const;

   private:
    UnifiedSearch m_search;
    QList<Pack> m_packs;
    mutable QHash<QString, QIcon> m_icons;
    mutable QSet<QString> m_requestedIcons;
};

}  // namespace Unified
