// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Wraith Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Wraith Launcher contributors
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

#include "PlayitTunnel.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#include <softpub.h>
#include <wintrust.h>
#endif

#include "Application.h"
#include "FileSystem.h"
#include "net/ChecksumValidator.h"
#include "net/Request.h"
#include "settings/SettingsObject.h"

namespace WraithServer {

namespace {
const QString s_releases = "https://api.github.com/repos/playit-cloud/playit-agent/releases?per_page=10";
// the signed Windows build (playit.gg's own code signature) is preferred over the unsigned one
const QStringList s_assets = { "playit-windows-x86_64-signed.exe", "playit-windows-x86_64.exe" };

QString toolsDir()
{
    return FS::PathCombine(APPLICATION->dataRoot(), "tools", "playit");
}

#ifdef Q_OS_WIN
// Windows' own check of the file's code signature (the signed agent is signed by playit.gg's company)
bool signatureValid(const QString& path)
{
    const std::wstring file = QDir::toNativeSeparators(path).toStdWString();
    WINTRUST_FILE_INFO fileInfo{};
    fileInfo.cbStruct = sizeof(fileInfo);
    fileInfo.pcwszFilePath = file.c_str();
    WINTRUST_DATA data{};
    data.cbStruct = sizeof(data);
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_FILE;
    data.pFile = &fileInfo;
    data.dwStateAction = WTD_STATEACTION_VERIFY;
    GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
    const LONG result = WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);
    data.dwStateAction = WTD_STATEACTION_CLOSE;
    WinVerifyTrust(static_cast<HWND>(INVALID_HANDLE_VALUE), &action, &data);
    return result == ERROR_SUCCESS;
}

bool processAlive(qint64 pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(pid));
    if (!process)
        return false;
    DWORD code = 0;
    const bool alive = GetExitCodeProcess(process, &code) && code == STILL_ACTIVE;
    CloseHandle(process);
    return alive;
}

void terminate(qint64 pid)
{
    if (HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(pid))) {
        TerminateProcess(process, 0);
        CloseHandle(process);
    }
}
#endif
}  // namespace

PlayitTunnel* PlayitTunnel::instance()
{
    static PlayitTunnel* s_instance = new PlayitTunnel(APPLICATION);
    return s_instance;
}

PlayitTunnel::PlayitTunnel(QObject* parent) : QObject(parent)
{
#ifdef Q_OS_WIN
    m_state = QFile::exists(agentPath()) ? State::Ready : State::NotInstalled;
#else
    m_state = State::Unsupported;
#endif
    m_poll.setInterval(4000);
    connect(&m_poll, &QTimer::timeout, this, &PlayitTunnel::poll);
    // the agent must not outlive the launcher
    connect(APPLICATION, &QCoreApplication::aboutToQuit, this, &PlayitTunnel::stop);
}

QString PlayitTunnel::agentPath() const
{
    return FS::PathCombine(toolsDir(), "playit.exe");
}

QString PlayitTunnel::agentVersion() const
{
    QFile file(FS::PathCombine(toolsDir(), "version.txt"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()).trimmed() : QString();
}

QString PlayitTunnel::address() const
{
    const QString manual = APPLICATION->settings()->get("WraithPlayitAddress").toString();
    return manual.isEmpty() ? m_detectedAddress : manual;
}

void PlayitTunnel::setManualAddress(const QString& address)
{
    APPLICATION->settings()->set("WraithPlayitAddress", address.trimmed());
    emit addressChanged();
}

void PlayitTunnel::setState(State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit stateChanged();
}

void PlayitTunnel::fail(const QString& error)
{
    m_error = error;
    emit message(tr("[playit.gg] %1").arg(error));
    setState(QFile::exists(agentPath()) ? State::Ready : State::NotInstalled);
}

void PlayitTunnel::install()
{
    if (m_state == State::Unsupported || m_state == State::Installing)
        return;
    setState(State::Installing);
    emit message(tr("[playit.gg] Looking up the latest agent release on GitHub..."));
    m_job = makeShared<NetJob>("Wraith::PlayitRelease", APPLICATION->network());
    auto [action, response] = Net::Request::makeByteArray(QUrl(s_releases));
    m_job->addNetAction(action);
    connect(m_job.get(), &Task::failed, this, [this](const QString& reason) { fail(tr("couldn't reach GitHub (%1)").arg(reason)); });
    connect(m_job.get(), &Task::succeeded, this, [this, response] {
        // the newest stable release whose Windows agent has a published SHA-256
        QString url;
        QString sha256;
        QString tag;
        for (const auto& r : QJsonDocument::fromJson(*response).array()) {
            const auto release = r.toObject();
            if (release.value("draft").toBool() || release.value("prerelease").toBool())
                continue;
            const auto assets = release.value("assets").toArray();
            for (const auto& wanted : s_assets) {
                for (const auto& a : assets) {
                    const auto asset = a.toObject();
                    const QString digest = asset.value("digest").toString();
                    if (asset.value("name").toString() == wanted && digest.startsWith("sha256:")) {
                        url = asset.value("browser_download_url").toString();
                        sha256 = digest.mid(7);
                        tag = release.value("tag_name").toString();
                        break;
                    }
                }
                if (!url.isEmpty())
                    break;
            }
            if (!url.isEmpty())
                break;
        }
        if (url.isEmpty() || !url.startsWith("https://"))
            return fail(tr("no agent release with a published checksum was found"));

        emit message(tr("[playit.gg] Downloading the agent %1 (SHA-256 %2...)").arg(tag, sha256.left(12)));
        FS::ensureFolderPathExists(toolsDir());
        const QString part = agentPath() + ".part";
        QFile::remove(part);
        m_job = makeShared<NetJob>("Wraith::PlayitAgent", APPLICATION->network());
        auto download = Net::Request::makeFile(QUrl(url), part);
        download->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha256, sha256));
        m_job->addNetAction(download);
        connect(m_job.get(), &Task::failed, this, [this, part](const QString& reason) {
            QFile::remove(part);
            fail(tr("the download failed or didn't match its checksum (%1)").arg(reason));
        });
        connect(m_job.get(), &Task::succeeded, this, [this, part, tag, url] {
#ifdef Q_OS_WIN
            // the signed build must also carry a valid code signature
            if (url.endsWith("-signed.exe") && !signatureValid(part)) {
                QFile::remove(part);
                return fail(tr("the agent's code signature isn't valid; it was not installed"));
            }
#endif
            QFile::remove(agentPath());
            if (!QFile::rename(part, agentPath()))
                return fail(tr("couldn't save the agent in %1").arg(toolsDir()));
            {
                QFile version(FS::PathCombine(toolsDir(), "version.txt"));
                if (version.open(QIODevice::WriteOnly))
                    version.write(tag.toUtf8());
            }  // closed before anyone reads it
            emit message(tr("[playit.gg] Agent %1 installed and verified.").arg(tag));
            m_error.clear();
            setState(State::Ready);
        });
        m_job->start();
    });
    m_job->start();
}

