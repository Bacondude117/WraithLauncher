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
#include "ServerDashboardPage.h"

#include <QCheckBox>
#include <QClipboard>
#include <QGroupBox>
#include <QGuiApplication>
#include <QUrl>
#include <QFontDatabase>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkInterface>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSpinBox>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "DesktopServices.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "server/FriendServer.h"
#include "server/PlayitTunnel.h"

using WraithServer::FriendServer;

ServerDashboardPage::ServerDashboardPage(MinecraftInstance* instance, QWidget* parent)
    : QWidget(parent), m_server(FriendServer::forInstance(instance))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* intro = new QLabel(tr("Run this instance as a dedicated server so friends can join you. The server lives in its own "
                                "folder inside the instance; your game files are never changed. Client-only mods are left out."),
                             this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    // status + controls
    auto* controls = new QHBoxLayout();
    m_status = new QLabel(this);
    m_status->setTextFormat(Qt::RichText);
    controls->addWidget(m_status, 1);
    m_setupButton = new QPushButton(this);
    m_startButton = new QPushButton(tr("Start"), this);
    m_stopButton = new QPushButton(tr("Stop"), this);
    m_folderButton = new QPushButton(tr("Open Folder"), this);
    controls->addWidget(m_setupButton);
    controls->addWidget(m_startButton);
    controls->addWidget(m_stopButton);
    controls->addWidget(m_folderButton);
    layout->addLayout(controls);

    // settings
    auto* settings = new QHBoxLayout();
    auto* form = new QFormLayout();
    m_port = new QSpinBox(this);
    m_port->setRange(1024, 65535);
    m_port->setValue(m_server->port());
    form->addRow(tr("Port:"), m_port);
    m_memory = new QSpinBox(this);
    m_memory->setRange(512, 65536);
    m_memory->setSingleStep(512);
    m_memory->setSuffix(" MiB");
    m_memory->setValue(m_server->memoryMb());
    form->addRow(tr("Memory:"), m_memory);
    settings->addLayout(form);
    auto* right = new QVBoxLayout();
    m_eula = new QCheckBox(this);
    m_eula->setChecked(m_server->eulaAccepted());
    auto* eulaLabel = new QLabel(tr("I accept the <a href=\"https://aka.ms/MinecraftEULA\">Minecraft EULA</a>"), this);
    eulaLabel->setOpenExternalLinks(true);
    auto* eulaRow = new QHBoxLayout();
    eulaRow->addWidget(m_eula);
    eulaRow->addWidget(eulaLabel, 1);
    right->addLayout(eulaRow);
    m_join = new QLabel(this);
    m_join->setWordWrap(true);
    m_join->setTextInteractionFlags(Qt::TextSelectableByMouse);
    right->addWidget(m_join);
    settings->addLayout(right, 1);
    layout->addLayout(settings);

    // optional: play over the internet through playit.gg (no port forwarding); off unless switched on per server
    {
        using WraithServer::PlayitTunnel;
        auto* box = new QGroupBox(tr("Play over the internet (optional)"), this);
        auto* boxLayout = new QVBoxLayout(box);
        auto* explain = new QLabel(
            tr("Without port forwarding, through <a href=\"https://playit.gg\">playit.gg</a>, a free third-party tunnel service. "
               "It needs your own playit.gg account, and your friends' connections pass through playit.gg's servers. Wraith "
               "downloads playit.gg's official agent from GitHub, checks its checksum, and runs it only while this server runs."),
            box);
        explain->setWordWrap(true);
        explain->setOpenExternalLinks(true);
        boxLayout->addWidget(explain);
        auto* row = new QHBoxLayout();
        m_playit = new QCheckBox(tr("Use playit.gg for this server"), box);
        m_playit->setChecked(m_server->playitEnabled());
        row->addWidget(m_playit);
        m_playitStatus = new QLabel(box);
        m_playitStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(m_playitStatus, 1);
        m_playitClaim = new QPushButton(tr("Link playit.gg account..."), box);
        row->addWidget(m_playitClaim);
        m_playitInstall = new QPushButton(box);
        row->addWidget(m_playitInstall);
        boxLayout->addLayout(row);
        auto* addressRow = new QHBoxLayout();
        addressRow->addWidget(new QLabel(tr("Address to share:"), box));
        m_playitAddress = new QLineEdit(box);
        m_playitAddress->setPlaceholderText(tr("Shown here once the tunnel is set up, e.g. name.joinmc.link (you can also paste it)"));
        addressRow->addWidget(m_playitAddress, 1);
        auto* copy = new QPushButton(tr("Copy"), box);
        addressRow->addWidget(copy);
        boxLayout->addLayout(addressRow);
        layout->addWidget(box);

        auto* tunnel = PlayitTunnel::instance();
        connect(m_playit, &QCheckBox::toggled, this, [this](bool on) {
            m_server->setPlayitEnabled(on);
            updatePlayit();
        });
        connect(m_playitInstall, &QPushButton::clicked, tunnel, &PlayitTunnel::install);
        connect(m_playitClaim, &QPushButton::clicked, this, [tunnel] { DesktopServices::openUrl(QUrl(tunnel->claimUrl())); });
        connect(m_playitAddress, &QLineEdit::editingFinished, this, [this, tunnel] {
            if (m_playitAddress->text().trimmed() != tunnel->address())
                tunnel->setManualAddress(m_playitAddress->text());
        });
        connect(copy, &QPushButton::clicked, this, [this] { QGuiApplication::clipboard()->setText(m_playitAddress->text().trimmed()); });
        connect(tunnel, &PlayitTunnel::stateChanged, this, &ServerDashboardPage::updatePlayit);
        connect(tunnel, &PlayitTunnel::addressChanged, this, &ServerDashboardPage::updatePlayit);
        connect(tunnel, &PlayitTunnel::message, this, &ServerDashboardPage::appendConsole);
        updatePlayit();
    }

    // mods (left) + console (right)
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    auto* modsPane = new QWidget(splitter);
    auto* modsLayout = new QVBoxLayout(modsPane);
    modsLayout->setContentsMargins(0, 0, 0, 0);
    m_mods = new QTreeWidget(modsPane);
    m_mods->setColumnCount(2);
    m_mods->setHeaderLabels({ tr("Mod"), tr("Why") });
    m_mods->setRootIsDecorated(false);
    m_mods->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    modsLayout->addWidget(m_mods);
    m_modsHint = new QLabel(tr("Untick a mod to leave it out. Changes apply the next time you press Rebuild."), modsPane);
    m_modsHint->setWordWrap(true);
    modsLayout->addWidget(m_modsHint);

    auto* consolePane = new QWidget(splitter);
    auto* consoleLayout = new QVBoxLayout(consolePane);
    consoleLayout->setContentsMargins(0, 0, 0, 0);
    m_console = new QPlainTextEdit(consolePane);
    m_console->setReadOnly(true);
    m_console->setMaximumBlockCount(5000);
    m_console->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_console->setLineWrapMode(QPlainTextEdit::NoWrap);
    consoleLayout->addWidget(m_console);
    m_command = new QLineEdit(consolePane);
    m_command->setPlaceholderText(tr("Type a server command (e.g. \"op YourName\", \"list\", \"say hi\") and press Enter"));
    consoleLayout->addWidget(m_command);
    splitter->addWidget(modsPane);
    splitter->addWidget(consolePane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    layout->addWidget(splitter, 1);

    // wiring
    connect(m_setupButton, &QPushButton::clicked, this, [this] { m_server->setup(m_eula->isChecked()); });
    connect(m_startButton, &QPushButton::clicked, this, [this] {
        m_server->setEulaAccepted(m_eula->isChecked());
        m_server->start();
    });
    connect(m_stopButton, &QPushButton::clicked, m_server, &FriendServer::stop);
    connect(m_folderButton, &QPushButton::clicked, this, [this] {
        FS::ensureFolderPathExists(m_server->serverDir());
        DesktopServices::openPath(m_server->serverDir(), true);
    });
    connect(m_port, &QSpinBox::valueChanged, this, [this](int port) {
        m_server->setPort(port);
        m_join->setText(joinHint());
    });
    connect(m_memory, &QSpinBox::valueChanged, m_server, &FriendServer::setMemoryMb);
    connect(m_eula, &QCheckBox::toggled, this, &ServerDashboardPage::updateState);
    connect(m_mods, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        if (m_loadingMods || column != 0)
            return;
        m_server->setModIncluded(item->data(0, Qt::UserRole).toString(), item->checkState(0) == Qt::Checked);
        reloadMods();
    });
    connect(m_command, &QLineEdit::returnPressed, this, [this] {
        m_server->sendCommand(m_command->text());
        m_command->clear();
    });
    connect(m_server, &FriendServer::stateChanged, this, &ServerDashboardPage::updateState);
    connect(m_server, &FriendServer::consoleLine, this, &ServerDashboardPage::appendConsole);

    for (const auto& line : m_server->consoleHistory())
        appendConsole(line);
    m_join->setText(joinHint());
    updateState();
}

