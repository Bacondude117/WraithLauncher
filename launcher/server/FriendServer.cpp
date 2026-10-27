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
#include "FriendServer.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include "Application.h"
#include "FileSystem.h"
#include "java/JavaUtils.h"
#include "minecraft/LaunchProfile.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "net/ApiRequest.h"
#include "settings/SettingsObject.h"

namespace WraithServer {

namespace {
QHash<QString, FriendServer*>& registry()
{
    static QHash<QString, FriendServer*> s_servers;
    return s_servers;
}

constexpr int s_maxConsoleLines = 5000;
const char* s_settingsFile = "wraith-server.json";
// Folders copied from the instance besides mods; they hold server-relevant configuration and scripts.
const QStringList s_copiedFolders = { "config", "defaultconfigs", "kubejs", "scripts", "global_packs" };
}  // namespace

FriendServer* FriendServer::forInstance(MinecraftInstance* instance)
{
    if (!instance)
        return nullptr;
    auto& servers = registry();
    if (auto it = servers.constFind(instance->id()); it != servers.constEnd())
        return *it;
    // Parented to the instance: when the instance is deleted (or the launcher quits) the server goes too.
    auto* server = new FriendServer(instance, instance);
    servers.insert(instance->id(), server);
    return server;
}

FriendServer::FriendServer(MinecraftInstance* instance, QObject* parent) : QObject(parent), m_instance(instance)
{
    m_memoryMb = m_instance->settings()->get("MaxMemAlloc").toInt();
    if (m_memoryMb < 1024)
        m_memoryMb = 2048;
    loadSettings();
    const bool setUp = QFile::exists(FS::PathCombine(serverDir(), "server.jar")) || !launchArguments().isEmpty();
    m_state = setUp ? State::Stopped : State::NotSetUp;

    m_killTimer.setSingleShot(true);
    connect(&m_killTimer, &QTimer::timeout, this, [this] {
        if (m_process && m_process->state() != QProcess::NotRunning) {
            log(tr("[Wraith] Server didn't stop in time, killing it."));
            m_process->kill();
        }
    });
}

FriendServer::~FriendServer()
{
    registry().remove(registry().key(this));
    if (m_process && m_process->state() != QProcess::NotRunning) {
        // Launcher is quitting: ask the server to save and stop, then make sure it's gone.
        m_process->write("stop\n");
        if (!m_process->waitForFinished(15000))
            m_process->kill();
    }
}

QString FriendServer::serverDir() const
{
    return FS::PathCombine(m_instance->instanceRoot(), "wraith-server");
}

PackProfile* FriendServer::profile() const
{
    auto* profile = m_instance->getPackProfile();
    // Components load lazily (when the instance is opened/launched); read them from disk if needed.
    if (profile->getComponentVersion("net.minecraft").isEmpty()) {
        if (auto res = profile->reload(Net::Mode::Offline); !res)
            qWarning() << "Friend server: couldn't load components of" << m_instance->name() << res.error();
    }
    return profile;
}

QString FriendServer::minecraftVersion() const
{
    return profile()->getComponentVersion("net.minecraft");
}

FriendServer::Loader FriendServer::loader() const
{
    auto* profile = this->profile();
    if (!profile->getComponentVersion("net.fabricmc.fabric-loader").isEmpty())
        return Loader::Fabric;
    if (!profile->getComponentVersion("org.quiltmc.quilt-loader").isEmpty())
        return Loader::Quilt;
    if (!profile->getComponentVersion("net.neoforged").isEmpty())
        return Loader::NeoForge;
    if (!profile->getComponentVersion("net.minecraftforge").isEmpty())
        return Loader::Forge;
    return Loader::Vanilla;
}

QString FriendServer::loaderVersion() const
{
    auto* profile = this->profile();
    switch (loader()) {
        case Loader::Fabric:
            return profile->getComponentVersion("net.fabricmc.fabric-loader");
        case Loader::Quilt:
            return profile->getComponentVersion("org.quiltmc.quilt-loader");
        case Loader::NeoForge:
            return profile->getComponentVersion("net.neoforged");
        case Loader::Forge:
            return profile->getComponentVersion("net.minecraftforge");
        default:
            return {};
    }
}

QString FriendServer::loaderName(Loader loader)
{
    switch (loader) {
        case Loader::Vanilla:
            return tr("Vanilla");
        case Loader::Fabric:
            return "Fabric";
        case Loader::Forge:
            return "Forge";
        case Loader::NeoForge:
            return "NeoForge";
        case Loader::Quilt:
            return "Quilt";
        case Loader::Unknown:
            break;
    }
    return tr("Unknown");
}

QString FriendServer::unsupportedReason() const
{
    if (minecraftVersion().isEmpty())
        return tr("This instance has no Minecraft version.");
    return {};
}

void FriendServer::loadSettings()
{
    QFile file(FS::PathCombine(serverDir(), s_settingsFile));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const auto obj = QJsonDocument::fromJson(file.readAll()).object();
    m_port = obj.value("port").toInt(m_port);
    m_memoryMb = obj.value("memoryMb").toInt(m_memoryMb);
    m_eulaAccepted = obj.value("eulaAccepted").toBool(false);
    for (const auto& v : obj.value("includedMods").toArray())
        m_included.insert(v.toString());
    for (const auto& v : obj.value("excludedMods").toArray())
        m_excluded.insert(v.toString());
}

void FriendServer::saveSettings() const
{
    QJsonObject obj;
    obj.insert("port", m_port);
    obj.insert("memoryMb", m_memoryMb);
    obj.insert("eulaAccepted", m_eulaAccepted);
    obj.insert("includedMods", QJsonArray::fromStringList(QStringList(m_included.begin(), m_included.end())));
    obj.insert("excludedMods", QJsonArray::fromStringList(QStringList(m_excluded.begin(), m_excluded.end())));
    FS::ensureFolderPathExists(serverDir());
    QFile file(FS::PathCombine(serverDir(), s_settingsFile));
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson());
}

