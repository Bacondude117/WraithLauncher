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

#include <QRegularExpression>
#include <QTimer>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "InstanceImportTask.h"
#include "icons/IconList.h"
#include "modplatform/atlauncher/ATLPackInstallTask.h"
#include "modplatform/ftb/FTBPackInstallTask.h"
#include "modplatform/technic/SingleZipPackInstallTask.h"
#include "modplatform/technic/SolderPackInstallTask.h"
#include "modplatform/technic/SolderPackManifest.h"
#include "net/ApiRequest.h"
#include "ui/MainWindow.h"
#include "ui/pages/modplatform/atlauncher/AtlUserInteractionSupportImpl.h"
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

    // Sources the user switched off stay off (and are not contacted) across restarts.
    for (const auto& name : APPLICATION->settings()->get("WraithBrowseSources").toString().split(',', Qt::SkipEmptyParts)) {
        if (auto source = sourceByName(name))
            m_disabled.insert(*source);
    }
    m_search->setSkippedSources(m_disabled);
}

bool BrowseModel::loadPopularOnOpen() const
{
    return APPLICATION->settings()->get("WraithBrowseLoadPopular").toBool();
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
    QStringList names;
    for (auto s : allSources()) {
        if (m_disabled.contains(s))
            names.append(sourceName(s));
    }
    APPLICATION->settings()->set("WraithBrowseSources", names.join(','));
    m_search->setSkippedSources(m_disabled);
    // a newly enabled source hasn't been asked yet
    if (enabled && !m_search->packs().isEmpty())
        m_search->search(m_search->query());
    rebuild();
    emit statusChanged();
}

QVariantMap BrowseModel::details(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    const auto& pack = m_visible.at(row);
    QVariantList hits;
    // The default install source is the one that ranked this pack best for this search, not a favoured source.
    const SourceHit* best = nullptr;
    for (const auto& hit : pack.hits) {
        if (m_disabled.contains(hit.source))
            continue;
        hits.append(QVariantMap{ { "source", sourceName(hit.source) }, { "url", hit.websiteUrl }, { "id", hit.id }, { "name", hit.name } });
        if (!best || hit.rank < best->rank)
            best = &hit;
    }
    return { { "defaultSource", best ? sourceName(best->source) : QString() },
             { "name", pack.name },
             { "author", pack.author },
             { "summary", pack.summary },
             { "icon", pack.iconUrl },
             { "downloads", pack.downloads > 0 ? PackCardDelegate::formatDownloads(pack.downloads) : QString() },
             { "hits", hits } };
}

// ---------------------------------------------------------------- versions (one code path per source, same UI for all)

namespace {
QString titleCaseLoader(QString name)
{
    if (name.isEmpty())
        return name;
    name[0] = name[0].toUpper();
    return name == "Neoforge" ? "NeoForge" : name;
}

QString curseForgeReleaseType(int type)
{
    switch (type) {
        case 2:
            return "beta";
        case 3:
            return "alpha";
        default:
            return "release";
    }
}
}  // namespace

