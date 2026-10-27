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
#include "WraithBrowse.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include "Application.h"
#include "InstanceImportTask.h"
#include "icons/IconList.h"
#include "net/ApiRequest.h"
#include "ui/MainWindow.h"
#include "ui/widgets/PackCardDelegate.h"

#include "WraithShell.h"

using namespace Unified;

namespace Wraith {

namespace {
QString pageIdFor(Source source)
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

std::optional<Source> sourceByName(const QString& name)
{
    for (auto source : allSources()) {
        if (sourceName(source) == name)
            return source;
    }
    return std::nullopt;
}
}  // namespace

BrowseModel::BrowseModel(Shell* shell) : QAbstractListModel(shell), m_shell(shell), m_search(new UnifiedSearch(this))
{
    connect(m_search, &UnifiedSearch::resultsChanged, this, [this] {
        rebuild();
        emit statusChanged();
    });
    connect(m_search, &UnifiedSearch::finished, this, &BrowseModel::statusChanged);
}

QHash<int, QByteArray> BrowseModel::roleNames() const
{
    return { { NameRole, "name" },           { AuthorRole, "author" },       { SummaryRole, "summary" },
             { IconRole, "icon" },           { DownloadsRole, "downloads" }, { SourcesRole, "sources" } };
}

QVariant BrowseModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const auto& pack = m_visible.at(index.row());
    switch (role) {
        case NameRole:
            return pack.name;
        case AuthorRole:
            return pack.author;
        case SummaryRole:
            return pack.summary;
        case IconRole:
            return pack.iconUrl;
        case DownloadsRole:
            return pack.downloads > 0 ? PackCardDelegate::formatDownloads(pack.downloads) : QString();
        case SourcesRole: {
            QStringList names;
            for (const auto& hit : pack.hits)
                names.append(sourceName(hit.source));
            return names;
        }
        default:
            return {};
    }
}

void BrowseModel::rebuild()
{
    beginResetModel();
    m_visible.clear();
    for (const auto& pack : m_search->packs()) {
        bool shown = false;
        for (const auto& hit : pack.hits)
            shown |= !m_disabled.contains(hit.source);
        if (shown)
            m_visible.append(pack);
    }
    if (m_sortMode == "downloads")
        std::stable_sort(m_visible.begin(), m_visible.end(), [](const Pack& a, const Pack& b) { return a.downloads > b.downloads; });
    endResetModel();
    emit filtersChanged();
}

QString BrowseModel::status() const
{
    if (m_search->isRunning()) {
        QStringList pending;
        for (auto source : m_search->pendingSources())
            pending.append(sourceName(source));
        return tr("Searching %1...").arg(pending.join(", "));
    }
    QStringList parts;
    for (auto it = m_search->errors().constBegin(); it != m_search->errors().constEnd(); ++it)
        parts.append(tr("Couldn't reach %1.").arg(sourceName(it.key())));
    return parts.join(' ');
}

QVariantList BrowseModel::sources() const
{
    const auto enabled = UnifiedSearch::enabledSources();
    QVariantList result;
    for (auto source : allSources()) {
        int count = 0;
        for (const auto& pack : m_search->packs())
            count += pack.hasSource(source) ? 1 : 0;
        result.append(QVariantMap{ { "name", sourceName(source) },
                                   { "count", count },
                                   { "enabled", !m_disabled.contains(source) },
                                   { "available", enabled.contains(source) },
                                   { "pending", m_search->pendingSources().contains(source) },
                                   { "failed", m_search->errors().contains(source) } });
    }
    return result;
}

void BrowseModel::setSortMode(const QString& mode)
{
    if (m_sortMode == mode)
        return;
    m_sortMode = mode;
    rebuild();
}

bool BrowseModel::curseForgeMissing() const
{
    return !UnifiedSearch::enabledSources().contains(Source::CurseForge);
}

void BrowseModel::search(const QString& query)
{
    m_search->search(query);
    emit statusChanged();
}

void BrowseModel::setSourceEnabled(const QString& name, bool enabled)
{
    auto source = sourceByName(name);
    if (!source)
        return;
    if (enabled)
        m_disabled.remove(*source);
    else
        m_disabled.insert(*source);
    rebuild();
    emit statusChanged();
}

QVariantMap BrowseModel::details(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    const auto& pack = m_visible.at(row);
    QVariantList hits;
    for (const auto& hit : pack.hits)
        hits.append(QVariantMap{ { "source", sourceName(hit.source) }, { "url", hit.websiteUrl }, { "id", hit.id }, { "name", hit.name } });
    return { { "name", pack.name },
             { "author", pack.author },
             { "summary", pack.summary },
             { "icon", pack.iconUrl },
             { "downloads", pack.downloads > 0 ? PackCardDelegate::formatDownloads(pack.downloads) : QString() },
             { "hits", hits } };
}

