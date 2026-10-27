#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <security/SecretStore.h>
#include <server/ServerFileSync.h>

using WraithServer::FileSync;

namespace {
void writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(data);
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}
}  // namespace

// Tests for the core-values fixes (VALUES_AUDIT.md): encrypted tokens, never overwriting manual edits.
class WraithValuesTest : public QObject {
    Q_OBJECT

   private slots:
    void secretStoreRoundTrip()
    {
        const QByteArray plain = R"({"accounts":[{"token":"secret-value"}]})";
        const QByteArray stored = Wraith::SecretStore::protect(plain);
        if (Wraith::SecretStore::available()) {
            QVERIFY(Wraith::SecretStore::isProtected(stored));
            QVERIFY(!stored.contains("secret-value"));
        }
        auto back = Wraith::SecretStore::unprotect(stored);
        QVERIFY(back.has_value());
        QCOMPARE(*back, plain);
    }

    void secretStoreReadsPlainFiles()
    {
        // accounts.json written by Prism (or an older Wraith) is plain JSON and must still load
        const QByteArray plain = R"({"formatVersion":3})";
        auto back = Wraith::SecretStore::unprotect(plain);
        QVERIFY(back.has_value());
        QCOMPARE(*back, plain);
    }

    void secretStoreRejectsGarbage()
    {
        if (!Wraith::SecretStore::available())
            QSKIP("no protection on this platform");
        QByteArray tampered = Wraith::SecretStore::protect("abc");
        tampered.chop(8);
        tampered += "AAAAAAA\n";
        QVERIFY(!Wraith::SecretStore::unprotect(tampered).has_value());
    }

    void fileSyncKeepsManualEdits()
    {
        QTemporaryDir source;
        QTemporaryDir target;
        QDir(target.path()).mkpath("mods");
        writeFile(source.filePath("a.jar"), "a1");
        writeFile(source.filePath("b.jar"), "b1");
        const QString manifest = target.filePath("manifest.json");
        const QList<FileSync::Item> items{ { source.filePath("a.jar"), "mods/a.jar" }, { source.filePath("b.jar"), "mods/b.jar" } };

        {
            FileSync sync(target.path(), manifest);
            auto result = sync.sync(items, { "mods/" });
            QCOMPARE(result.copied, 2);
        }

        // the user edits a.jar in the server folder and adds their own mod
        writeFile(target.filePath("mods/a.jar"), "edited by hand");
        writeFile(target.filePath("mods/mine.jar"), "mine");
        // the instance updates both mods
        writeFile(source.filePath("a.jar"), "a2");
        writeFile(source.filePath("b.jar"), "b2");

        FileSync sync(target.path(), manifest);
        auto result = sync.sync(items, { "mods/" });
        QCOMPARE(readFile(target.filePath("mods/a.jar")), QByteArray("edited by hand"));
        QCOMPARE(readFile(target.filePath("mods/b.jar")), QByteArray("b2"));
        QVERIFY(QFile::exists(target.filePath("mods/mine.jar")));
        QCOMPARE(result.keptEdited, QStringList{ "mods/a.jar" });
    }

    void fileSyncRemovesOnlyUntouchedCopies()
    {
        QTemporaryDir source;
        QTemporaryDir target;
        writeFile(source.filePath("x.jar"), "x");
        writeFile(source.filePath("y.jar"), "y");
        const QString manifest = target.filePath("manifest.json");
        {
            FileSync sync(target.path(), manifest);
            sync.sync({ { source.filePath("x.jar"), "mods/x.jar" }, { source.filePath("y.jar"), "mods/y.jar" } }, { "mods/" });
        }
        writeFile(target.filePath("mods/y.jar"), "changed");
        // both mods were removed from the instance
        FileSync sync(target.path(), manifest);
        auto result = sync.sync({}, { "mods/" });
        QCOMPARE(result.removed, 1);
        QVERIFY(!QFile::exists(target.filePath("mods/x.jar")));
        QVERIFY(QFile::exists(target.filePath("mods/y.jar")));
    }
};

QTEST_GUILESS_MAIN(WraithValuesTest)

#include "WraithValues_test.moc"
