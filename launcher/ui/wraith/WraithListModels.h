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

class MinecraftInstance;
class ResourceFolderModel;
class WorldList;

namespace Wraith {

/** QML view of an instance's mods / resource packs / shaders ...: named roles, search, enable and remove. */
class ResourceListModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int enabledCount READ enabledCount NOTIFY countChanged)

   public:
    enum Roles { NameRole = Qt::UserRole + 200, VersionRole, FileNameRole, EnabledRole, ProviderRole, HasIssuesRole, SizeRole };

    ResourceListModel(ResourceFolderModel* model, QObject* parent);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString& text);
    int count() const { return rowCount(); }
    int enabledCount() const;
    ResourceFolderModel* resources() const { return m_model; }

    /** Prism's models load lazily (when their page opens); this loads the folder now. */
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setEnabled(int row, bool enabled);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void openFolder();

   signals:
    void filterTextChanged();
    void countChanged();

   protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;

   private:
    ResourceFolderModel* m_model;
    QString m_filterText;
};

/** QML view of an instance's worlds. */
class WorldListModel : public QSortFilterProxyModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

   public:
    enum Roles { NameRole = Qt::UserRole + 300, FolderRole, GameModeRole, LastPlayedRole, SeedRole, SizeRole, IconRole };

    WorldListModel(MinecraftInstance* instance, QObject* parent);

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex& index, int role) const override;
    int count() const { return rowCount(); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void play(int row);
    Q_INVOKABLE void openFolder(int row);
    Q_INVOKABLE void copySeed(int row);

   signals:
    void countChanged();

   private:
    MinecraftInstance* m_instance;
    WorldList* m_worlds;
};

}  // namespace Wraith