QList<ModEntry> FriendServer::mods() const
{
    return classifyModsFolder(QDir(m_instance->modsRoot()), m_included, m_excluded);
}

void FriendServer::setModIncluded(const QString& fileName, bool include)
{
    m_included.remove(fileName);
    m_excluded.remove(fileName);
    // store an override only when it differs from the automatic decision
    for (const auto& entry : classifyModsFolder(QDir(m_instance->modsRoot()))) {
        if (entry.fileName == fileName && entry.include != include)
            (include ? m_included : m_excluded).insert(fileName);
    }
    saveSettings();
}

void FriendServer::resetModOverrides()
{
    m_included.clear();
    m_excluded.clear();
    saveSettings();
}

void FriendServer::setPort(int port)
{
    m_port = port;
    saveSettings();
}

void FriendServer::setMemoryMb(int memoryMb)
{
    m_memoryMb = memoryMb;
    saveSettings();
}

void FriendServer::log(const QString& line)
{
    m_console.append(line);
    if (m_console.size() > s_maxConsoleLines)
        m_console.remove(0, m_console.size() - s_maxConsoleLines);
    emit consoleLine(line);
}

void FriendServer::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

namespace {
/** Major version of a Java install, read from <home>/release (JAVA_VERSION="17.0.2" / "1.8.0_392"); 0 if unknown. */
int javaMajor(const QString& javaExe)
{
    QFile release(QFileInfo(javaExe).dir().filePath("../release"));
    if (!release.open(QIODevice::ReadOnly | QIODevice::Text))
        return 0;
    static const QRegularExpression s_version(R"re(JAVA_VERSION="(\d+)(?:\.(\d+))?)re");
    const auto match = s_version.match(QString::fromUtf8(release.readAll()));
    if (!match.hasMatch())
        return 0;
    const int first = match.captured(1).toInt();
    return first == 1 ? match.captured(2).toInt() : first;
}

/** Prefer java(.exe) over javaw(.exe) so the server writes to a normal console stream. */
QString consoleJava(const QString& javaExe)
{
    QFileInfo info(javaExe);
    if (info.baseName() == "javaw") {
        const QString java = info.dir().filePath(QString("java") + (info.suffix().isEmpty() ? "" : "." + info.suffix()));
        if (QFile::exists(java))
            return java;
    }
    return javaExe;
}
}  // namespace

