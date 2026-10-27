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
#include "UnifiedPack.h"

#include <QRegularExpression>
#include <QSet>

#include <algorithm>
#include <climits>
#include <cmath>

namespace Unified {

QList<Source> allSources()
{
    return { Source::Modrinth, Source::CurseForge, Source::FTB, Source::ATLauncher, Source::Technic };
}

QString sourceName(Source source)
{
    switch (source) {
        case Source::Modrinth:
            return "Modrinth";
        case Source::CurseForge:
            return "CurseForge";
        case Source::FTB:
            return "FTB";
        case Source::ATLauncher:
            return "ATLauncher";
        case Source::Technic:
            return "Technic";
    }
    return {};
}

bool Pack::hasSource(Source source) const
{
    return hit(source) != nullptr;
}

const SourceHit* Pack::hit(Source source) const
{
    for (const auto& h : hits) {
        if (h.source == source)
            return &h;
    }
    return nullptr;
}

int Pack::bestRank() const
{
    int best = INT_MAX;
    for (const auto& h : hits)
        best = std::min(best, h.rank);
    return best;
}

namespace {
QString stripToAlnum(QString text)
{
    // Decompose accents (é -> e + combining mark), then keep only letters and digits.
    text = text.normalized(QString::NormalizationForm_KD).toLower();
    QString out;
    out.reserve(text.size());
    for (const QChar c : text) {
        if (c.isLetterOrNumber())
            out.append(c);
    }
    return out;
}

QString removeBrackets(const QString& name)
{
    static const QRegularExpression s_brackets(R"(\([^)]*\)|\[[^\]]*\]|\{[^}]*\})");
    return QString(name).remove(s_brackets);
}

QStringList tokens(const QString& text)
{
    static const QRegularExpression s_split(R"([^\p{L}\p{N}]+)");
    QStringList out;
    for (const auto& t : text.normalized(QString::NormalizationForm_KD).toLower().split(s_split, Qt::SkipEmptyParts)) {
        QString clean = stripToAlnum(t);
        if (!clean.isEmpty())
            out.append(clean);
    }
    return out;
}

bool authorsCompatible(const QString& a, const QString& b)
{
    const QString na = normalizeAuthor(a);
    const QString nb = normalizeAuthor(b);
    if (na.isEmpty() || nb.isEmpty())
        return false;
    return na == nb || na.contains(nb) || nb.contains(na);
}

bool authorUnknown(const SourceHit& hit)
{
    return normalizeAuthor(hit.author).isEmpty();
}
}  // namespace

QString normalizeName(const QString& name)
{
    return stripToAlnum(removeBrackets(name));
}

QString baseName(const QString& name)
{
    static const QRegularExpression s_subtitle(R"(\s+[-–—|:]\s+.*$|:\s.*$)");
    QString base = removeBrackets(name);
    base.remove(s_subtitle);
    return stripToAlnum(base);
}

QString normalizeAuthor(const QString& author)
{
    // "TeamName", "Team Name" and "team-name" should match
    return stripToAlnum(author);
}

bool isSamePack(const SourceHit& a, const SourceHit& b)
{
    if (a.source == b.source)
        return a.id == b.id;

    const QString na = normalizeName(a.name);
    const QString nb = normalizeName(b.name);
    if (na.isEmpty() || nb.isEmpty())
        return false;

    // Same full name: merge unless both authors are known and clearly different.
    if (na == nb)
        return authorUnknown(a) || authorUnknown(b) || authorsCompatible(a.author, b.author);

    // Same name once subtitles are dropped ("All the Mods 9 - ATM9" vs "All the Mods 9"):
    // only merge when both authors are known and agree, since generic base names collide easily.
    const QString ba = baseName(a.name);
    const QString bb = baseName(b.name);
    if (!ba.isEmpty() && ba == bb)
        return authorsCompatible(a.author, b.author);

    return false;
}

double textRelevance(const QString& query, const QString& name)
{
    const QString q = normalizeName(query);
    const QString n = normalizeName(name);
    if (q.isEmpty() || n.isEmpty())
        return 0;
    if (q == n)
        return 1.0;
    if (n.startsWith(q))
        return 0.85;
    if (n.contains(q))
        return 0.7;

    const QStringList queryTokens = tokens(query);
    if (queryTokens.isEmpty())
        return 0;
    const QStringList nameTokenList = tokens(name);
    const QSet<QString> nameTokens(nameTokenList.begin(), nameTokenList.end());
    int matched = 0;
    for (const auto& t : queryTokens) {
        bool found = nameTokens.contains(t);
        for (auto it = nameTokens.begin(); !found && it != nameTokens.end(); ++it)
            found = t.size() >= 3 && it->startsWith(t);
        if (found)
            matched++;
    }
    return 0.6 * matched / static_cast<double>(queryTokens.size());
}

namespace {
int sourcePriority(Source source)
{
    return static_cast<int>(allSources().indexOf(source));
}

void fillPackDetails(Pack& pack)
{
    std::sort(pack.hits.begin(), pack.hits.end(),
              [](const SourceHit& a, const SourceHit& b) { return sourcePriority(a.source) < sourcePriority(b.source); });

    pack.downloads = 0;
    for (const auto& h : pack.hits) {
        if (h.downloads > 0)
            pack.downloads += h.downloads;
        // Prefer data from the higher-priority source, fall back to whatever the others have.
        if (pack.name.isEmpty())
            pack.name = h.name;
        if (pack.author.isEmpty())
            pack.author = h.author;
        if (pack.summary.isEmpty())
            pack.summary = h.summary;
        if (pack.iconUrl.isEmpty())
            pack.iconUrl = h.iconUrl;
    }
}
}  // namespace

QList<Pack> mergeHits(const QList<SourceHit>& hits)
{
    // Process hits in a stable order (source priority, then rank) so the result doesn't depend on
    // which source answered first.
    QList<SourceHit> ordered = hits;
    std::stable_sort(ordered.begin(), ordered.end(), [](const SourceHit& a, const SourceHit& b) {
        if (a.source != b.source)
            return sourcePriority(a.source) < sourcePriority(b.source);
        return a.rank < b.rank;
    });

    QList<Pack> packs;
    for (const auto& hit : ordered) {
        Pack* target = nullptr;
        for (auto& pack : packs) {
            if (pack.hasSource(hit.source))
                continue;  // a pack never holds two entries from the same source
            const bool matches = std::any_of(pack.hits.begin(), pack.hits.end(), [&hit](const SourceHit& h) { return isSamePack(h, hit); });
            if (matches) {
                target = &pack;
                break;
            }
        }
        if (!target) {
            packs.append(Pack{});
            target = &packs.last();
        }
        target->hits.append(hit);
    }

    for (auto& pack : packs)
        fillPackDetails(pack);
    return packs;
}

void rankPacks(QList<Pack>& packs, const QString& query)
{
    const bool browsing = normalizeName(query).isEmpty();

    for (auto& pack : packs) {
        if (browsing) {
            pack.relevance = 0;
            continue;
        }
        double text = 0;
        for (const auto& h : pack.hits)
            text = std::max(text, textRelevance(query, h.name));
        // Each source already ranks by relevance (including descriptions/tags we don't see); trust its top results.
        const double sourceRank = 1.0 / (1.0 + pack.bestRank() / 4.0);
        const double multiSource = 0.05 * static_cast<double>(pack.hits.size() - 1);
        pack.relevance = std::min(1.0, 0.7 * text + 0.3 * sourceRank + multiSource);
        // An exact name match is what the user typed; keep it at the top tier whatever the sources say.
        if (text >= 1.0)
            pack.relevance = std::max(pack.relevance, 0.95);
    }

    // Relevance is bucketed into tiers of 0.1, then popularity decides within a tier.
    auto tier = [](double relevance) { return static_cast<int>(std::floor(relevance * 10.0 + 1e-9)); };
    std::stable_sort(packs.begin(), packs.end(), [&tier](const Pack& a, const Pack& b) {
        if (tier(a.relevance) != tier(b.relevance))
            return tier(a.relevance) > tier(b.relevance);
        if (a.downloads != b.downloads)
            return a.downloads > b.downloads;
        if (a.bestRank() != b.bestRank())
            return a.bestRank() < b.bestRank();
        return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
    });
}

}  // namespace Unified
