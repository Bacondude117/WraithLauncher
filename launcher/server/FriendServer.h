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

#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <memory>

#include "net/NetJob.h"
#include "server/ServerModFilter.h"

class MinecraftInstance;

namespace WraithServer {

/**
 * A local dedicated server built from one instance ("friend server").
 *
 * Lives in <instance>/wraith-server/. One object per instance, owned by the application so a running server
 * keeps going (and keeps its console history) when the instance window is closed. Settings are stored in
 * wraith-server.json next to the server files; the instance itself is never modified.
 */
class FriendServer : public QObject {
    Q_OBJECT
   public:
    enum class State { NotSetUp, SettingUp, Stopped, Starting, Running, Stopping };
    enum class Loader { Vanilla, Fabric, Forge, NeoForge, Quilt, Unknown };

    /** Get (or create) the server object for an instance. It is deleted together with the instance. */
    static FriendServer* forInstance(MinecraftInstance* instance);

    ~FriendServer() override;

    QString serverDir() const;
    State state() const { return m_state; }
    bool isRunning() const { return m_state == State::Starting || m_state == State::Running || m_state == State::Stopping; }
    QString minecraftVersion() const;
    Loader loader() const;
    QString loaderVersion() const;
    static QString loaderName(Loader loader);
    /** Empty when this instance can be turned into a server, otherwise a user-facing explanation. */
    QString unsupportedReason() const;

    QList<ModEntry> mods() const;
    void setModIncluded(const QString& fileName, bool include);
    void resetModOverrides();

    int port() const { return m_port; }
    void setPort(int port);
    int memoryMb() const { return m_memoryMb; }
    void setMemoryMb(int memoryMb);

    bool eulaAccepted() const { return m_eulaAccepted; }
    /** Record the user's EULA choice and write eula.txt if the server folder exists. */
    void setEulaAccepted(bool accepted);

    QStringList consoleHistory() const { return m_console; }

    /** (Re)build the server folder: server software, mods, configs. EULA must be accepted by the user. */
    void setup(bool eulaAccepted);
    void start();
    void stop();
    void sendCommand(const QString& command);

   signals:
    void stateChanged();
    void consoleLine(const QString& line);

   private:
    explicit FriendServer(MinecraftInstance* instance, QObject* parent);

    void log(const QString& line);
    void setState(State state);
    void loadSettings();
    void saveSettings() const;
    QString javaPath() const;
    QStringList launchArguments() const;

    // setup steps
    void fail(const QString& error);
    void downloadVanilla();
    void downloadFabric();
    void runForgeInstaller();
    void copyFiles();
    void finishSetup();
    void fetchJson(const QString& url, std::function<void(const QByteArray&)> onSuccess);
    void downloadFile(const QString& url, const QString& path, std::function<void()> onSuccess);

    void onProcessOutput();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);

   private:
    MinecraftInstance* m_instance;
    State m_state = State::NotSetUp;
    bool m_eulaAccepted = false;
    int m_port = 25565;
    int m_memoryMb = 4096;
    QSet<QString> m_included;
    QSet<QString> m_excluded;
    QStringList m_console;
    QString m_partialLine;
    QPointer<QProcess> m_process;
    NetJob::Ptr m_job;
    QTimer m_killTimer;
};

}  // namespace WraithServer
