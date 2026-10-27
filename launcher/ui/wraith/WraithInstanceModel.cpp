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
#include "WraithInstanceModel.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"

#include <QCollator>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "MMCTime.h"
#include "minecraft/MinecraftInstance.h"

namespace Wraith {

namespace {
int s_iconRevision = 0;
}

PackInfo packInfo(MinecraftInstance* instance)
{
    if (!instance)
        return {};
    struct Cached {
        QDateTime modified;
        PackInfo info;
    };
    static QHash<QString, Cached> s_cache;
    const QFileInfo packFile(FS::PathCombine(instance->instanceRoot(), "mmc-pack.json"));
    auto it = s_cache.find(instance->id());
    if (it != s_cache.end() && it->modified == packFile.lastModified())
        return it->info;

    QHash<QString, QString> versions;
    QFile file(packFile.filePath());
    if (file.open(QIODevice::ReadOnly)) {
        for (const auto& component : QJsonDocument::fromJson(file.readAll()).object().value("components").toArray()) {
            const auto obj = component.toObject();
            versions.insert(obj.value("uid").toString(), obj.value("version").toString(obj.value("cachedVersion").toString()));
        }
    }
    PackInfo info{ "Vanilla", versions.value("net.minecraft") };
    if (versions.contains("net.fabricmc.fabric-loader"))
        info.loader = "Fabric";
    else if (versions.contains("org.quiltmc.quilt-loader"))
        info.loader = "Quilt";
    else if (versions.contains("net.neoforged"))
        info.loader = "NeoForge";
    else if (versions.contains("net.minecraftforge"))
        info.loader = "Forge";
    else if (versions.contains("com.mumfrey.liteloader"))
        info.loader = "LiteLoader";
    s_cache.insert(instance->id(), { packFile.lastModified(), info });
    return info;
}

QString relativeTime(qint64 msSinceEpoch)
{
    if (msSinceEpoch <= 0)
        return QObject::tr("Never played");
    const qint64 seconds = (QDateTime::currentMSecsSinceEpoch() - msSinceEpoch) / 1000;
    if (seconds < 60)
        return QObject::tr("Just now");
    // explicit singular/plural: there are no translations yet to resolve %n forms
    auto ago = [](qint64 n, const QString& one, const QString& many) { return n == 1 ? one : many.arg(n); };
    if (seconds < 3600)
        return ago(seconds / 60, QObject::tr("1 minute ago"), QObject::tr("%1 minutes ago"));
    if (seconds < 86400)
        return ago(seconds / 3600, QObject::tr("1 hour ago"), QObject::tr("%1 hours ago"));
    if (seconds < 86400 * 2)
        return QObject::tr("Yesterday");
    if (seconds < 86400 * 30)
        return ago(seconds / 86400, QObject::tr("1 day ago"), QObject::tr("%1 days ago"));
    return QDateTime::fromMSecsSinceEpoch(msSinceEpoch).date().toString(QLocale().dateFormat(QLocale::ShortFormat));
}

QString playTimeText(qint64 seconds)
{
    if (seconds <= 0)
        return QObject::tr("No playtime");
    return Time::prettifyDuration(seconds, APPLICATION->settings()->get("ShowGameTimeWithoutDays").toBool());
}

QString iconUrl(const QString& iconKey)
{
    return QString("image://wraith/icon/%1?r=%2").arg(QString::fromUtf8(QUrl::toPercentEncoding(iconKey))).arg(s_iconRevision);
}

QStringList instanceTags(BaseInstance* instance)
{
    // kept in the instance's own instance.cfg, so tags travel with it (Prism ignores the key)
    return instance->settings()->getOrRegisterSetting("WraithTags", QString())->get().toString().split(',', Qt::SkipEmptyParts);
}

void setInstanceTags(BaseInstance* instance, const QStringList& tags)
{
    QStringList clean;
    for (const auto& tag : tags)
        if (!tag.trimmed().isEmpty() && !clean.contains(tag.trimmed(), Qt::CaseInsensitive))
            clean.append(tag.trimmed());
    instance->settings()->getOrRegisterSetting("WraithTags", QString());
    instance->settings()->set("WraithTags", clean.join(','));
}

QVariantMap instanceInfo(MinecraftInstance* instance)
{
    if (!instance)
        return {};
    const auto pack = packInfo(instance);
    return {
        { "id", instance->id() },
        { "name", instance->name() },
        { "icon", iconUrl(instance->iconKey()) },
        { "loader", pack.loader },
        { "minecraft", pack.minecraft },
        { "group", APPLICATION->instances()->getInstanceGroup(instance->id()) },
        { "lastPlayed", instance->lastLaunch() },
        { "lastPlayedText", relativeTime(instance->lastLaunch()) },
        { "playTimeText", playTimeText(instance->totalTimePlayed()) },
        { "running", instance->isRunning() },
        { "canLaunch", instance->canLaunch() },
        { "managed", instance->isManagedPack() },
        { "managedType", instance->getManagedPackType() },
    };
}

InstanceModel::InstanceModel(QObject* parent) : QSortFilterProxyModel(parent)
{
    setSourceModel(APPLICATION->instances());
    setDynamicSortFilter(true);
    m_sortMode = APPLICATION->settings()->get("InstSortMode").toString();
    sort(0);
    connect(this, &QAbstractItemModel::rowsInserted, this, &InstanceModel::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &InstanceModel::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &InstanceModel::countChanged);
    connect(this, &InstanceModel::filtersChanged, this, &InstanceModel::countChanged);
    connect(APPLICATION->instances(), &QAbstractItemModel::modelReset, this, &InstanceModel::watchRunningState);
    connect(APPLICATION->instances(), &QAbstractItemModel::rowsInserted, this, &InstanceModel::watchRunningState);
    watchRunningState();
}

void InstanceModel::watchRunningState()
{
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++) {
        auto* instance = list->at(i);
        if (instance->property("wraithWatched").toBool())
            continue;
        instance->setProperty("wraithWatched", true);
        connect(instance, &BaseInstance::runningStatusChanged, this, [this, instance] {
            const int row = APPLICATION->instances()->getInstIndex(instance);
            if (row < 0)
                return;
            const auto index = mapFromSource(APPLICATION->instances()->index(row));
            if (index.isValid())
                emit dataChanged(index, index);
        });
    }
}

