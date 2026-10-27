#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <snapshots/SnapshotDiff.h>
#include <snapshots/SnapshotStore.h>

using namespace Wraith::Snapshots;

namespace {
void write(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data);
}

QByteArray read(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

int objectCount(const Store& store)
{
    int n = 0;
    QDirIterator it(QDir(store.storeDir()).filePath("objects"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        n++;
    }
    return n;
}

// A small instance: instance.cfg, mmc-pack.json, mods, config, a world and a log
void makeInstance(const QString& root)
{
    write(root + "/instance.cfg", "[General]\nname=Test\ntotalTimePlayed=10\n");
    write(root + "/mmc-pack.json", R"({"components":[{"uid":"net.minecraft","version":"1.20.1"}]})");
    write(root + "/.minecraft/mods/sodium-fabric-0.5.8+mc1.20.1.jar", "sodium 0.5.8");
    write(root + "/.minecraft/mods/lithium-fabric-0.11.2.jar", "lithium");
    write(root + "/.minecraft/config/sodium.json", "{\"a\":1}");
    write(root + "/.minecraft/saves/World/level.dat", "world");
    write(root + "/.minecraft/logs/latest.log", "log");
}
}  // namespace

class SnapshotTest : public QObject {
    Q_OBJECT

   private slots:
    void scopeLeavesOutWorldsLogsAndWraithFolders()
    {
        QTemporaryDir dir;
        Store store(dir.path(), dir.filePath(".minecraft"));
        QVERIFY(store.inScope("instance.cfg", false));
        QVERIFY(store.inScope(".minecraft/mods/a.jar", false));
        QVERIFY(store.inScope(".minecraft/options.txt", false));
        QVERIFY(!store.inScope(".minecraft/saves/World/level.dat", false));
        QVERIFY(store.inScope(".minecraft/saves/World/level.dat", true));
        QVERIFY(!store.inScope(".minecraft/logs/latest.log", true));
        QVERIFY(!store.inScope(".wraith/snapshots/x.json", true));
        QVERIFY(!store.inScope("wraith-server/server.jar", true));
    }

    void createListsFilesAndDeduplicates()
    {
        QTemporaryDir dir;
        makeInstance(dir.path());
        Store store(dir.path(), dir.filePath(".minecraft"));
        auto first = store.create({ "first", "Manual", false, false });
        QVERIFY(first.has_value());
        QCOMPARE(first->files.size(), 5);  // cfg, pack, 2 mods, config (no world, no log)
        QVERIFY(!first->files.contains(".minecraft/saves/World/level.dat"));
        const int objects = objectCount(store);
        QCOMPARE(objects, 5);

        // nothing changed: a second snapshot stores no new contents
        auto second = store.create({ "second", "Manual", false, false });
        QVERIFY(second.has_value());
        QCOMPARE(objectCount(store), objects);
        QCOMPARE(store.list().size(), 2);
        QCOMPARE(store.list().first().label, QString("second"));  // newest first

        auto withWorlds = store.create({ "worlds", "Manual", false, true });
        QVERIFY(withWorlds->files.contains(".minecraft/saves/World/level.dat"));
    }

    void restoreBringsBackFilesButKeepsInstanceCfg()
    {
        QTemporaryDir dir;
        makeInstance(dir.path());
        Store store(dir.path(), dir.filePath(".minecraft"));
        auto snapshot = store.create({ "before update", "Manual", false, false });
        QVERIFY(snapshot.has_value());

        // an "update": sodium replaced, lithium removed, a new mod, a config edit, playtime grows, world changes
        QFile::remove(dir.filePath(".minecraft/mods/sodium-fabric-0.5.8+mc1.20.1.jar"));
        write(dir.filePath(".minecraft/mods/sodium-fabric-0.6.5+mc1.21.1.jar"), "sodium 0.6.5");
        QFile::remove(dir.filePath(".minecraft/mods/lithium-fabric-0.11.2.jar"));
        write(dir.filePath(".minecraft/mods/iris-1.7.jar"), "iris");
        write(dir.filePath(".minecraft/config/sodium.json"), "{\"a\":2}");
        write(dir.filePath("instance.cfg"), "[General]\nname=Test\ntotalTimePlayed=99\n");
        write(dir.filePath(".minecraft/saves/World/level.dat"), "world changed");

        const Diff preview = store.restorePreview(*snapshot);
        QCOMPARE(preview.added.size(), 2);    // sodium 0.5.8, lithium come back
        QCOMPARE(preview.removed.size(), 2);  // sodium 0.6.5, iris go
        QCOMPARE(preview.changed, QStringList{ ".minecraft/config/sodium.json" });

        QVERIFY(store.restore(*snapshot).has_value());
        QCOMPARE(read(dir.filePath(".minecraft/mods/sodium-fabric-0.5.8+mc1.20.1.jar")), QByteArray("sodium 0.5.8"));
        QVERIFY(QFile::exists(dir.filePath(".minecraft/mods/lithium-fabric-0.11.2.jar")));
        QVERIFY(!QFile::exists(dir.filePath(".minecraft/mods/iris-1.7.jar")));
        QCOMPARE(read(dir.filePath(".minecraft/config/sodium.json")), QByteArray("{\"a\":1}"));
        // never rolled back: playtime/settings, and worlds (not in this snapshot)
        QVERIFY(read(dir.filePath("instance.cfg")).contains("totalTimePlayed=99"));
        QCOMPARE(read(dir.filePath(".minecraft/saves/World/level.dat")), QByteArray("world changed"));
        QVERIFY(store.restorePreview(*snapshot).isEmpty());
    }

    void pruneKeepsPinnedAndNewest()
    {
        QTemporaryDir dir;
        makeInstance(dir.path());
        Store store(dir.path(), dir.filePath(".minecraft"));
        QStringList ids;
        for (int i = 0; i < 5; i++) {
            write(dir.filePath(".minecraft/config/sodium.json"), QByteArray::number(i));
            auto s = store.create({ QString::number(i), "Manual", true, false });
            QVERIFY(s.has_value());
            ids.append(s->id);
            QTest::qWait(5);  // distinct ids
        }
        QVERIFY(store.setPinned(ids[0], true));
        const auto removed = store.prune(2);
        QCOMPARE(removed.size(), 2);  // 4 unpinned, keep 2 newest
        QVERIFY(store.get(ids[0]).has_value());  // pinned, oldest, kept
        QVERIFY(!store.get(ids[1]).has_value());
        QVERIFY(store.get(ids[4]).has_value());
        // contents only the pruned snapshots used are gone; everything still listed can be restored
        for (const auto& s : store.list())
            for (const auto& f : s.files)
                QVERIFY(QFile::exists(QDir(store.storeDir()).filePath(QString("objects/%1/%2").arg(f.hash.left(2), f.hash))));
        QCOMPARE(store.prune(0).size(), 0);  // 0 = keep everything
    }

    void exportFolderMatchesSnapshot()
    {
        QTemporaryDir dir;
        QTemporaryDir out;
        makeInstance(dir.path());
        Store store(dir.path(), dir.filePath(".minecraft"));
        auto s = store.create({});
        QVERIFY(store.exportToFolder(*s, out.filePath("export")).has_value());
        QCOMPARE(read(out.filePath("export/.minecraft/mods/lithium-fabric-0.11.2.jar")), QByteArray("lithium"));
        QVERIFY(!store.exportToFolder(*s, out.filePath("export")).has_value());  // never overwrites
    }

    void modStemsPairVersions()
    {
        QCOMPARE(modStem("sodium-fabric-0.5.8+mc1.20.1.jar"), QString("sodium-fabric"));
        QCOMPARE(modStem("fabric-api-0.92.2+1.20.1.jar"), QString("fabric-api"));
        QCOMPARE(modStem("Xaeros_Minimap_24.0_Fabric_1.20.jar"), QString("xaeros-minimap"));
        QCOMPARE(modStem("iris-mc1.21.1-1.8.1.jar.disabled"), QString("iris"));
        QCOMPARE(displayName(".minecraft/mods/iris-1.7.jar.disabled"), QString("iris-1.7"));
    }

    void modChangesAndChangelog()
    {
        Diff d;
        d.removed = { ".minecraft/mods/sodium-fabric-0.5.8.jar", ".minecraft/mods/lithium-0.11.jar", ".minecraft/mods/iris-1.7.jar" };
        d.added = { ".minecraft/mods/sodium-fabric-0.6.5.jar", ".minecraft/mods/ferritecore-6.0.jar", ".minecraft/mods/iris-1.7.jar.disabled" };
        d.changed = { ".minecraft/config/sodium.json", "mmc-pack.json" };
        const auto mods = modChanges(d, ".minecraft");
        QCOMPARE(mods.size(), 4);
        auto kindOf = [&](const QString& name) {
            for (const auto& m : mods)
                if (m.name.startsWith(name))
                    return m.kind;
            return ModChange::Kind::Added;
        };
        QCOMPARE(kindOf("sodium"), ModChange::Kind::Updated);
        QCOMPARE(kindOf("lithium"), ModChange::Kind::Removed);
        QCOMPARE(kindOf("ferritecore"), ModChange::Kind::Added);
        QCOMPARE(kindOf("iris"), ModChange::Kind::Disabled);

        const QString md = markdownChangelog("Pack 1.1", d, ".minecraft");
        QVERIFY(md.startsWith("# Pack 1.1"));
        QVERIFY(md.contains("## Updated mods\n- sodium-fabric-0.6.5 (was sodium-fabric-0.5.8)"));
        QVERIFY(md.contains("## Added mods\n- ferritecore-6.0"));
        QVERIFY(md.contains("Minecraft and loader: 1 file(s) changed"));
        QVERIFY(md.contains("Config: 1 file(s) changed"));
    }
};

QTEST_GUILESS_MAIN(SnapshotTest)

#include "Snapshot_test.moc"