QString FriendServer::javaPath() const
{
    // Same resolution as the instance launch ($INST_JAVA): relative paths are relative to the data dir.
    const QString path = m_instance->settings()->get("JavaPath").toString();
    if (!path.isEmpty())
        return consoleJava(QDir(path).absolutePath());

    // The instance was never launched (no Java picked yet): find an installed Java that suits this version.
    QList<int> wanted;
    if (auto launchProfile = profile()->getProfile())
        wanted = launchProfile->getCompatibleJavaMajors();
    QStringList candidates = JavaUtils().FindJavaPaths();
    // Java runtimes the launcher downloaded itself (<data>/java/<runtime>/bin/java)
    QDirIterator managed(APPLICATION->javaPath(), { "java", "java.exe" }, QDir::Files, QDirIterator::Subdirectories);
    while (managed.hasNext())
        candidates.append(managed.next());

    QString best;
    int bestMajor = -1;
    for (const auto& candidate : candidates) {
        if (!QFile::exists(candidate))
            continue;
        const int major = javaMajor(candidate);
        // exact match with what this Minecraft version asks for wins, otherwise the newest Java 17+
        const int score = wanted.contains(major) ? 1000 + major : (major >= 17 ? major : major - 100);
        if (score > bestMajor) {
            bestMajor = score;
            best = candidate;
        }
    }
    return best.isEmpty() ? QString("java") : consoleJava(best);
}

QStringList FriendServer::launchArguments() const
{
    const QDir dir(serverDir());
    // Modern Forge/NeoForge installers write an argument file under libraries/ instead of a runnable jar.
#ifdef Q_OS_WIN
    const QString argsName = "win_args.txt";
#else
    const QString argsName = "unix_args.txt";
#endif
    QDirIterator it(dir.filePath("libraries"), { argsName }, QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext())
        return { "@" + dir.relativeFilePath(it.next()) };
    // Quilt installs its launcher next to the vanilla server.jar; the launcher must win.
    if (dir.exists("quilt-server-launch.jar"))
        return { "-jar", "quilt-server-launch.jar" };
    if (dir.exists("server.jar"))
        return { "-jar", "server.jar" };
    // Older Forge: a forge-<version>.jar (not the installer) next to the libraries
    for (const auto& jar : dir.entryList({ "forge-*.jar", "neoforge-*.jar" }, QDir::Files)) {
        if (!jar.contains("installer"))
            return { "-jar", jar };
    }
    return {};
}

// ---------------------------------------------------------------- setup

void FriendServer::fail(const QString& error)
{
    log(tr("[Wraith] Setup failed: %1").arg(error));
    m_job.reset();
    setState(launchArguments().isEmpty() ? State::NotSetUp : State::Stopped);
}

void FriendServer::fetchJson(const QString& url, std::function<void(const QByteArray&)> onSuccess)
{
    m_job = makeShared<NetJob>("WraithServer::fetch", APPLICATION->network());
    auto [action, response] = Net::ApiRequest::makeByteArray(QUrl(url));
    m_job->addNetAction(action);
    connect(m_job.get(), &Task::succeeded, this, [onSuccess, response] { onSuccess(*response); });
    connect(m_job.get(), &Task::failed, this, [this, url](const QString& reason) { fail(tr("couldn't download %1 (%2)").arg(url, reason)); });
    m_job->start();
}

