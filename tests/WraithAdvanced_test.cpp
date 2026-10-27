#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <advanced/CrashAnalyzer.h>
#include <advanced/JvmPresets.h>
#include <advanced/LauncherImport.h>
#include <advanced/SharedFolders.h>

using namespace Wraith::Advanced;

namespace {
void write(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data);
}

bool hasFinding(const QList<CrashFinding>& findings, const QString& id)
{
    for (const auto& f : findings)
        if (f.id == id)
            return true;
    return false;
}
}  // namespace

// Advanced user tools and player comforts: pure logic behind presets, crash analysis, imports and shared folders.
class WraithAdvancedTest : public QObject {
    Q_OBJECT

   private slots:
    void presetsRoundTrip()
    {
        const auto presets = jvmPresets();
        QVERIFY(presets.size() >= 3);
        for (const auto& p : presets) {
            QCOMPARE(presetIdFor("  " + p.args + "  "), p.id);
            QVERIFY(!p.explanation.isEmpty());
        }
        QCOMPARE(presetIdFor("-Xss2M"), QString());  // someone's own arguments are "custom"
        QCOMPARE(presetIdFor(""), QString());
        for (const auto& p : presets)
            if (p.id == "zgc")
                QVERIFY(!presetProblem(p, 17).isEmpty() && presetProblem(p, 21).isEmpty());
    }

    void memoryAdvice()
    {
        QCOMPARE(recommendMemory(0, "1.20.1", 32768, 2048).recommendedMiB, 3072);
        QCOMPARE(recommendMemory(200, "1.20.1", 32768, 8192).recommendedMiB, 8192);
        // limited by a small computer
        const auto small = recommendMemory(300, "1.20.1", 8192, 4096);
        QVERIFY(small.recommendedMiB <= 4096);
        QVERIFY(small.reason.contains("RAM"));
        QVERIFY(!recommendMemory(200, "1.20.1", 32768, 2048).warning.isEmpty());   // too low
        QVERIFY(!recommendMemory(10, "1.20.1", 16384, 15000).warning.isEmpty());   // most of the RAM
    }

    void logLevels()
    {
        QCOMPARE(lineLevel("[12:00:01] [Render thread/WARN]: careful"), LogLevel::Warn);
        QCOMPARE(lineLevel("[12:00:01] [main/ERROR]: broken"), LogLevel::Error);
        QCOMPARE(lineLevel("[12:00:01 INFO]: hello"), LogLevel::Info);
        QCOMPARE(lineLevel("\tat net.minecraft.Foo.bar(Foo.java:1)", LogLevel::Error), LogLevel::Error);
        QCOMPARE(lineLevel("java.lang.NullPointerException: x"), LogLevel::Error);
        QCOMPARE(lineLevel("plain text"), LogLevel::Unknown);
    }

    void crashCauses()
    {
        auto f = analyzeCrash("Exception in thread \"main\" java.lang.UnsupportedClassVersionError: x has been compiled by a more recent "
                              "version of the Java Runtime (class file version 65.0), this version of the Java Runtime only recognizes "
                              "class file versions up to 61.0");
        QVERIFY(hasFinding(f, "java-too-old"));
        QVERIFY(f.first().title.contains("21") && f.first().title.contains("17"));
        QVERIFY(f.first().java);

        f = analyzeCrash("java.lang.OutOfMemoryError: Java heap space");
        QVERIFY(hasFinding(f, "out-of-memory"));
        QVERIFY(f.first().memory);

        f = analyzeCrash("net.fabricmc.loader.impl.FormattedException: Mod resolution failed\n"
                         " - Mod 'Sodium Extra' (sodium-extra) 0.5 requires any version of 'sodium', which is missing!");
        QVERIFY(hasFinding(f, "dependencies"));
        QVERIFY(f.first().mods.contains("sodium"));

        f = analyzeCrash("org.spongepowered.asm.mixin.transformer.throwables.MixinTransformerError: An unexpected critical error\n"
                         "Caused by: Mixin apply for mod coolmod failed coolmod.mixins.json:FooMixin");
        QVERIFY(hasFinding(f, "mixin"));
        QVERIFY(f.first().mods.contains("coolmod"));

        f = analyzeCrash("---- Minecraft Crash Report ----\nDescription: Ticking entity\n\njava.lang.IllegalStateException: boom\n");
        QVERIFY(hasFinding(f, "generic"));
        QVERIFY(analyzeCrash("all good").isEmpty());
    }