void BrowseModel::loadVersions(int row, const QString& sourceName)
{
    auto source = sourceByName(sourceName);
    if (row < 0 || row >= m_visible.size() || !source)
        return;
    const auto* hit = m_visible.at(row).hit(*source);
    if (!hit)
        return;
    if (m_versionsJob)
        m_versionsJob->abort();
    m_versionsJob.reset();

    const QString key = sourceName;
    auto finish = [this, row, key](QList<QVariantMap> list) {
        QVariantList versions;
        for (auto& version : list) {
            version.insert("source", key);
            m_versions.insert(key + "|" + version.value("id").toString(), version);
            versions.append(version);
        }
        emit versionsLoaded(row, key, versions);
    };
    auto fetch = [this, row, key](const QUrl& url, std::function<void(const QByteArray&)> parse) {
        m_versionsJob = makeShared<NetJob>("Wraith::Versions", APPLICATION->network());
        auto [action, response] = Net::ApiRequest::makeByteArray(url);
        m_versionsJob->addNetAction(action);
        connect(m_versionsJob.get(), &Task::succeeded, this, [response, parse] { parse(*response); });
        connect(m_versionsJob.get(), &Task::failed, this, [this, row, key](const QString& reason) { emit versionsFailed(row, key, reason); });
        m_versionsJob->start();
    };

    switch (*source) {
        case Source::Modrinth:
            fetch(QUrl(BuildConfig.MODRINTH_PROD_URL + QString("/project/%1/version").arg(hit->id)), [finish](const QByteArray& data) {
                QList<QVariantMap> list;
                for (const auto& value : QJsonDocument::fromJson(data).array()) {
                    const auto obj = value.toObject();
                    QJsonObject file;
                    for (const auto& f : obj.value("files").toArray()) {
                        if (file.isEmpty() || f.toObject().value("primary").toBool())
                            file = f.toObject();
                    }
                    QStringList games;
                    for (const auto& game : obj.value("game_versions").toArray())
                        games.append(game.toString());
                    QStringList loaders;
                    for (const auto& loader : obj.value("loaders").toArray())
                        loaders.append(titleCaseLoader(loader.toString()));
                    list.append({ { "id", obj.value("id").toString() },
                                  { "number", obj.value("version_number").toString() },
                                  { "type", obj.value("version_type").toString() },
                                  { "minecraft", games.join(", ") },
                                  { "loaders", loaders.join(", ") },
                                  { "url", file.value("url").toString() },
                                  { "hashAlgo", "sha512" },
                                  { "hash", file.value("hashes").toObject().value("sha512").toString() } });
                }
                finish(list);
            });
            break;
        case Source::CurseForge:
            fetch(QUrl(BuildConfig.FLAME_BASE_URL + QString("/mods/%1/files?pageSize=50").arg(hit->id)), [finish](const QByteArray& data) {
                QList<QVariantMap> list;
                for (const auto& value : QJsonDocument::fromJson(data).object().value("data").toArray()) {
                    const auto obj = value.toObject();
                    QString sha1;
                    for (const auto& h : obj.value("hashes").toArray()) {
                        if (h.toObject().value("algo").toInt() == 1)
                            sha1 = h.toObject().value("value").toString();
                    }
                    QStringList games;
                    QStringList loaders;
                    for (const auto& v : obj.value("gameVersions").toArray()) {
                        const QString s = v.toString();
                        (s.isEmpty() || s.front().isDigit() ? games : loaders).append(s);
                    }
                    loaders.removeAll("Client");
                    loaders.removeAll("Server");
                    list.append({ { "id", QString::number(obj.value("id").toInteger()) },
                                  { "number", obj.value("displayName").toString() },
                                  { "type", curseForgeReleaseType(obj.value("releaseType").toInt()) },
                                  { "minecraft", games.join(", ") },
                                  { "loaders", loaders.join(", ") },
                                  { "url", obj.value("downloadUrl").toString() },
                                  { "hashAlgo", "sha1" },
                                  { "hash", sha1 } });
                }
                finish(list);
            });
            break;
        case Source::FTB:
            fetch(QUrl(BuildConfig.FTB_API_BASE_URL + QString("/modpack/%1").arg(hit->id)), [this, finish, id = hit->id](const QByteArray& data) {
                FTB::Modpack pack;
                if (!FTB::loadModpack(pack, QJsonDocument::fromJson(data).object()))
                    return finish({});
                m_ftbPacks.insert(id, pack);
                QList<FTB::VersionInfo> versions(pack.versions.begin(), pack.versions.end());
                std::sort(versions.begin(), versions.end(), [](const auto& a, const auto& b) { return a.updated > b.updated; });
                QList<QVariantMap> list;
                // FTB's pack listing has no Minecraft/loader per version (only the install manifest does)
                for (const auto& version : versions)
                    list.append({ { "id", version.name }, { "number", version.name }, { "type", version.type.toLower() } });
                finish(list);
            });
            break;
        case Source::ATLauncher: {
            // The ATLauncher index (already downloaded by the search) lists every version.
            QList<QVariantMap> list;
            for (const auto& pack : Unified::UnifiedSearch::atlauncherPacks()) {
                if (QString::number(pack.id) != hit->id)
                    continue;
                for (const auto& version : pack.versions)
                    list.append({ { "id", version.version }, { "number", version.version }, { "type", "release" },
                                  { "minecraft", version.minecraft }, { "name", pack.name } });
            }
            QTimer::singleShot(0, this, [finish, list] { finish(list); });
            break;
        }
        case Source::Technic:
            fetch(QUrl(QString("%1modpack/%2?build=%3").arg(BuildConfig.TECHNIC_API_BASE_URL, hit->id, BuildConfig.TECHNIC_API_BUILD)),
                  [this, finish, row, key, slug = hit->id](const QByteArray& data) {
                      const auto obj = QJsonDocument::fromJson(data).object();
                      const QString minecraft = obj.value("minecraft").toString();
                      if (obj.value("url").isString() && !obj.value("url").toString().isEmpty()) {
                          // a single zip: only its current version can be installed
                          finish({ { { "id", obj.value("version").toString() },
                                     { "number", obj.value("version").toString() },
                                     { "type", "release" },
                                     { "minecraft", minecraft },
                                     { "url", obj.value("url").toString() } } });
                          return;
                      }
                      QString solder = obj.value("solder").toString();
                      while (solder.endsWith('/'))
                          solder.chop(1);
                      if (solder.isEmpty())
                          return finish({});
                      // a Solder pack: ask its Solder server for the builds
                      QTimer::singleShot(0, this, [this, finish, row, key, slug, solder, minecraft] {
                          m_versionsJob = makeShared<NetJob>("Wraith::Solder", APPLICATION->network());
                          auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(QString("%1/modpack/%2").arg(solder, slug)));
                          m_versionsJob->addNetAction(action);
                          connect(m_versionsJob.get(), &Task::succeeded, this, [finish, response, solder, minecraft] {
                              TechnicSolder::Pack pack;
                              QList<QVariantMap> list;
                              if (TechnicSolder::loadPack(pack, QJsonDocument::fromJson(*response).object())) {
                                  for (auto i = pack.builds.size(); i-- > 0;) {
                                      const QString build = pack.builds.at(i);
                                      list.append({ { "id", build },
                                                    { "number", build == pack.recommended ? build + " (recommended)" : build },
                                                    { "type", "release" },
                                                    { "minecraft", minecraft },
                                                    { "solder", solder } });
                                  }
                              }
                              finish(list);
                          });
                          connect(m_versionsJob.get(), &Task::failed, this,
                                  [this, row, key](const QString& reason) { emit versionsFailed(row, key, reason); });
                          m_versionsJob->start();
                      });
                  });
            break;
    }
}

