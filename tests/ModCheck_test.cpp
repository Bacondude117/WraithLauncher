#include <QTest>

#include <creator/ModCheck.h>
#include <creator/TextDiff.h>

using namespace Wraith::Creator;

namespace {
ModInfo mod(const QString& id, const QString& version, const QString& loader = "fabric")
{
    ModInfo m;
    m.id = id;
    m.name = id;
    m.version = version;
    m.loader = loader;
    m.file = id + "-" + version + ".jar";
    return m;
}

bool hasIssue(const QList<Issue>& issues, const QString& text, Issue::Severity severity)
{
    for (const auto& i : issues)
        if (i.title.contains(text) && i.severity == severity)
            return true;
    return false;
}
}  // namespace

class ModCheckTest : public QObject {
    Q_OBJECT

   private slots:
    void versionRanges()
    {
        QVERIFY(versionSatisfies("0.5.8", ">=0.5"));
        QVERIFY(!versionSatisfies("0.4.9", ">=0.5"));
        QVERIFY(versionSatisfies("1.20.1", "1.20.x"));
        QVERIFY(!versionSatisfies("1.21", "1.20.x"));
        QVERIFY(versionSatisfies("1.20.1", "~1.20"));
        QVERIFY(versionSatisfies("1.20.4", ">=1.20 <1.21"));
        QVERIFY(!versionSatisfies("1.21.1", ">=1.20 <1.21"));
        QVERIFY(versionSatisfies("1.21.1", "1.20.1 || 1.21.1"));
        QVERIFY(versionSatisfies("47.2.0", "[47,)"));
        QVERIFY(!versionSatisfies("46.0.1", "[47,)"));
        QVERIFY(versionSatisfies("1.20.1", "[1.20,1.21)"));
        QVERIFY(!versionSatisfies("1.21", "[1.20,1.21)"));
        QVERIFY(versionSatisfies("anything", "*"));
        QVERIFY(compareVersions("1.0.10", "1.0.9") > 0);
        QVERIFY(compareVersions("1.0-beta", "1.0") < 0);
        QVERIFY(compareVersions("0.5.8+mc1.20.1", "0.5.8") == 0);
    }

    void missingAndDisabledDependencies()
    {
        auto sodiumExtra = mod("sodium-extra", "0.5");
        sodiumExtra.depends.insert("sodium", ">=0.5");
        sodiumExtra.depends.insert("fabricloader", ">=0.15");  // platform id, never "missing"
        auto modmenuUser = mod("needs-modmenu", "1.0");
        modmenuUser.depends.insert("modmenu", "");
        auto modmenu = mod("modmenu", "7.2");
        modmenu.enabled = false;
        const auto issues = checkMods({ sodiumExtra, modmenuUser, modmenu }, "fabric", "1.20.1");
        QVERIFY(hasIssue(issues, "sodium-extra needs sodium", Issue::Severity::Error));
        QVERIFY(hasIssue(issues, "needs modmenu, which is disabled", Issue::Severity::Error));
        QCOMPARE(issues.size(), 2);
    }

    void providedIdsAndVersions()
    {
        auto api = mod("fabric-api", "0.92.0");
        api.provides = { "fabric", "fabric-resource-loader-v0" };  // bundled modules
        auto user = mod("user", "1.0");
        user.depends.insert("fabric-resource-loader-v0", "");
        user.depends.insert("fabric-api", ">=0.95");
        const auto issues = checkMods({ api, user }, "fabric", "1.20.1");
        QCOMPARE(issues.size(), 1);
        QVERIFY(hasIssue(issues, "needs a different version of fabric-api", Issue::Severity::Warning));
    }

    void conflictsDuplicatesAndLoaders()
    {
        auto optifabric = mod("optifabric", "1.0");
        optifabric.breaks.insert("sodium", "");
        auto sodium = mod("sodium", "0.5.8");
        auto sodiumOld = mod("sodium", "0.4.0");
        auto forgeMod = mod("create", "0.5", "forge");
        auto wrongMc = mod("oldmod", "1.0");
        wrongMc.depends.insert("minecraft", "1.19.x");
        const auto issues = checkMods({ optifabric, sodium, sodiumOld, forgeMod, wrongMc }, "fabric", "1.20.1");
        QVERIFY(hasIssue(issues, "optifabric doesn't work with sodium", Issue::Severity::Error));
        QVERIFY(hasIssue(issues, "sodium is installed more than once", Issue::Severity::Error));
        QVERIFY(hasIssue(issues, "create is made for forge", Issue::Severity::Error));
        QVERIFY(hasIssue(issues, "oldmod asks for a different Minecraft version", Issue::Severity::Warning));
        // errors are listed first
        QCOMPARE(issues.first().severity, Issue::Severity::Error);
    }

    void quiltRunsFabricMods()
    {
        auto fabricMod = mod("lithium", "0.11");
        fabricMod.depends.insert("fabric-api", "*");
        const auto issues = checkMods({ fabricMod }, "quilt", "1.20.1");
        QVERIFY(issues.isEmpty());
    }

    void unifiedDiffShowsChangedLines()
    {
        const QString before = "a=1\nb=2\nc=3\nd=4\ne=5\nf=6\ng=7\nh=8\n";
        const QString after = "a=1\nb=2\nc=3\nd=40\ne=5\nf=6\ng=7\nh=8\ni=9\n";
        const QString diff = unifiedDiff(before, after, "config/x.properties");
        QVERIFY(diff.contains("-d=4\n+d=40\n"));
        QVERIFY(diff.contains("+i=9"));
        QVERIFY(diff.contains("@@ -1,"));
        QVERIFY(unifiedDiff(before, before, "x").contains("(no differences)"));
        QVERIFY(looksLikeText("hello"));
        QVERIFY(!looksLikeText(QByteArray("a\0b", 3)));
    }
};

QTEST_GUILESS_MAIN(ModCheckTest)

#include "ModCheck_test.moc"