void ServerDashboardPage::openedImpl()
{
    reloadMods();
    updateState();
}

QString ServerDashboardPage::joinHint() const
{
    QStringList addresses;
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && !address.isLoopback() && !address.isLinkLocal())
            addresses.append(QString("%1:%2").arg(address.toString()).arg(m_port->value()));
    }
    if (addresses.isEmpty())
        return tr("Friends on your network join with your computer's IP and port %1.").arg(m_port->value());
    return tr("Friends on your network can join at: %1. Over the internet they'll need port %2 forwarded on your router, "
              "or the optional playit.gg tunnel below.")
        .arg(addresses.join(", "))
        .arg(m_port->value());
}

void ServerDashboardPage::updateState()
{
    using State = FriendServer::State;
    const State state = m_server->state();
    const QString unsupported = m_server->unsupportedReason();

    QString stateText;
    switch (state) {
        case State::NotSetUp:
            stateText = tr("Not set up");
            break;
        case State::SettingUp:
            stateText = tr("Setting up...");
            break;
        case State::Stopped:
            stateText = tr("Stopped");
            break;
        case State::Starting:
            stateText = tr("Starting...");
            break;
        case State::Running:
            stateText = tr("Running");
            break;
        case State::Stopping:
            stateText = tr("Stopping...");
            break;
    }
    QString details = QString("%1 %2").arg(FriendServer::loaderName(m_server->loader()), m_server->minecraftVersion());
    if (!unsupported.isEmpty())
        details = unsupported;
    m_status->setText(QString("<b>%1</b> &middot; %2").arg(stateText, details.toHtmlEscaped()));

    const bool busy = state == State::SettingUp || m_server->isRunning();
    m_setupButton->setText(state == State::NotSetUp ? tr("Set Up") : tr("Rebuild"));
    m_setupButton->setEnabled(!busy && unsupported.isEmpty());
    m_startButton->setEnabled(!busy && unsupported.isEmpty() && m_eula->isChecked());
    m_startButton->setToolTip(m_eula->isChecked() ? QString() : tr("Accept the Minecraft EULA first."));
    m_stopButton->setEnabled(state == State::Running || state == State::Starting);
    m_port->setEnabled(!m_server->isRunning());
    m_memory->setEnabled(!m_server->isRunning());
    m_command->setEnabled(state == State::Running || state == State::Starting);
}