void FriendServer::downloadFile(const QString& url, const QString& path, std::function<void()> onSuccess)
{
    log(tr("[Wraith] Downloading %1").arg(url));
    m_job = makeShared<NetJob>("WraithServer::download", APPLICATION->network());
    m_job->addNetAction(Net::ApiRequest::makeFile(QUrl(url), path));
    connect(m_job.get(), &Task::succeeded, this, [onSuccess] { onSuccess(); });
    connect(m_job.get(), &Task::failed, this, [this, url](const QString& reason) { fail(tr("couldn't download %1 (%2)").arg(url, reason)); });
    m_job->start();
}

void FriendServer::setup(bool eulaAccepted)
{
    if (isRunning() || m_state == State::SettingUp)
        return;
    if (auto reason = unsupportedReason(); !reason.isEmpty()) {
        log(tr("[Wraith] %1").arg(reason));
        return;
    }
    m_eulaAccepted = eulaAccepted;
    saveSettings();
    setState(State::SettingUp);
    log(tr("[Wraith] Setting up a %1 %2 server in %3").arg(loaderName(loader()), minecraftVersion(), serverDir()));
    FS::ensureFolderPathExists(serverDir());

    switch (loader()) {
        case Loader::Fabric:
            downloadFabric();
            break;
        case Loader::Forge:
        case Loader::NeoForge:
            runForgeInstaller();
            break;
        case Loader::Quilt:
            runQuiltInstaller();
            break;
        default:
            downloadVanilla();
            break;
    }
}

void FriendServer::downloadVanilla()
{
    const QString version = minecraftVersion();
    fetchJson("https://piston-meta.mojang.com/mc/game/version_manifest_v2.json", [this, version](const QByteArray& data) {
        QString versionUrl;
        for (const auto& v : QJsonDocument::fromJson(data).object().value("versions").toArray()) {
            if (v.toObject().value("id").toString() == version)
                versionUrl = v.toObject().value("url").toString();
        }
        if (versionUrl.isEmpty())
            return fail(tr("Minecraft %1 isn't in Mojang's version manifest").arg(version));
        fetchJson(versionUrl, [this, version](const QByteArray& versionData) {
            const auto server = QJsonDocument::fromJson(versionData).object().value("downloads").toObject().value("server").toObject();
            const QString url = server.value("url").toString();
            if (url.isEmpty())
                return fail(tr("Mojang doesn't provide a server for Minecraft %1").arg(version));
            downloadFile(url, FS::PathCombine(serverDir(), "server.jar"), [this] { copyFiles(); });
        });
    });
}

void FriendServer::downloadFabric()
{
    // The Fabric server launcher downloads the matching vanilla server itself on first start.
    fetchJson("https://meta.fabricmc.net/v2/versions/installer", [this](const QByteArray& data) {
        QString installer;
        for (const auto& v : QJsonDocument::fromJson(data).array()) {
            if (v.toObject().value("stable").toBool()) {
                installer = v.toObject().value("version").toString();
                break;
            }
        }
        if (installer.isEmpty())
            return fail(tr("couldn't find a Fabric installer version"));
        const QString url =
            QString("https://meta.fabricmc.net/v2/versions/loader/%1/%2/%3/server/jar").arg(minecraftVersion(), loaderVersion(), installer);
        downloadFile(url, FS::PathCombine(serverDir(), "server.jar"), [this] { copyFiles(); });
    });
}

void FriendServer::runForgeInstaller()
{
    const QString mc = minecraftVersion();
    const QString version = loaderVersion();
    QString url;
    if (loader() == Loader::NeoForge) {
        url = QString("https://maven.neoforged.net/releases/net/neoforged/neoforge/%1/neoforge-%1-installer.jar").arg(version);
    } else {
        url = QString("https://maven.minecraftforge.net/net/minecraftforge/forge/%1-%2/forge-%1-%2-installer.jar").arg(mc, version);
    }
    runInstaller(url, { "--installServer" });
}