void PlayitTunnel::start(int localPort)
{
#ifndef Q_OS_WIN
    Q_UNUSED(localPort)
#else
    if (m_state == State::Running || !QFile::exists(agentPath()))
        return;
    // its own console window: the agent shows its claim link and tunnel status there, and says it must stay open
    QProcess process;
    process.setProgram(agentPath());
    process.setWorkingDirectory(toolsDir());
    if (!process.startDetached(&m_pid)) {
        fail(tr("couldn't start the agent"));
        return;
    }
    emit message(tr("[playit.gg] Agent started in its own window. The first time, it asks you to link it to your playit.gg account "
                    "and to add a Minecraft Java tunnel to port %1 on the playit.gg website.")
                     .arg(localPort));
    setState(State::Running);
    m_poll.start();
    QTimer::singleShot(1500, this, &PlayitTunnel::scanLogs);
#endif
}

void PlayitTunnel::stop()
{
#ifdef Q_OS_WIN
    m_poll.stop();
    if (m_pid > 0 && processAlive(m_pid))
        terminate(m_pid);
    m_pid = 0;
    if (m_state == State::Running) {
        emit message(tr("[playit.gg] Agent stopped; the tunnel is offline."));
        setState(State::Ready);
    }
#endif
}

void PlayitTunnel::poll()
{
#ifdef Q_OS_WIN
    if (m_pid > 0 && !processAlive(m_pid)) {
        m_pid = 0;
        m_poll.stop();
        emit message(tr("[playit.gg] The agent window was closed; the tunnel is offline."));
        setState(State::Ready);
        return;
    }
#endif
    scanLogs();
}

void PlayitTunnel::scanLogs()
{
    // The agent logs to its own folders (portable: %LOCALAPPDATA%\playit_gg, service: %ProgramData%\playit_gg).
    // Recent logs are searched for the tunnel address and a pending claim link; nothing is written there.
    static const QRegularExpression s_address(R"(\b([a-z0-9][a-z0-9-]*\.(?:joinmc\.link|ply\.gg|at\.ply\.gg)(?::\d{2,5})?)\b)");
    static const QRegularExpression s_claim(R"((https://playit\.gg/claim/[A-Za-z0-9]+))");
    QStringList roots;
    for (const char* variable : { "LOCALAPPDATA", "ProgramData", "APPDATA" })
        if (const QString base = qEnvironmentVariable(variable); !base.isEmpty())
            roots.append(QDir(base).filePath("playit_gg"));
    QString address;
    QString claim;
    QDateTime newest;
    for (const auto& root : roots) {
        QDirIterator it(root, { "*.log", "*.txt" }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo info(it.next());
            if (info.lastModified().secsTo(QDateTime::currentDateTime()) > 7 * 24 * 3600 || info.size() > 20 * 1024 * 1024)
                continue;
            QFile file(info.absoluteFilePath());
            if (!file.open(QIODevice::ReadOnly))
                continue;
            const QString text = QString::fromUtf8(file.readAll().right(512 * 1024));
            auto matches = s_address.globalMatch(text);
            QString last;
            while (matches.hasNext())
                last = matches.next().captured(1);
            if (!last.isEmpty() && (newest.isNull() || info.lastModified() > newest)) {
                address = last;
                newest = info.lastModified();
            }
            auto claims = s_claim.globalMatch(text);
            while (claims.hasNext())
                claim = claims.next().captured(1);
        }
    }
    if (!address.isEmpty() && address != m_detectedAddress) {
        m_detectedAddress = address;
        emit message(tr("[playit.gg] Tunnel address: %1").arg(address));
        emit addressChanged();
    }
    // once an address is known the account is linked, so an old claim link no longer matters
    const QString pending = address.isEmpty() ? claim : QString();
    if (pending != m_claimUrl) {
        m_claimUrl = pending;
        emit stateChanged();
    }
}

}  // namespace WraithServer