QHash<int, QByteArray> InstanceModel::roleNames() const
{
    return {
        { IdRole, "instanceId" },          { NameRole, "name" },
        { IconRole, "icon" },              { LoaderRole, "loader" },
        { MinecraftRole, "minecraft" },    { GroupNameRole, "group" },
        { LastPlayedRole, "lastPlayed" },  { LastPlayedTextRole, "lastPlayedText" },
        { PlayTimeTextRole, "playTimeText" }, { RunningRole, "running" },
        { CanLaunchRole, "canLaunch" },    { TagsRole, "tags" },
        { UpdateRole, "updateAvailable" },
    };
}

MinecraftInstance* InstanceModel::instanceAt(const QModelIndex& sourceIndex) const
{
    return APPLICATION->instances()->at(sourceIndex.row());
}

QVariant InstanceModel::data(const QModelIndex& index, int role) const
{
    if (role < IdRole)
        return QSortFilterProxyModel::data(index, role);
    auto* instance = instanceAt(mapToSource(index));
    if (!instance)
        return {};
    switch (role) {
        case IdRole:
            return instance->id();
        case NameRole:
            return instance->name();
        case IconRole:
            return iconUrl(instance->iconKey());
        case LoaderRole:
            return packInfo(instance).loader;
        case MinecraftRole:
            return packInfo(instance).minecraft;
        case GroupNameRole:
            return APPLICATION->instances()->getInstanceGroup(instance->id());
        case LastPlayedRole:
            return instance->lastLaunch();
        case LastPlayedTextRole:
            return relativeTime(instance->lastLaunch());
        case PlayTimeTextRole:
            return playTimeText(instance->totalTimePlayed());
        case RunningRole:
            return instance->isRunning();
        case CanLaunchRole:
            return instance->canLaunch();
        case TagsRole:
            return instanceTags(instance);
        case UpdateRole:
            return instance->property("wraithUpdateAvailable").toString();
        default:
            return {};
    }
}

