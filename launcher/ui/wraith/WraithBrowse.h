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
#include <QSet>
#include <QStringList>

#include "modplatform/ftb/FTBPackManifest.h"
#include "modplatform/unified/UnifiedSearch.h"
#include "net/NetJob.h"

namespace Wraith {

class Shell;

/**
 * The Browse page's model: the unified search (every modpack source at once) with source filters and sorting,
 * plus installing. Every source gets the same treatment: a version list and a direct Install that runs that
 * source's own Prism install task (import task with hash check, FTB, ATLauncher, Technic Solder/zip).
 * The original per-source pages stay reachable from "Open in <source> page".
 */
class BrowseModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY statusChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QVariantList sources READ sources NOTIFY statusChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)
    Q_PROPERTY(int count READ count NOTIFY filtersChanged)
    Q_PROPERTY(bool curseForgeMissing READ curseForgeMissing CONSTANT)

   public:
    enum Roles { NameRole = Qt::UserRole + 400, AuthorRole, SummaryRole, IconRole, DownloadsRole, SourcesRole };

    explicit BrowseModel(Shell* shell);

    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : static_cast<int>(m_visible.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool running() const { return m_search->isRunning(); }
    QString status() const;
    /** [{ name, count, enabled, available }] for the filter panel. */
    QVariantList sources() const;
    QString sortMode() const { return m_sortMode; }
    void setSortMode(const QString& mode);
    int count() const { return rowCount(); }
    bool curseForgeMissing() const;

    Q_INVOKABLE void search(const QString& query);
    Q_INVOKABLE void setSourceEnabled(const QString& source, bool enabled);
    /** Details for the side panel: name, author, summary, icon, downloads, hits [{ source, url, id }]. */
    Q_INVOKABLE QVariantMap details(int row) const;
    /** Fetch the versions of `source`'s entry for this pack; answers with versionsLoaded/versionsFailed. */
    Q_INVOKABLE void loadVersions(int row, const QString& source);
    Q_INVOKABLE void install(int row, const QString& source, const QString& versionId);
    /** Open the original per-source page with the pack searched (for source-specific extras). */
    Q_INVOKABLE void installWith(int row, const QString& source);
    Q_INVOKABLE void openAdvanced();
    /** Whether Browse should list popular packs when opened (Settings: WraithBrowseLoadPopular). */
    Q_INVOKABLE bool loadPopularOnOpen() const;

   signals:
    void statusChanged();
    void filtersChanged();
    void versionsLoaded(int row, const QString& source, const QVariantList& versions);
    void versionsFailed(int row, const QString& source, const QString& error);

   private:
    void rebuild();

    Shell* m_shell;
    Unified::UnifiedSearch* m_search;
    QList<Unified::Pack> m_visible;
    QSet<Unified::Source> m_disabled;
    QString m_sortMode = "relevance";
    NetJob::Ptr m_versionsJob;
    NetJob::Ptr m_iconJob;
    QHash<QString, QVariantMap> m_versions;  // "<source>|<version id>" -> { url, number, hash, ... }
    QHash<QString, FTB::Modpack> m_ftbPacks;  // FTB pack id -> details (the install task needs them)
};

}  // namespace Wraith
