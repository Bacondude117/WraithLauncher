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

#include <QSortFilterProxyModel>
#include <QVariantMap>

class MinecraftInstance;

namespace Wraith {

/** Loader name ("Fabric", "Vanilla", ...) and Minecraft version, read from mmc-pack.json (cached by file time). */
struct PackInfo {
    QString loader;
    QString minecraft;
};
PackInfo packInfo(MinecraftInstance* instance);

QString relativeTime(qint64 msSinceEpoch);
QString playTimeText(qint64 seconds);
QString iconUrl(const QString& iconKey);

/** Everything the QML views show about one instance. */
QVariantMap instanceInfo(MinecraftInstance* instance);

/**
 * The instance list as QML sees it: named roles, a text/loader/group filter and the three sort orders.
 */
class InstanceModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filtersChanged)
    Q_PROPERTY(QString loaderFilter READ loaderFilter WRITE setLoaderFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString groupFilter READ groupFilter WRITE setGroupFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

   public:
    enum Roles {
        IdRole = Qt::UserRole + 100,
        NameRole,
        IconRole,
        LoaderRole,
        MinecraftRole,
        GroupNameRole,
        LastPlayedRole,
        LastPlayedTextRole,
        PlayTimeTextRole,
        RunningRole,
        CanLaunchRole,
    };

    explicit InstanceModel(QObject* parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString& text);
    QString loaderFilter() const { return m_loaderFilter; }
    void setLoaderFilter(const QString& loader);
    QString groupFilter() const { return m_groupFilter; }
    void setGroupFilter(const QString& group);
    QString sortMode() const { return m_sortMode; }
    void setSortMode(const QString& mode);
    int count() const { return rowCount(); }

    /** Bumped when icons change so image URLs are re-requested. */
    void iconsChanged();

   signals:
    void filtersChanged();
    void countChanged();

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

   private:
    MinecraftInstance* instanceAt(const QModelIndex& sourceIndex) const;
    void watchRunningState();

    QString m_filterText;
    QString m_loaderFilter;
    QString m_groupFilter;
    QString m_sortMode = "name";
};

}  // namespace Wraith