void ServerDashboardPage::updatePlayit()
{
    using WraithServer::PlayitTunnel;
    auto* tunnel = PlayitTunnel::instance();
    const auto state = tunnel->state();
    const QString version = tunnel->agentVersion();
    QString text;
    switch (state) {
        case PlayitTunnel::State::Unsupported:
            text = tr("Available on Windows for now.");
            break;
        case PlayitTunnel::State::NotInstalled:
            text = m_playit->isChecked() ? tr("Install the agent to use it.") : QString();
            break;
        case PlayitTunnel::State::Installing:
            text = tr("Downloading and verifying the agent...");
            break;
        case PlayitTunnel::State::Ready:
            text = tr("Agent %1 installed; starts with the server.").arg(version);
            break;
        case PlayitTunnel::State::Running:
            text = tr("Tunnel agent running (its window must stay open).");
            break;
    }
    m_playitStatus->setText(text);
    m_playit->setEnabled(state != PlayitTunnel::State::Unsupported);
    m_playitInstall->setVisible(m_playit->isChecked() && state != PlayitTunnel::State::Unsupported && state != PlayitTunnel::State::Running);
    m_playitInstall->setEnabled(state != PlayitTunnel::State::Installing);
    m_playitInstall->setText(state == PlayitTunnel::State::NotInstalled ? tr("Install agent") : tr("Update agent"));
    m_playitClaim->setVisible(!tunnel->claimUrl().isEmpty());
    if (!m_playitAddress->hasFocus())
        m_playitAddress->setText(tunnel->address());
    m_playitAddress->setEnabled(m_playit->isChecked());
}

void ServerDashboardPage::reloadMods()
{
    m_loadingMods = true;
    m_mods->clear();
    int excluded = 0;
    const auto entries = m_server->mods();
    for (const auto& mod : entries) {
        auto* item = new QTreeWidgetItem(m_mods, { mod.name, WraithServer::describe(mod.reason) });
        item->setData(0, Qt::UserRole, mod.fileName);
        item->setToolTip(0, mod.fileName);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, mod.include ? Qt::Checked : Qt::Unchecked);
        if (!mod.include)
            excluded++;
    }
    m_modsHint->setText(entries.isEmpty() ? tr("This instance has no mods.")
                                          : tr("%1 of %2 mods go on the server. Untick a mod to leave it out; changes apply the next "
                                               "time you press Rebuild.")
                                                .arg(entries.size() - excluded)
                                                .arg(entries.size()));
    m_loadingMods = false;
}

void ServerDashboardPage::appendConsole(const QString& line)
{
    auto* bar = m_console->verticalScrollBar();
    const bool atBottom = bar->value() >= bar->maximum() - 4;
    m_console->appendPlainText(line);
    if (atBottom)
        bar->setValue(bar->maximum());
}