void InstanceModel::setFilterText(const QString& text)
{
    if (m_filterText == text)
        return;
    m_filterText = text;
    invalidateFilter();
    emit filtersChanged();
}

void InstanceModel::setLoaderFilter(const QString& loader)
{
    if (m_loaderFilter == loader)
        return;
    m_loaderFilter = loader;
    invalidateFilter();
    emit filtersChanged();
}

void InstanceModel::setGroupFilter(const QString& group)
{
    if (m_groupFilter == group)
        return;
    m_groupFilter = group;
    invalidateFilter();
    emit filtersChanged();
}

void InstanceModel::setTagFilter(const QString& tag)
{
    if (m_tagFilter == tag)
        return;
    m_tagFilter = tag;
    invalidateFilter();
    emit filtersChanged();
}

QStringList InstanceModel::allTags() const
{
    QStringList tags;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++)
        for (const auto& tag : instanceTags(list->at(i)))
            if (!tags.contains(tag, Qt::CaseInsensitive))
                tags.append(tag);
    tags.sort(Qt::CaseInsensitive);
    return tags;
}

void InstanceModel::tagsChanged()
{
    if (rowCount() > 0)
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { TagsRole });
    invalidateFilter();
    emit filtersChanged();
}

void InstanceModel::setSortMode(const QString& mode)
{
    if (m_sortMode == mode)
        return;
    m_sortMode = mode;
    APPLICATION->settings()->set("InstSortMode", mode);  // same setting as Settings > General > Instance sorting
    invalidate();
    sort(0);
    emit filtersChanged();
}

void InstanceModel::iconsChanged()
{
    s_iconRevision++;
    if (rowCount() > 0)
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), { IconRole });
}

bool InstanceModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    auto* instance = instanceAt(sourceModel()->index(sourceRow, 0, sourceParent));
    if (!instance)
        return false;
    if (!m_filterText.isEmpty() && !instance->name().contains(m_filterText, Qt::CaseInsensitive))
        return false;
    if (!m_groupFilter.isEmpty() && APPLICATION->instances()->getInstanceGroup(instance->id()) != m_groupFilter)
        return false;
    if (!m_tagFilter.isEmpty() && !instanceTags(instance).contains(m_tagFilter, Qt::CaseInsensitive))
        return false;
    if (!m_loaderFilter.isEmpty()) {
        const QString loader = packInfo(instance).loader;
        static const QStringList s_main = { "Vanilla", "Fabric", "Quilt", "Forge", "NeoForge" };
        if (m_loaderFilter == "Other" ? s_main.contains(loader) : loader != m_loaderFilter)
            return false;
    }
    return true;
}

bool InstanceModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    auto* a = instanceAt(left);
    auto* b = instanceAt(right);
    static QCollator s_collator = [] {
        QCollator collator;
        collator.setNumericMode(true);
        collator.setCaseSensitivity(Qt::CaseInsensitive);
        return collator;
    }();
    if (m_sortMode == "LastLaunch" && a->lastLaunch() != b->lastLaunch())
        return a->lastLaunch() > b->lastLaunch();
    if (m_sortMode == "Playtime" && a->totalTimePlayed() != b->totalTimePlayed())
        return a->totalTimePlayed() > b->totalTimePlayed();
    return s_collator.compare(a->name(), b->name()) < 0;
}

}  // namespace Wraith