void FriendServer::runQuiltInstaller()
{
    // The Quilt installer sets up quilt-server-launch.jar and (with --download-server) the vanilla server.jar.
    fetchJson("https://maven.quiltmc.org/repository/release/org/quiltmc/quilt-installer/maven-metadata.xml", [this](const QByteArray& data) {
        static const QRegularExpression s_release("<release>([^<]+)</release>");
        const auto match = s_release.match(QString::fromUtf8(data));
        if (!match.hasMatch())
            return fail(tr("couldn't find a Quilt installer version"));
        const QString installer = match.captured(1);
        const QString url =
            QString("https://maven.quiltmc.org/repository/release/org/quiltmc/quilt-installer/%1/quilt-installer-%1.jar").arg(installer);
        runInstaller(url, { "install", "server", minecraftVersion(), loaderVersion(), "--download-server", "--install-dir=." });
    });
}

void FriendServer::runInstaller(const QString& url, const QStringList& installerArgs)
{
    const QString installerPath = FS::PathCombine(serverDir(), "installer.jar");
    downloadFile(url, installerPath, [this, installerPath, installerArgs] {
        log(tr("[Wraith] Running the %1 installer with %2 (this can take a few minutes)...").arg(loaderName(loader()), javaPath()));
        auto* installer = new QProcess(this);
        installer->setWorkingDirectory(serverDir());
        installer->setProcessChannelMode(QProcess::MergedChannels);
        connect(installer, &QProcess::readyRead, this, [this, installer] {
            for (const auto& line : QString::fromLocal8Bit(installer->readAll()).split('\n', Qt::SkipEmptyParts))
                log(line.trimmed());
        });
        connect(installer, &QProcess::finished, this, [this, installer, installerPath](int exitCode, QProcess::ExitStatus status) {
            installer->deleteLater();
            QFile::remove(installerPath);
            QFile::remove(installerPath + ".log");
            if (status != QProcess::NormalExit || exitCode != 0)
                return fail(tr("the installer exited with code %1").arg(exitCode));
            copyFiles();
        });
        connect(installer, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) { fail(tr("couldn't run Java at %1").arg(javaPath())); });
        installer->start(javaPath(), QStringList{ "-jar", "installer.jar" } + installerArgs);
    });
}

void FriendServer::copyFiles()
{
    m_job.reset();
    const QDir server(serverDir());
    const QDir game(m_instance->gameRoot());

    // mods: rebuild from scratch so removed/excluded mods don't linger
    const QString modsDir = server.filePath("mods");
    FS::deletePath(modsDir);
    FS::ensureFolderPathExists(modsDir);
    int copied = 0;
    int skipped = 0;
    for (const auto& mod : mods()) {
        if (!mod.include) {
            log(tr("[Wraith] Skipping %1 (%2)").arg(mod.name, describe(mod.reason)));
            skipped++;
            continue;
        }
        if (QFile::copy(QDir(m_instance->modsRoot()).filePath(mod.fileName), QDir(modsDir).filePath(mod.fileName)))
            copied++;
        else
            log(tr("[Wraith] Couldn't copy %1").arg(mod.fileName));
    }
    log(tr("[Wraith] Copied %1 mod(s), left out %2 client-only mod(s).").arg(copied).arg(skipped));

    for (const auto& folder : s_copiedFolders) {
        if (!game.exists(folder))
            continue;
        FS::copy copy(game.filePath(folder), server.filePath(folder));
        copy.overwrite(true);
        copy();
        log(tr("[Wraith] Copied %1/").arg(folder));
    }
    finishSetup();
}

void FriendServer::setEulaAccepted(bool accepted)
{
    m_eulaAccepted = accepted;
    saveSettings();
    QFile eula(FS::PathCombine(serverDir(), "eula.txt"));
    if (eula.open(QIODevice::WriteOnly | QIODevice::Text)) {
        // Only true if the user ticked "I accept the Minecraft EULA" in the dashboard.
        eula.write(QString("# https://aka.ms/MinecraftEULA\neula=%1\n").arg(m_eulaAccepted ? "true" : "false").toUtf8());
    }
}

