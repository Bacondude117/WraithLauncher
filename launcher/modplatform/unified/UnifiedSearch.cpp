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
#include "UnifiedSearch.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUrlQuery>

#include <algorithm>
#include <memory>

#include "Application.h"
#include "BuildConfig.h"
#include "modplatform/atlauncher/ATLPackIndex.h"
#include "modplatform/ftb/FTBPackManifest.h"
#include "net/ApiRequest.h"

namespace Unified {

namespace {
// The ATLauncher pack index is one large JSON file without a search API; download it once per session.
QList<ATLauncher::IndexedPack>& atlCache()
{
    static QList<ATLauncher::IndexedPack> s_packs;
    return s_packs;
}

QJsonObject parseObject(const QByteArray& data)
{
    return QJsonDocument::fromJson(data).object();
}
}  // namespace

UnifiedSearch::UnifiedSearch(QObject* parent) : QObject(parent) {}

UnifiedSearch::~UnifiedSearch()
{
    abort();
}

QList<Source> UnifiedSearch::enabledSources()
{
    QList<Source> sources;
    for (auto source : allSources()) {
        if (source == Source::CurseForge && !(APPLICATION->capabilities() & Application::SupportsFlame))
            continue;
        sources.append(source);
    }
    return sources;
}

void UnifiedSearch::search(const QString& query)
{
    abort();
    m_generation++;
    m_query = query.trimmed();
    m_hits.clear();
    m_packs.clear();
    m_errors.clear();
    m_pending = enabledSources();
    emit resultsChanged();

    for (auto source : QList<Source>(m_pending)) {
        switch (source) {
            case Source::Modrinth:
                startModrinth();
                break;
            case Source::CurseForge:
                startCurseForge();
                break;
            case Source::FTB:
                startFtb();
                break;
            case Source::ATLauncher:
                startAtl();
                break;
            case Source::Technic:
                startTechnic();
                break;
        }
    }
}

void UnifiedSearch::abort()
{
    // bump the generation first so callbacks of aborted jobs are ignored
    m_generation++;
    auto jobs = std::move(m_jobs);
    m_jobs.clear();
    for (auto& job : jobs)
        job->abort();
    m_pending.clear();
}

void UnifiedSearch::runJob(Source source, NetJob::Ptr job, Parser parse, bool lastStage)
{
    const int generation = m_generation;
    m_jobs.append(job);
    connect(job.get(), &Task::finished, this, [this, source, job, parse, lastStage, generation] {
        m_jobs.removeAll(job);
        if (generation != m_generation)
            return;
        const auto hits = parse();
        addHits(source, hits);
        if (lastStage)
            done(source, job->wasSuccessful() || !hits.isEmpty() ? QString() : job->failReason());
    });
    job->start();
}

void UnifiedSearch::addHits(Source source, const QList<SourceHit>& hits)
{
    Q_UNUSED(source)
    if (hits.isEmpty())
        return;
    m_hits.append(hits);
    m_packs = mergeHits(m_hits);
    rankPacks(m_packs, m_query);
    emit resultsChanged();
}

void UnifiedSearch::done(Source source, const QString& error)
{
    if (!error.isEmpty()) {
        qWarning() << "Unified search:" << sourceName(source) << "failed:" << error;
        m_errors.insert(source, error);
    }
    m_pending.removeAll(source);
    emit resultsChanged();
    if (m_pending.isEmpty())
        emit finished();
}

void UnifiedSearch::startModrinth()
{
    QUrlQuery q;
    q.addQueryItem("limit", QString::number(s_perSourceLimit));
    q.addQueryItem("index", m_query.isEmpty() ? "downloads" : "relevance");
    q.addQueryItem("facets", R"([["project_type:modpack"]])");
    if (!m_query.isEmpty())
        q.addQueryItem("query", m_query);
    QUrl url(BuildConfig.MODRINTH_PROD_URL + "/search");
    url.setQuery(q);

    auto job = makeShared<NetJob>("Unified::Modrinth", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(url);
    job->addNetAction(action);
    runJob(Source::Modrinth, job, [response] {
        QList<SourceHit> hits;
        const auto results = parseObject(*response).value("hits").toArray();
        for (int i = 0; i < results.size(); i++) {
            const auto obj = results[i].toObject();
            SourceHit hit;
            hit.source = Source::Modrinth;
            hit.id = obj.value("project_id").toString();
            hit.name = obj.value("title").toString();
            hit.author = obj.value("author").toString();
            hit.summary = obj.value("description").toString();
            hit.iconUrl = obj.value("icon_url").toString();
            hit.downloads = obj.value("downloads").toInteger(-1);
            hit.websiteUrl = "https://modrinth.com/modpack/" + obj.value("slug").toString();
            hit.rank = i;
            if (!hit.id.isEmpty() && !hit.name.isEmpty())
                hits.append(hit);
        }
        return hits;
    });
}

void UnifiedSearch::startCurseForge()
{
    QUrlQuery q;
    q.addQueryItem("gameId", "432");
    q.addQueryItem("classId", "4471");  // modpacks
    q.addQueryItem("pageSize", QString::number(s_perSourceLimit));
    q.addQueryItem("sortField", "2");  // popularity; relevance to the query is applied by rankPacks()
    q.addQueryItem("sortOrder", "desc");
    if (!m_query.isEmpty())
        q.addQueryItem("searchFilter", m_query);
    QUrl url(BuildConfig.FLAME_BASE_URL + "/mods/search");
    url.setQuery(q);

    auto job = makeShared<NetJob>("Unified::CurseForge", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(url);
    job->addNetAction(action);
    runJob(Source::CurseForge, job, [response] {
        QList<SourceHit> hits;
        const auto results = parseObject(*response).value("data").toArray();
        for (int i = 0; i < results.size(); i++) {
            const auto obj = results[i].toObject();
            SourceHit hit;
            hit.source = Source::CurseForge;
            hit.id = QString::number(obj.value("id").toInteger());
            hit.name = obj.value("name").toString();
            const auto authors = obj.value("authors").toArray();
            if (!authors.isEmpty())
                hit.author = authors.first().toObject().value("name").toString();
            hit.summary = obj.value("summary").toString();
            hit.iconUrl = obj.value("logo").toObject().value("thumbnailUrl").toString();
            hit.websiteUrl = obj.value("links").toObject().value("websiteUrl").toString();
            hit.downloads = static_cast<qint64>(obj.value("downloadCount").toDouble(-1));
            hit.rank = i;
            if (!hit.name.isEmpty())
                hits.append(hit);
        }
        return hits;
    });
}

void UnifiedSearch::startFtb()
{
    // Stage 1: search (or popular list) returns pack ids only.
    const QString path = m_query.isEmpty() ? QString("/modpack/popular/installs/%1").arg(s_perSourceLimit)
                                           : QString("/modpack/search/%1?term=%2")
                                                 .arg(s_perSourceLimit)
                                                 .arg(QString::fromUtf8(QUrl::toPercentEncoding(m_query)));
    auto job = makeShared<NetJob>("Unified::FTB", APPLICATION->network());
    auto [action, response] = Net::Request::makeByteArray(QUrl(BuildConfig.FTB_API_BASE_URL + path));
    job->addNetAction(action);
    const int generation = m_generation;
    runJob(
        Source::FTB, job,
        [this, response, generation] {
            QList<int> ids;
            for (const auto& v : parseObject(*response).value("packs").toArray())
                ids.append(v.toInt());
            if (ids.isEmpty()) {
                done(Source::FTB);
            } else {
                startFtbDetails(ids.mid(0, 12), generation);
            }
            return QList<SourceHit>();
        },
        false);
}

void UnifiedSearch::startFtbDetails(const QList<int>& ids, int generation)
{
    if (generation != m_generation)
        return;
    // Stage 2: fetch details for each id; the job runs the requests concurrently.
    auto job = makeShared<NetJob>("Unified::FTBDetails", APPLICATION->network());
    auto responses = std::make_shared<QList<std::pair<int, QByteArray*>>>();
    for (int i = 0; i < ids.size(); i++) {
        auto [action, response] = Net::Request::makeByteArray(QUrl(BuildConfig.FTB_API_BASE_URL + QString("/modpack/%1").arg(ids[i])));
        job->addNetAction(action);
        responses->append({ i, response });
    }
    runJob(Source::FTB, job, [responses] {
        QList<SourceHit> hits;
        for (const auto& [rank, data] : *responses) {
            if (data->isEmpty())
                continue;
            FTB::Modpack pack;
            if (!FTB::loadModpack(pack, parseObject(*data)) || pack.versions.isEmpty())
                continue;
            SourceHit hit;
            hit.source = Source::FTB;
            hit.id = QString::number(pack.id);
            hit.name = pack.name;
            if (!pack.authors.isEmpty())
                hit.author = pack.authors.first().name;
            hit.summary = pack.synopsis;
            for (const auto& art : pack.art) {
                if (art.type == "square")
                    hit.iconUrl = art.url;
            }
            hit.downloads = pack.installs;
            hit.rank = rank;
            hits.append(hit);
        }
        return hits;
    });
}

void UnifiedSearch::startAtl()
{
    auto filter = [this] {
        QList<std::pair<double, const ATLauncher::IndexedPack*>> matches;
        for (const auto& pack : atlCache()) {
            double score = 1;
            if (!m_query.isEmpty()) {
                score = textRelevance(m_query, pack.name);
                if (score <= 0 && pack.description.contains(m_query, Qt::CaseInsensitive))
                    score = 0.2;
            }
            if (score > 0)
                matches.append({ score, &pack });
        }
        // ATLauncher's "position" is its own popularity ordering (lower = more popular)
        std::stable_sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) {
            if (a.first != b.first)
                return a.first > b.first;
            return a.second->position < b.second->position;
        });

        QList<SourceHit> hits;
        for (int i = 0; i < matches.size() && i < s_perSourceLimit; i++) {
            const auto* pack = matches[i].second;
            SourceHit hit;
            hit.source = Source::ATLauncher;
            hit.id = QString::number(pack->id);
            hit.name = pack->name;
            hit.summary = pack->description;
            hit.iconUrl = BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/images/" + pack->safeName;
            hit.rank = i;
            hits.append(hit);
        }
        return hits;
    };

    if (!atlCache().isEmpty()) {
        const int generation = m_generation;
        QTimer::singleShot(0, this, [this, filter, generation] {
            if (generation != m_generation)
                return;
            addHits(Source::ATLauncher, filter());
            done(Source::ATLauncher);
        });
        return;
    }

    auto job = makeShared<NetJob>("Unified::ATLauncher", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "launcher/json/packsnew.json"));
    job->addNetAction(action);
    runJob(Source::ATLauncher, job, [response, filter] {
        auto& cache = atlCache();
        for (const auto& v : QJsonDocument::fromJson(*response).array()) {
            auto obj = v.toObject();
            ATLauncher::IndexedPack pack;
            if (!ATLauncher::loadIndexedPack(pack, obj))
                continue;
            // same rules as the ATLauncher page: public, non-system packs with at least one version
            if (pack.type != ATLauncher::PackType::Public || pack.system || pack.versions.isEmpty())
                continue;
            cache.append(pack);
        }
        return filter();
    });
}

void UnifiedSearch::startTechnic()
{
    QUrlQuery q;
    q.addQueryItem("build", BuildConfig.TECHNIC_API_BUILD);
    if (!m_query.isEmpty())
        q.addQueryItem("q", m_query);
    QUrl url(BuildConfig.TECHNIC_API_BASE_URL + (m_query.isEmpty() ? "trending" : "search"));
    url.setQuery(q);

    auto job = makeShared<NetJob>("Unified::Technic", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(url);
    job->addNetAction(action);
    runJob(Source::Technic, job, [response] {
        QList<SourceHit> hits;
        const auto results = parseObject(*response).value("modpacks").toArray();
        for (int i = 0; i < results.size() && hits.size() < s_perSourceLimit; i++) {
            const auto obj = results[i].toObject();
            SourceHit hit;
            hit.source = Source::Technic;
            hit.id = obj.value("slug").toString();
            hit.name = obj.value("name").toString();
            hit.iconUrl = obj.value("iconUrl").toString();
            hit.websiteUrl = obj.value("url").toString();
            hit.rank = i;
            if (!hit.id.isEmpty() && hit.id != "vanilla" && !hit.name.isEmpty())
                hits.append(hit);
        }
        return hits;
    });
}

}  // namespace Unified
