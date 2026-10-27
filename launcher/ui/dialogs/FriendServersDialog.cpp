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
#include "FriendServersDialog.h"

#include <QDir>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "Application.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "server/FriendServer.h"

using WraithServer::FriendServer;

namespace {
bool hasServer(MinecraftInstance* instance)
{
    return QDir(FS::PathCombine(instance->instanceRoot(), "wraith-server")).exists();
}

QString stateText(FriendServer::State state)
{
    switch (state) {
        case FriendServer::State::NotSetUp:
            return QObject::tr("Not set up");
        case FriendServer::State::SettingUp:
            return QObject::tr("Setting up...");
        case FriendServer::State::Stopped:
            return QObject::tr("Offline");
        case FriendServer::State::Starting:
            return QObject::tr("Starting...");
        case FriendServer::State::Running:
            return QObject::tr("Online");
        case FriendServer::State::Stopping:
            return QObject::tr("Stopping...");
    }
    return {};
}
}  // namespace

FriendServersDialog::FriendServersDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Friend Servers"));
    resize(820, 560);
    auto* layout = new QVBoxLayout(this);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel(tr("Friend Servers"), this);
    title->setObjectName("wraithTitle");
    header->addWidget(title);
    header->addStretch(1);
    m_search = new QLineEdit(this);
    m_search->setObjectName("wraithSearch");
    m_search->setPlaceholderText(tr("Search servers..."));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedWidth(260);
    header->addWidget(m_search);
    auto* newButton = new QPushButton(QIcon::fromTheme("new"), tr("New server"), this);
    newButton->setObjectName("wraithAccentButton");
    header->addWidget(newButton);
    layout->addLayout(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* listWidget = new QWidget(scroll);
    m_list = new QVBoxLayout(listWidget);
    m_list->setContentsMargins(0, 8, 0, 0);
    m_list->setSpacing(10);
    scroll->setWidget(listWidget);
    layout->addWidget(scroll, 1);

    connect(m_search, &QLineEdit::textChanged, this, &FriendServersDialog::rebuild);
    connect(newButton, &QPushButton::clicked, this, &FriendServersDialog::newServer);
    rebuild();
}

void FriendServersDialog::rebuild()
{
    while (auto* item = m_list->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();  // may be called from one of these widgets' own click handlers
        delete item;
    }

    auto* instances = APPLICATION->instances();
    int shown = 0;
    for (int i = 0; i < instances->count(); i++) {
        auto* instance = instances->at(i);
        if (!hasServer(instance) || !instance->name().contains(m_search->text(), Qt::CaseInsensitive))
            continue;
        auto* server = FriendServer::forInstance(instance);
        shown++;

        auto* card = new QFrame();
        card->setObjectName("wraithCard");
        auto* row = new QHBoxLayout(card);
        row->setContentsMargins(14, 12, 14, 12);
        row->setSpacing(14);

        auto* icon = new QLabel(card);
        icon->setPixmap(APPLICATION->icons()->getIcon(instance->iconKey()).pixmap(56, 56));
        row->addWidget(icon);

        auto* text = new QVBoxLayout();
        auto* name = new QLabel(QString("<b style='font-size:13pt'>%1</b>").arg(instance->name().toHtmlEscaped()), card);
        const bool online = server->state() == FriendServer::State::Running;
        auto* details = new QLabel(QString("%1 %2  &middot;  %3 %4  &middot;  <span style='color:%5'>&#9679; %6</span>")
                                       .arg(FriendServer::loaderName(server->loader()), server->minecraftVersion().toHtmlEscaped(),
                                            tr("port"), QString::number(server->port()), online ? "#5fd38d" : "#7a8091",
                                            stateText(server->state())),
                                   card);
        text->addWidget(name);
        text->addWidget(details);
        row->addLayout(text, 1);

        auto* toggle = new QPushButton(server->isRunning() ? tr("Stop") : tr("Start"), card);
        toggle->setObjectName(server->isRunning() ? QString() : QString("wraithAccentButton"));
        toggle->setEnabled(server->state() != FriendServer::State::SettingUp && server->state() != FriendServer::State::NotSetUp);
        connect(toggle, &QPushButton::clicked, this, [server, instance] {
            if (server->isRunning())
                server->stop();
            else if (!server->eulaAccepted())
                APPLICATION->showInstanceWindow(instance, "friend_server");  // the EULA is accepted on the dashboard
            else
                server->start();
        });
        auto* manage = new QPushButton(tr("Manage"), card);
        connect(manage, &QPushButton::clicked, this, [instance] { APPLICATION->showInstanceWindow(instance, "friend_server"); });
        row->addWidget(toggle);
        row->addWidget(manage);

        connect(server, &FriendServer::stateChanged, this, &FriendServersDialog::rebuild,
                static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        m_list->addWidget(card);
    }

    if (shown == 0) {
        auto* empty = new QLabel(m_search->text().isEmpty()
                                     ? tr("No friend servers yet. Press <b>New server</b> to turn one of your instances into a server.")
                                     : tr("No servers match your search."));
        empty->setAlignment(Qt::AlignCenter);
        empty->setWordWrap(true);
        m_list->addWidget(empty);
    }
    m_list->addStretch(1);
}

void FriendServersDialog::newServer()
{
    auto* instances = APPLICATION->instances();
    QStringList names;
    QList<MinecraftInstance*> candidates;
    for (int i = 0; i < instances->count(); i++) {
        candidates.append(instances->at(i));
        names.append(instances->at(i)->name());
    }
    if (candidates.isEmpty()) {
        QMessageBox::information(this, tr("New server"), tr("Create an instance first, then turn it into a server."));
        return;
    }
    bool ok = false;
    const QString chosen =
        QInputDialog::getItem(this, tr("New server"), tr("Which instance should become a server?"), names, 0, false, &ok);
    if (!ok)
        return;
    auto* instance = candidates.at(names.indexOf(chosen));
    APPLICATION->showInstanceWindow(instance, "friend_server");
    accept();
}
