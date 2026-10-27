#include <QTest>

#include <modplatform/unified/UnifiedPack.h>

using namespace Unified;

namespace {
SourceHit hit(Source source, const QString& id, const QString& name, const QString& author, qint64 downloads = -1, int rank = 0)
{
    SourceHit h;
    h.source = source;
    h.id = id;
    h.name = name;
    h.author = author;
    h.downloads = downloads;
    h.rank = rank;
    return h;
}
}  // namespace

class UnifiedPackTest : public QObject {
    Q_OBJECT

   private slots:
    void normalize_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("normalized");
        QTest::addColumn<QString>("base");

        QTest::newRow("plain") << "Better MC" << "bettermc" << "bettermc";
        QTest::newRow("brackets") << "Better MC [FABRIC] (1.20.1)" << "bettermc" << "bettermc";
        QTest::newRow("subtitle dash") << "All the Mods 9 - ATM9" << "allthemods9atm9" << "allthemods9";
        QTest::newRow("subtitle colon") << "Create: Above and Beyond" << "createaboveandbeyond" << "create";
        QTest::newRow("accents") << "Pokémon Café" << "pokemoncafe" << "pokemoncafe";
        QTest::newRow("hyphenated word stays") << "Sky-Factory 4" << "skyfactory4" << "skyfactory4";
    }

    void normalize()
    {
        QFETCH(QString, input);
        QFETCH(QString, normalized);
        QFETCH(QString, base);
        QCOMPARE(normalizeName(input), normalized);
        QCOMPARE(baseName(input), base);
    }

    void samePackRules()
    {
        // identical names, compatible authors
        QVERIFY(isSamePack(hit(Source::Modrinth, "a", "Fabulously Optimized", "robotkoer"),
                           hit(Source::CurseForge, "1", "Fabulously Optimized", "Robotkoer")));
        // identical names, one author unknown (Technic search has no authors)
        QVERIFY(isSamePack(hit(Source::Technic, "t", "Tekkit", ""), hit(Source::ATLauncher, "x", "Tekkit", "Tekkit Team")));
        // identical names, clearly different authors: different packs
        QVERIFY(!isSamePack(hit(Source::Modrinth, "a", "Skyblock", "alice"), hit(Source::CurseForge, "1", "Skyblock", "bob")));
        // subtitle dropped, authors agree
        QVERIFY(isSamePack(hit(Source::CurseForge, "1", "All the Mods 9 - ATM9", "ATMTeam"),
                           hit(Source::FTB, "f", "All the Mods 9", "ATM Team")));
        // subtitle dropped but author unknown: too risky
        QVERIFY(!isSamePack(hit(Source::CurseForge, "1", "Skyblock - Ocean", "x"), hit(Source::Technic, "t", "Skyblock", "")));
        // same source: only the same id is the same pack
        QVERIFY(!isSamePack(hit(Source::Modrinth, "a", "Same", "x"), hit(Source::Modrinth, "b", "Same", "x")));
    }

    void mergeCombinesSources()
    {
        const QList<SourceHit> hits = {
            hit(Source::CurseForge, "1", "Fabulously Optimized", "robotkoer", 5000, 0),
            hit(Source::Modrinth, "fo", "Fabulously Optimized", "robotkoer", 7000, 1),
            hit(Source::Modrinth, "other", "Other Pack", "someone", 10, 0),
        };
        auto packs = mergeHits(hits);
        QCOMPARE(packs.size(), 2);

        const Pack* fo = nullptr;
        for (const auto& p : packs) {
            if (p.name == "Fabulously Optimized")
                fo = &p;
        }
        QVERIFY(fo);
        QCOMPARE(fo->hits.size(), 2);
        QVERIFY(fo->hasSource(Source::Modrinth));
        QVERIFY(fo->hasSource(Source::CurseForge));
        QCOMPARE(fo->downloads, qint64(12000));
        QCOMPARE(fo->bestRank(), 0);
        // hits are ordered by source priority: Modrinth first
        QCOMPARE(fo->hits.first().source, Source::Modrinth);
    }

    void mergeNeverDuplicatesASource()
    {
        // Two different Modrinth packs with the same name must not end up in one card.
        const QList<SourceHit> hits = {
            hit(Source::Modrinth, "a", "Vanilla Plus", ""),
            hit(Source::Modrinth, "b", "Vanilla Plus", ""),
            hit(Source::CurseForge, "1", "Vanilla Plus", ""),
        };
        auto packs = mergeHits(hits);
        QCOMPARE(packs.size(), 2);
        for (const auto& p : packs) {
            int modrinthCount = 0;
            for (const auto& h : p.hits)
                modrinthCount += h.source == Source::Modrinth;
            QVERIFY(modrinthCount <= 1);
        }
    }

    void mergeIsOrderIndependent()
    {
        QList<SourceHit> hits = {
            hit(Source::Technic, "t", "Tekkit", "", -1, 0),
            hit(Source::ATLauncher, "x", "Tekkit", "Tekkit Team", -1, 2),
            hit(Source::Modrinth, "m", "Unrelated", "z", 1, 0),
        };
        auto first = mergeHits(hits);
        std::reverse(hits.begin(), hits.end());
        auto second = mergeHits(hits);
        QCOMPARE(first.size(), second.size());
        for (int i = 0; i < first.size(); i++) {
            QCOMPARE(first[i].name, second[i].name);
            QCOMPARE(first[i].hits.size(), second[i].hits.size());
        }
    }

    void textRelevanceOrder()
    {
        QVERIFY(textRelevance("tekkit", "Tekkit") > textRelevance("tekkit", "Tekkit Legends"));
        QVERIFY(textRelevance("tekkit", "Tekkit Legends") > textRelevance("tekkit", "Legends of Tekkit"));
        QVERIFY(textRelevance("tekkit", "Legends of Tekkit") > textRelevance("tekkit", "Something Else"));
        QVERIFY(textRelevance("sky factory", "SkyFactory 4") > 0.5);
        QCOMPARE(textRelevance("", "Anything"), 0.0);
    }

    void rankRelevanceBeforePopularity()
    {
        auto packs = mergeHits({
            hit(Source::Modrinth, "a", "Popular Unrelated Pack", "x", 9'000'000, 5),
            hit(Source::Modrinth, "b", "Tekkit", "y", 100, 0),
        });
        rankPacks(packs, "tekkit");
        QCOMPARE(packs.first().name, QString("Tekkit"));
    }

    void rankExactMatchFirst()
    {
        // The source lists "Tekkit Legends" before "Tekkit"; the exact match must still win.
        auto packs = mergeHits({
            hit(Source::Technic, "tekkit-legends", "Tekkit Legends", "", -1, 0),
            hit(Source::Technic, "tekkitmain", "Tekkit", "", -1, 1),
        });
        rankPacks(packs, "tekkit");
        QCOMPARE(packs.first().name, QString("Tekkit"));
    }

    void rankPopularityWithinSameRelevance()
    {
        auto packs = mergeHits({
            hit(Source::Modrinth, "a", "Tekkit Small", "x", 100, 0),
            hit(Source::CurseForge, "b", "Tekkit Big", "y", 1'000'000, 0),
        });
        rankPacks(packs, "tekkit");
        QCOMPARE(packs.first().name, QString("Tekkit Big"));
    }

    void rankBrowsingByPopularity()
    {
        auto packs = mergeHits({
            hit(Source::Modrinth, "a", "Small", "x", 10, 0),
            hit(Source::Modrinth, "b", "Large", "y", 10'000, 1),
        });
        rankPacks(packs, "");
        QCOMPARE(packs.first().name, QString("Large"));
    }
};

QTEST_GUILESS_MAIN(UnifiedPackTest)

#include "UnifiedPack_test.moc"