    void scansOtherLaunchers()
    {
        QTemporaryDir dir;
        // Prism-format
        write(dir.filePath("prism/instances/Pack/instance.cfg"), "[General]\nname=My Pack\n");
        write(dir.filePath("prism/instances/Pack/mmc-pack.json"),
              R"({"components":[{"uid":"net.minecraft","version":"1.20.1"},{"uid":"net.fabricmc.fabric-loader","version":"0.15.7"}]})");
        auto prism = scanLauncher({ "Prism Launcher", dir.filePath("prism") });
        QCOMPARE(prism.size(), 1);
        QCOMPARE(prism.first().name, QString("My Pack"));
        QVERIFY(prism.first().prismFormat);
        QCOMPARE(prism.first().loader, QString("fabric"));
        QCOMPARE(prism.first().minecraft, QString("1.20.1"));

        // CurseForge
        write(dir.filePath("cf/Instances/All the Mods/minecraftinstance.json"),
              R"({"name":"All the Mods 9","gameVersion":"1.20.1","baseModLoader":{"name":"forge-47.2.0"}})");
        auto cf = scanLauncher({ "CurseForge", dir.filePath("cf") });
        QCOMPARE(cf.size(), 1);
        QCOMPARE(cf.first().loader, QString("forge"));
        QCOMPARE(cf.first().loaderVersion, QString("47.2.0"));

        // official launcher: fixed-version profiles only
        write(dir.filePath("mc/launcher_profiles.json"),
              R"({"profiles":{"a":{"name":"Fabric","lastVersionId":"fabric-loader-0.15.7-1.20.1"},"b":{"name":"Latest","lastVersionId":"latest-release"},"c":{"name":"Old","lastVersionId":"1.12.2"}}})");
        auto official = scanLauncher({ "Minecraft Launcher", dir.filePath("mc") });
        QCOMPARE(official.size(), 2);
        bool sawFabric = false;
        for (const auto& i : official)
            if (i.loader == "fabric")
                sawFabric = i.minecraft == "1.20.1" && i.loaderVersion == "0.15.7";
        QVERIFY(sawFabric);
    }

    void sharingNeverOverwrites()
    {
        QTemporaryDir dir;
        const QString game = dir.filePath("instance/minecraft");
        const QString shared = dir.filePath("shared");
        write(game + "/resourcepacks/mine.zip", "mine");
        write(game + "/resourcepacks/both.zip", "instance copy");
        write(shared + "/resourcepacks/both.zip", "shared copy");

        auto result = SharedFolders::share(game, "resourcepacks", shared);
        QVERIFY2(result.has_value(), qPrintable(result.has_value() ? QString() : result.error()));
        QVERIFY(SharedFolders::isShared(game, "resourcepacks", shared));
        QVERIFY(QFile::exists(shared + "/resourcepacks/mine.zip"));                 // moved
        QFile both(shared + "/resourcepacks/both.zip");
        QVERIFY(both.open(QIODevice::ReadOnly));
        QCOMPARE(both.readAll(), QByteArray("shared copy"));                         // not overwritten
        QVERIFY(QFile::exists(game + "/resourcepacks.before-sharing/both.zip"));      // kept aside
        QVERIFY(QFile::exists(game + "/resourcepacks/mine.zip"));                     // visible through the link

        QVERIFY(SharedFolders::unshare(game, "resourcepacks").has_value());
        QVERIFY(!SharedFolders::isShared(game, "resourcepacks", shared));
        QVERIFY(QFile::exists(game + "/resourcepacks/mine.zip"));    // the instance got a copy
        QVERIFY(QFile::exists(shared + "/resourcepacks/mine.zip"));  // the shared folder kept its files
        QVERIFY(!SharedFolders::share(game, "saves", shared).has_value());  // worlds are never shared
    }
};

QTEST_GUILESS_MAIN(WraithAdvancedTest)

#include "WraithAdvanced_test.moc"
