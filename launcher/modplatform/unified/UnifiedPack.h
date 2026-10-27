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

#include <QList>
#include <QString>

/**
 * Data model and pure logic for the unified modpack browser: one search across every modpack source,
 * with the same pack from several sources merged into a single entry.
 * Nothing in here touches the network or the UI so it can be unit tested.
 */
namespace Unified {

enum class Source { Modrinth, CurseForge, FTB, ATLauncher, Technic };

/** All sources, in the order their tags are shown and their data is preferred when merging. */
QList<Source> allSources();
QString sourceName(Source source);

/** One search result from one source. */
struct SourceHit {
    Source source = Source::Modrinth;
    QString id;  ///< source-specific id (project id, pack id, slug...)
    QString name;
    QString author;
    QString summary;
    QString iconUrl;
    QString websiteUrl;
    qint64 downloads = -1;  ///< -1 when the source doesn't report popularity
    int rank = 0;           ///< position in that source's own result list (0 = best match)
};

/** A modpack after merging duplicates; has at most one hit per source. */
struct Pack {
    QString name;
    QString author;
    QString summary;
    QString iconUrl;
    QList<SourceHit> hits;
    qint64 downloads = 0;  ///< sum of the known download counts
    double relevance = 0;

    bool hasSource(Source source) const;
    const SourceHit* hit(Source source) const;
    int bestRank() const;
};

/** Lowercase, drop bracketed text, accents and anything that isn't a letter or digit. */
QString normalizeName(const QString& name);
/** Like normalizeName, but also drops a trailing subtitle ("Pack - Subtitle", "Pack: Subtitle", "Pack | x"). */
QString baseName(const QString& name);
QString normalizeAuthor(const QString& author);

/** true when the two hits are confidently the same pack (see UnifiedPack.cpp for the rules). */
bool isSamePack(const SourceHit& a, const SourceHit& b);

/** How well a pack name matches the query, 0..1, independent of the sources' own ordering. */
double textRelevance(const QString& query, const QString& name);

/** Merge hits from all sources into packs. Order of the input does not matter. */
QList<Pack> mergeHits(const QList<SourceHit>& hits);

/**
 * Score and sort packs: relevance first (text match, each source's own ranking, multi-source bonus),
 * bucketed so that packs of similar relevance are then ordered by popularity.
 * With an empty query, packs are ordered by popularity.
 */
void rankPacks(QList<Pack>& packs, const QString& query);

}  // namespace Unified
