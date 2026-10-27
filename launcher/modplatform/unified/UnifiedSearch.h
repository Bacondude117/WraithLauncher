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

#include <QHash>
#include <QObject>
#include <functional>

#include "modplatform/unified/UnifiedPack.h"
#include "net/NetJob.h"

namespace Unified {

/**
 * Searches every enabled modpack source in parallel and keeps a merged, ranked result list.
 * Results are re-merged each time a source answers, so the UI can show partial results early.
 * A failing source never blocks the others; its error is kept in errors().
 */
class UnifiedSearch : public QObject {
    Q_OBJECT
   public:
    static constexpr int s_perSourceLimit = 20;

    explicit UnifiedSearch(QObject* parent = nullptr);
    ~UnifiedSearch() override;

    void search(const QString& query);
    void abort();

    QString query() const { return m_query; }
    const QList<Pack>& packs() const { return m_packs; }
    bool isRunning() const { return !m_pending.isEmpty(); }
    const QList<Source>& pendingSources() const { return m_pending; }
    const QHash<Source, QString>& errors() const { return m_errors; }

    /** Sources that will be searched; CurseForge needs an API key. */
    static QList<Source> enabledSources();

   signals:
    void resultsChanged();
    void finished();

   private:
    using Parser = std::function<QList<SourceHit>()>;

    void startModrinth();
    void startCurseForge();
    void startFtb();
    void startFtbDetails(const QList<int>& ids, int generation);
    void startAtl();
    void startTechnic();

    /** Run job for source; when it finishes (successfully or not), parse() turns whatever arrived into hits. */
    void runJob(Source source, NetJob::Ptr job, Parser parse, bool lastStage = true);
    void addHits(Source source, const QList<SourceHit>& hits);
    void done(Source source, const QString& error = {});

   private:
    QString m_query;
    int m_generation = 0;
    QList<Source> m_pending;
    QList<SourceHit> m_hits;
    QList<Pack> m_packs;
    QHash<Source, QString> m_errors;
    QList<NetJob::Ptr> m_jobs;
};

}  // namespace Unified
