#include <QTemporaryDir>
#include <QTest>

#include <modplatform/packwiz/Packwiz.h>
#include <server/ServerModFilter.h>

using namespace WraithServer;

class ServerModFilterTest : public QObject {
    Q_OBJECT

   private slots:
    void knownClientOnly_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<bool>("clientOnly");

        QTest::newRow("sodium fabric") << "sodium-fabric-0.5.8+mc1.20.1.jar" << true;
        QTest::newRow("Sodium caps") << "Sodium-Fabric-0.5.8.jar" << true;
        QTest::newRow("iris") << "iris-mc1.20.1-1.7.0.jar" << true;
        QTest::newRow("modmenu") << "modmenu-7.2.2.jar" << true;
        QTest::newRow("optifine") << "OptiFine_1.20.1_HD_U_I6.jar" << true;
        QTest::newRow("disabled suffix") << "embeddium-0.3.18.jar.disabled" << true;
        QTest::newRow("sodium extra") << "sodium-extra-0.5.4+mc1.20.1.jar" << true;
        // gameplay mods with similar-looking names must not match
        QTest::newRow("create") << "create-1.20.1-0.5.1.f.jar" << false;
        QTest::newRow("jei is both") << "jei-1.20.1-forge-15.2.0.27.jar" << false;
        QTest::newRow("irons spellbooks") << "irons_spellbooks-1.20.1-3.4.0.jar" << false;
        QTest::newRow("sodium-dependent addon") << "sodiumdynamiclights-1.0.jar" << false;
    }

    void knownClientOnly()
    {
        QFETCH(QString, file);
        QFETCH(bool, clientOnly);
        QCOMPARE(isKnownClientOnly(file), clientOnly);
    }

    void classifyPriorities()
    {
        // metadata wins over the built-in list
        auto e = classify("sodium-0.5.jar", "Sodium", ModPlatform::SideType(ModPlatform::SideType::UniversalSide));
        QVERIFY(e.include);
        QCOMPARE(e.reason, ModReason::MetadataBoth);

        e = classify("somemod-1.0.jar", "Some Mod", ModPlatform::SideType(ModPlatform::SideType::ClientSide));
        QVERIFY(!e.include);
        QCOMPARE(e.reason, ModReason::MetadataClientOnly);

        e = classify("servermod-1.0.jar", "", ModPlatform::SideType(ModPlatform::SideType::ServerSide));
        QVERIFY(e.include);
        QCOMPARE(e.name, QString("servermod-1.0.jar"));

        // no metadata: known list, else include
        e = classify("modmenu-7.jar", "", std::nullopt);
        QVERIFY(!e.include);
        QCOMPARE(e.reason, ModReason::KnownClientOnly);
        e = classify("unknown-1.0.jar", "", std::nullopt);
        QVERIFY(e.include);
        QCOMPARE(e.reason, ModReason::NoInformation);

        // user overrides win over everything
        e = classify("modmenu-7.jar", "", std::nullopt, { "modmenu-7.jar" }, {});
        QVERIFY(e.include);
        QCOMPARE(e.reason, ModReason::UserIncluded);
        e = classify("create.jar", "", ModPlatform::SideType(ModPlatform::SideType::UniversalSide), {}, { "create.jar" });
        QVERIFY(!e.include);
        QCOMPARE(e.reason, ModReason::UserExcluded);
    }

    void classifyFolderUsesIndex()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QDir mods(tmp.path());
        QVERIFY(mods.mkpath(".index"));
        for (const auto& f : { "client-thing-1.0.jar", "gameplay-2.0.jar", "modmenu-7.2.jar", "plain-1.0.jar" }) {
            QFile file(mods.filePath(f));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        {
            QFile file(mods.filePath("old.jar.disabled"));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }

        QDir indexDir(mods.filePath(".index"));
        Packwiz::V1::Mod clientMod;
        clientMod.slug = "client-thing";
        clientMod.name = "Client Thing";
        clientMod.filename = "client-thing-1.0.jar";
        clientMod.side = ModPlatform::SideType::ClientSide;
        clientMod.provider = ModPlatform::ResourceProvider::MODRINTH;
        clientMod.projectId = "abc";
        clientMod.fileId = "def";
        clientMod.mode = "url";
        clientMod.url = QUrl("https://example.invalid/client-thing-1.0.jar");
        Packwiz::V1::updateModIndex(indexDir, clientMod);

        Packwiz::V1::Mod bothMod = clientMod;
        bothMod.slug = "gameplay";
        bothMod.name = "Gameplay";
        bothMod.filename = "gameplay-2.0.jar";
        bothMod.side = ModPlatform::SideType::UniversalSide;
        bothMod.url = QUrl("https://example.invalid/gameplay-2.0.jar");
        Packwiz::V1::updateModIndex(indexDir, bothMod);

        const auto entries = classifyModsFolder(mods);
        QCOMPARE(entries.size(), 4);  // .disabled files are not part of the server
        QHash<QString, ModEntry> byFile;
        for (const auto& e : entries)
            byFile.insert(e.fileName, e);

        QCOMPARE(byFile["client-thing-1.0.jar"].reason, ModReason::MetadataClientOnly);
        QCOMPARE(byFile["client-thing-1.0.jar"].name, QString("Client Thing"));
        QVERIFY(!byFile["client-thing-1.0.jar"].include);
        QCOMPARE(byFile["gameplay-2.0.jar"].reason, ModReason::MetadataBoth);
        QCOMPARE(byFile["modmenu-7.2.jar"].reason, ModReason::KnownClientOnly);
        QCOMPARE(byFile["plain-1.0.jar"].reason, ModReason::NoInformation);
        QVERIFY(byFile["plain-1.0.jar"].include);
    }
};

QTEST_GUILESS_MAIN(ServerModFilterTest)

#include "ServerModFilter_test.moc"
