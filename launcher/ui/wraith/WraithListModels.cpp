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
#include "WraithListModels.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QLocale>
#include <QMessageBox>
#include <QUrl>

#include "Application.h"
#include "DesktopServices.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/World.h"
#include "minecraft/WorldList.h"
#include "minecraft/mod/ResourceFolderModel.h"

#include "WraithInstanceModel.h"

namespace Wraith {

ResourceListModel::ResourceListModel(ResourceFolderModel* model, QObject* parent) : QSortFilterProxyModel(parent), m_model(model)
{
    setSourceModel(model);
    setDynamicSortFilter(true);
    sort(0);
    connect(this, &QAbstractItemModel::rowsInserted, this, &ResourceListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ResourceListModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &ResourceListModel::countChanged);
    connect(this, &QAbstractItemModel::dataChanged, this, &ResourceListModel::countChanged);
}

QHash<int, QByteArray> ResourceListModel::roleNames() const
{
    return { { NameRole, "name" },         { VersionRole, "version" },     { FileNameRole, "fileName" }, { EnabledRole, "isEnabled" },
             { ProviderRole, "provider" }, { HasIssuesRole, "hasIssues" }, { SizeRole, "size" }, { PinnedRole, "pinned" },
             { HasMetadataRole, "hasMetadata" } };
}

void ResourceListModel::setPinned(int row, bool pinned)
{
    // Prism's "lock updates": update checks skip a pinned version (stored in the mod's metadata)
    const QModelIndex source = mapToSource(index(row, 0));
    if (source.isValid())
        m_model->setUpdateLock({ source }, pinned ? EnableAction::ENABLE : EnableAction::DISABLE);
}

QVariant ResourceListModel::data(const QModelIndex& index, int role) const
{
    if (role < NameRole)
        return QSortFilterProxyModel::data(index, role);
    const int row = mapToSource(index).row();
    if (row < 0 || row >= m_model->size())
        return {};
    const auto& resource = m_model->at(row);
    switch (role) {
        case NameRole:
            return resource.name();
        case VersionRole:
            return resource.version();
        case FileNameRole:
            return resource.fileinfo().fileName();
        case EnabledRole:
            return resource.enabled();
        case ProviderRole:
            return resource.provider();
        case HasIssuesRole:
            return resource.hasIssues();
        case SizeRole:
            return resource.sizeStr();
        case PinnedRole:
            return resource.lockUpdate();
        case HasMetadataRole:
            return resource.metadata() != nullptr;
        default:
            return {};
    }
}

void ResourceListModel::setFilterText(const QString& text)
{
    if (m_filterText == text)
        return;
    m_filterText = text;
    invalidateFilter();
    emit filterTextChanged();
    emit countChanged();
}

int ResourceListModel::enabledCount() const
{
    int enabled = 0;
    for (int i = 0; i < m_model->size(); i++)
        enabled += m_model->at(i).enabled() ? 1 : 0;
    return enabled;
}

bool ResourceListModel::filterAcceptsRow(int sourceRow, const QModelIndex&) const
{
    if (m_filterText.isEmpty())
        return true;
    const auto& resource = m_model->at(sourceRow);
    return resource.name().contains(m_filterText, Qt::CaseInsensitive) ||
           resource.fileinfo().fileName().contains(m_filterText, Qt::CaseInsensitive);
}

bool ResourceListModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    return m_model->at(left.row()).name().compare(m_model->at(right.row()).name(), Qt::CaseInsensitive) < 0;
}

void ResourceListModel::refresh()
{
    m_model->update();
}

void ResourceListModel::setEnabled(int row, bool enabled)
{
    const auto source = mapToSource(index(row, 0));
    if (source.isValid())
        m_model->setResourceEnabled({ source }, enabled ? EnableAction::ENABLE : EnableAction::DISABLE);
}

void ResourceListModel::remove(int row)
{
    const auto source = mapToSource(index(row, 0));
    if (!source.isValid())
        return;
    const auto answer = QMessageBox::question(QApplication::activeWindow(), tr("Remove"),
                                              tr("Remove \"%1\" from this instance? The file will be deleted.").arg(m_model->at(source.row()).name()));
    if (answer == QMessageBox::Yes)
        m_model->deleteResources({ source });
}

void ResourceListModel::openFolder()
{
    DesktopServices::openPath(m_model->dir().absolutePath(), true);
}

WorldListModel::WorldListModel(MinecraftInstance* instance, QObject* parent)
    : QSortFilterProxyModel(parent), m_instance(instance), m_worlds(instance->worldList())
{
    setSourceModel(m_worlds);
    connect(this, &QAbstractItemModel::rowsInserted, this, &WorldListModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &WorldListModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &WorldListModel::countChanged);
}

QHash<int, QByteArray> WorldListModel::roleNames() const
{
    return { { NameRole, "name" },         { FolderRole, "folder" }, { GameModeRole, "gameMode" }, { LastPlayedRole, "lastPlayedText" },
             { SeedRole, "seed" },         { SizeRole, "size" },     { IconRole, "icon" } };
}

QVariant WorldListModel::data(const QModelIndex& index, int role) const
{
    if (role < NameRole)
        return QSortFilterProxyModel::data(index, role);
    const auto source = mapToSource(index);
    switch (role) {
        case NameRole:
            return m_worlds->data(source, WorldList::NameRole);
        case FolderRole:
            return m_worlds->data(source, WorldList::FolderRole);
        case GameModeRole:
            return m_worlds->data(m_worlds->index(source.row(), WorldList::GameModeColumn), Qt::DisplayRole);
        case LastPlayedRole: {
            const auto last = m_worlds->data(source, WorldList::LastPlayedRole).toDateTime();
            return last.isValid() ? relativeTime(last.toMSecsSinceEpoch()) : tr("Never played");
        }
        case SeedRole:
            return QString::number(m_worlds->data(source, WorldList::SeedRole).toLongLong());
        case SizeRole:
            return QLocale().formattedDataSize(m_worlds->data(source, WorldList::SizeRole).toLongLong());
        case IconRole: {
            const QString icon = m_worlds->data(source, WorldList::IconFileRole).toString();
            return icon.isEmpty() ? QString() : QUrl::fromLocalFile(icon).toString();
        }
        default:
            return {};
    }
}

void WorldListModel::refresh()
{
    m_worlds->update();
}

void WorldListModel::play(int row)
{
    const auto source = mapToSource(index(row, 0));
    auto* world = static_cast<World*>(m_worlds->data(source, WorldList::ObjectRole).value<void*>());
    if (world)
        APPLICATION->launch(m_instance, LaunchMode::Normal, std::make_shared<MinecraftTarget>(MinecraftTarget::parse(world->folderName(), true)));
}

void WorldListModel::openFolder(int row)
{
    DesktopServices::openPath(m_worlds->data(mapToSource(index(row, 0)), WorldList::FolderRole).toString(), true);
}

void WorldListModel::copySeed(int row)
{
    QApplication::clipboard()->setText(data(index(row, 0), SeedRole).toString());
}

}  // namespace Wraith
