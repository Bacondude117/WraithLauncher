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

#pragma once

#include <QObject>
#include <QTimer>

#include "net/NetJob.h"

namespace WraithServer {

/**
 * Optional playit.gg tunnel for friend servers, so friends can join over the internet without port forwarding.
 *
 * playit.gg is a third-party service: it needs the user's own (free) playit.gg account, and players' traffic goes through
 * its servers. It is off by default and only used for servers where the user switches it on (core value: LAN-only by
 * default). Wraith downloads the official signed agent from playit.gg's GitHub releases, checks its SHA-256 against the
 * digest GitHub publishes for it, and starts it in its own window while the server runs. The agent links itself to the
 * account (claim) and keeps its own secret; tunnels are created on the playit.gg website. Wraith only reads the
 * agent's log to show the address to share.
 *
 * One agent per computer (the agent itself refuses a second copy). Windows only for now.
 */
class PlayitTunnel : public QObject {
    Q_OBJECT
   public:
    enum class State { Unsupported, NotInstalled, Installing, Ready, Running };

    static PlayitTunnel* instance();

    State state() const { return m_state; }
    QString lastError() const { return m_error; }
    QString agentPath() const;
    QString agentVersion() const;
    /** The address to give friends: found in the agent's log, or typed in by the user. */
    QString address() const;
    void setManualAddress(const QString& address);
    /** A link to connect the agent to the user's playit.gg account, while it waits for that. */
    QString claimUrl() const { return m_claimUrl; }

    /** Download the agent (newest release with a published SHA-256) into Wraith's data folder. */
    void install();
    /** Start the agent for a server on localPort (the port the tunnel on playit.gg should point to). */
    void start(int localPort);
    void stop();
    bool isRunning() const { return m_state == State::Running; }

   signals:
    void stateChanged();
    void addressChanged();
    void message(const QString& line);

   private:
    explicit PlayitTunnel(QObject* parent);
    void setState(State state);
    void fail(const QString& error);
    void poll();
    void scanLogs();

    State m_state = State::NotInstalled;
    QString m_error;
    QString m_detectedAddress;
    QString m_claimUrl;
    qint64 m_pid = 0;
    QTimer m_poll;
    NetJob::Ptr m_job;
};

}  // namespace WraithServer
