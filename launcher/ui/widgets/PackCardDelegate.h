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

#include <QStyledItemDelegate>

/**
 * Card-style delegate for modpack lists (Modrinth/CurseForge-like): icon, title, author, two-line summary,
 * source chips and download count. Colors come from the active palette so it works with every theme.
 *
 * Reads Qt::DisplayRole, Qt::DecorationRole and the optional roles passed to the constructor.
 */
class PackCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
   public:
    struct Roles {
        int author = -1;
        int summary = -1;
        int sources = -1;    ///< QStringList of source names shown as chips
        int downloads = -1;  ///< qint64; hidden when <= 0
    };

    PackCardDelegate(Roles roles, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    /** Brand-ish chip color for a source name; neutral accent for unknown names. */
    static QColor sourceColor(const QString& source, const QPalette& palette);
    static QString formatDownloads(qint64 downloads);

   private:
    Roles m_roles;
};