void BrowseModel::loadVersions(int row)
{
    if (row < 0 || row >= m_visible.size())
        return;
    const auto* hit = m_visible.at(row).hit(Source::Modrinth);
    if (!hit)
        return;
    m_versionsJob = makeShared<NetJob>("Wraith::ModrinthVersions", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(QString("https://api.modrinth.com/v2/project/%1/version").arg(hit->id)));
    m_versionsJob->addNetAction(action);
    connect(m_versionsJob.get(), &Task::succeeded, this, [this, row, response] {
        QVariantList versions;
        for (const auto& value : QJsonDocument::fromJson(*response).array()) {
            const auto obj = value.toObject();
            QString url;
            for (const auto& file : obj.value("files").toArray()) {
                if (url.isEmpty() || file.toObject().value("primary").toBool())
                    url = file.toObject().value("url").toString();
            }
            QStringList games;
            for (const auto& game : obj.value("game_versions").toArray())
                games.append(game.toString());
            QStringList loaders;
            for (const auto& loader : obj.value("loaders").toArray()) {
                QString name = loader.toString();
                name[0] = name[0].toUpper();
                loaders.append(name == "Neoforge" ? "NeoForge" : name);
            }
            QVariantMap version{ { "id", obj.value("id").toString() },
                                 { "name", obj.value("name").toString() },
                                 { "number", obj.value("version_number").toString() },
                                 { "type", obj.value("version_type").toString() },
                                 { "minecraft", games.join(", ") },
                                 { "loaders", loaders.join(", ") },
                                 { "url", url } };
            m_versions.insert(version.value("id").toString(), version);
            versions.append(version);
        }
        emit versionsLoaded(row, versions);
    });
    connect(m_versionsJob.get(), &Task::failed, this, [this, row](const QString& reason) { emit versionsFailed(row, reason); });
    m_versionsJob->start();
}

void BrowseModel::installModrinth(int row, const QString& versionId)
{
    if (row < 0 || row >= m_visible.size() || !m_versions.contains(versionId))
        return;
    const auto pack = m_visible.at(row);
    const auto* hit = pack.hit(Source::Modrinth);
    const auto version = m_versions.value(versionId);
    if (!hit || version.value("url").toString().isEmpty())
        return;

    auto start = [this, pack, projectId = hit->id, version](const QString& iconKey) {
        QMap<QString, QString> extra{ { "pack_id", projectId }, { "pack_version_id", version.value("id").toString() } };
        auto* task = new InstanceImportTask(QUrl(version.value("url").toString()), true, m_shell, std::move(extra));
        task->setName(pack.name);
        task->setOriginalName(pack.name, version.value("number").toString());
        task->setGroup(APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString());
        task->setIcon(iconKey);
        m_shell->toast(tr("Installing %1...").arg(pack.name));
        m_shell->window()->wraithInstallFromTask(task);
    };

    // bring the pack's icon along, like the Modrinth page does
    if (pack.iconUrl.isEmpty()) {
        start("default");
        return;
    }
    const QString iconName = "modrinth_" + hit->id + ".png";
    const QString iconPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("wraith_" + iconName);
    m_iconJob = makeShared<NetJob>("Wraith::PackIcon", APPLICATION->network());
    m_iconJob->addNetAction(Net::ApiRequest::makeFile(QUrl(pack.iconUrl), iconPath));
    connect(m_iconJob.get(), &Task::finished, this, [iconPath, iconName, start] {
        QString key = "default";
        if (QFile::exists(iconPath)) {
            APPLICATION->icons()->installIcon(iconPath, iconName);
            key = iconName.left(iconName.lastIndexOf('.'));
        }
        start(key);
    });
    m_iconJob->start();
}

void BrowseModel::installWith(int row, const QString& sourceName)
{
    auto source = sourceByName(sourceName);
    if (row < 0 || row >= m_visible.size() || !source)
        return;
    const auto* hit = m_visible.at(row).hit(*source);
    if (!hit)
        return;
    // Technic's page resolves "#slug" to exactly one pack; the others filter by name
    const QString term = *source == Source::Technic ? "#" + hit->id : hit->name;
    m_shell->window()->wraithNewInstanceDialog({}, pageIdFor(*source), term);
}

void BrowseModel::openAdvanced()
{
    m_shell->window()->wraithNewInstanceDialog({}, "advanced_sources", {});
}

}  // namespace Wraith