void FriendServer::finishSetup()
{
    const QDir server(serverDir());
    setEulaAccepted(m_eulaAccepted);

    QFile properties(server.filePath("server.properties"));
    if (!properties.exists() && properties.open(QIODevice::WriteOnly | QIODevice::Text)) {
        properties.write(QString("server-port=%1\nmotd=%2 (Wraith friend server)\nonline-mode=true\n")
                             .arg(m_port)
                             .arg(m_instance->name())
                             .toUtf8());
    }

    if (launchArguments().isEmpty())
        return fail(tr("the server files are missing after setup"));
    log(tr("[Wraith] Server is ready. Press Start to run it."));
    setState(State::Stopped);
}

// ---------------------------------------------------------------- process

void FriendServer::start()
{
    if (isRunning() || m_state == State::SettingUp)
        return;
    const QStringList launch = launchArguments();
    if (launch.isEmpty()) {
        log(tr("[Wraith] Set the server up first."));
        return;
    }
    if (!m_eulaAccepted) {
        log(tr("[Wraith] You need to accept the Minecraft EULA before the server can start."));
        return;
    }

    // keep server.properties' port in sync with the dashboard setting
    QFile properties(FS::PathCombine(serverDir(), "server.properties"));
    if (properties.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QStringList lines = QString::fromUtf8(properties.readAll()).split('\n');
        properties.close();
        bool found = false;
        for (auto& line : lines) {
            if (line.startsWith("server-port=")) {
                line = QString("server-port=%1").arg(m_port);
                found = true;
            }
        }
        if (!found)
            lines.prepend(QString("server-port=%1").arg(m_port));
        if (properties.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            properties.write(lines.join('\n').toUtf8());
    }

    m_process = new QProcess(this);
    m_process->setWorkingDirectory(serverDir());
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, &QProcess::readyRead, this, &FriendServer::onProcessOutput);
    connect(m_process, &QProcess::finished, this, &FriendServer::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            log(tr("[Wraith] Couldn't start Java at %1. Check the instance's Java settings.").arg(javaPath()));
            m_process->deleteLater();
            setState(State::Stopped);
        }
    });

    QStringList args = { QString("-Xms%1m").arg(std::min(1024, m_memoryMb)), QString("-Xmx%1m").arg(m_memoryMb) };
    args += launch;
    args << "nogui";
    log(tr("[Wraith] Starting: %1 %2").arg(javaPath(), args.join(' ')));
    setState(State::Starting);
    m_process->start(javaPath(), args);
}

void FriendServer::stop()
{
    if (!m_process || m_process->state() == QProcess::NotRunning)
        return;
    log(tr("[Wraith] Stopping the server (saving worlds)..."));
    setState(State::Stopping);
    m_process->write("stop\n");
    m_killTimer.start(30000);
}

void FriendServer::sendCommand(const QString& command)
{
    if (!m_process || m_process->state() != QProcess::Running || command.trimmed().isEmpty())
        return;
    log("> " + command);
    m_process->write(command.trimmed().toUtf8() + "\n");
}

void FriendServer::onProcessOutput()
{
    m_partialLine += QString::fromLocal8Bit(m_process->readAll());
    qsizetype newline;
    while ((newline = m_partialLine.indexOf('\n')) >= 0) {
        const QString line = m_partialLine.left(newline).trimmed();
        m_partialLine.remove(0, newline + 1);
        if (line.isEmpty())
            continue;
        log(line);
        // "Done (4.2s)! For help, type "help"" is the vanilla/Fabric/Forge ready line
        if (m_state == State::Starting && line.contains("Done (") && line.contains("help"))
            setState(State::Running);
    }
}

void FriendServer::onProcessFinished(int exitCode, QProcess::ExitStatus status)
{
    m_killTimer.stop();
    if (!m_partialLine.trimmed().isEmpty())
        log(m_partialLine.trimmed());
    m_partialLine.clear();
    log(status == QProcess::NormalExit ? tr("[Wraith] Server stopped (exit code %1).").arg(exitCode) : tr("[Wraith] Server crashed."));
    m_process->deleteLater();
    setState(State::Stopped);
}

}  // namespace WraithServer