// ---------------------------------------------------------------- install

void BrowseModel::install(int row, const QString& sourceName, const QString& versionId)
{
    auto source = sourceByName(sourceName);
    if (row < 0 || row >= m_visible.size() || !source || !m_versions.contains(sourceName + "|" + versionId))
        return;
    const auto pack = m_visible.at(row);
    const auto* hit = pack.hit(*source);
    const auto version = m_versions.value(sourceName + "|" + versionId);
    if (!hit)
        return;

    InstanceTask* task = nullptr;
    const QString url = version.value("url").toString();
    switch (*source) {
        case Source::Modrinth:
        case Source::CurseForge: {
            if (url.isEmpty()) {
                // CurseForge authors can forbid third-party downloads; respect that and point to their page.
                m_shell->toast(tr("The author of %1 only allows downloads from the %2 website.").arg(pack.name, sourceName), "warning",
                               tr("Open website"), [site = hit->websiteUrl] { DesktopServices::openUrl(QUrl(site)); });
                return;
            }
            QMap<QString, QString> extra{ { "pack_id", hit->id },
                                          { "pack_version_id", versionId },
                                          { "file_hash", version.value("hash").toString() },
                                          { "file_hash_algo", version.value("hashAlgo").toString() } };
            task = new InstanceImportTask(QUrl(url), true, m_shell, std::move(extra));
            break;
        }
        case Source::FTB:
            if (!m_ftbPacks.contains(hit->id))
                return;
            task = new FTB::PackInstallTask(m_ftbPacks.value(hit->id), versionId, m_shell);
            break;
        case Source::ATLauncher:
            task = new ATLauncher::PackInstallTask(new AtlUserInteractionSupportImpl(m_shell), version.value("name").toString(), versionId);
            break;
        case Source::Technic:
            if (version.contains("solder"))
                task = new Technic::SolderPackInstallTask(APPLICATION->network(), version.value("solder").toString(), hit->id, versionId,
                                                          version.value("minecraft").toString());
            else
                task = new Technic::SingleZipPackInstallTask(QUrl(url), version.value("minecraft").toString());
            break;
    }
    if (!task)
        return;

    auto start = [this, pack, task, number = version.value("number").toString()](const QString& iconKey) {
        task->setName(pack.name);
        task->setOriginalName(pack.name, number);
        task->setGroup(APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString());
        task->setIcon(iconKey);
        m_shell->toast(tr("Installing %1...").arg(pack.name));
        m_shell->window()->wraithInstallFromTask(task);
    };

    // bring the pack's icon along, like the per-source pages do
    const QString iconUrl = hit->iconUrl.isEmpty() ? pack.iconUrl : hit->iconUrl;
    if (iconUrl.isEmpty()) {
        start("default");
        return;
    }
    QString safeId = hit->id;
    safeId.replace(QRegularExpression("[^A-Za-z0-9_-]"), "_");
    const QString iconName = sourceName.toLower() + "_" + safeId + ".png";
    const QString iconPath = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("wraith_" + iconName);
    m_iconJob = makeShared<NetJob>("Wraith::PackIcon", APPLICATION->network());
    m_iconJob->addNetAction(Net::ApiRequest::makeFile(QUrl(iconUrl), iconPath));
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
