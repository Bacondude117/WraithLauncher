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
#include "WraithBackend.h"

#include <QAction>
#include <QBuffer>
#include <QCursor>
#include <QFile>
#include <QFileDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QMenu>
#include <QTimer>
#include <QUrl>

#include <QDir>

#include "Application.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "server/FriendServer.h"
#include "icons/IconList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/auth/AccountList.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"
#include "ui/themes/ThemeManager.h"

#include "WraithInstanceModel.h"
#include "WraithShell.h"

namespace Wraith {

ImageProvider::ImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}

QImage ImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    const QSize target = requestedSize.isValid() && !requestedSize.isEmpty() ? requestedSize : QSize(64, 64);
    QImage image;
    if (id.startsWith("icon/")) {
        QString key = QUrl::fromPercentEncoding(id.mid(5).section('?', 0, 0).toUtf8());
        image = APPLICATION->icons()->getIcon(key).pixmap(target).toImage();
    } else if (id.startsWith("glyph/")) {
        // glyph/<name>/<hex colour without #>
        const QStringList parts = id.mid(6).section('?', 0, 0).split('/');
        QFile file(QString(":/wraith/icons/%1.svg").arg(parts.value(0)));
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray svg = file.readAll();
            svg.replace("currentColor", ("#" + parts.value(1, "e8eaf2")).toUtf8());
            QBuffer buffer(&svg);
            QImageReader reader(&buffer, "svg");
            reader.setScaledSize(target);
            image = reader.read();
        }
    }
    if (size)
        *size = image.size();
    return image;
}

Backend::Backend(Shell* shell, MainWindow* window) : QObject(shell), m_shell(shell), m_window(window), m_instances(new InstanceModel(this))
{
    auto bump = [this] {
        m_revision++;
        emit instancesUpdated();
    };
    auto* list = APPLICATION->instances();
    connect(list, &QAbstractItemModel::dataChanged, this, bump);
    connect(list, &QAbstractItemModel::rowsInserted, this, bump);
    connect(list, &QAbstractItemModel::rowsRemoved, this, bump);
    connect(list, &QAbstractItemModel::modelReset, this, bump);
    connect(m_instances, &QAbstractItemModel::dataChanged, this, bump);
    connect(APPLICATION->icons(), &IconList::iconUpdated, this, [this, bump] {
        m_instances->iconsChanged();
        bump();
    });

    auto accounts = APPLICATION->accounts();
    connect(accounts, &AccountList::defaultAccountChanged, this, &Backend::accountChanged);
    connect(accounts, &AccountList::listChanged, this, &Backend::accountChanged);
    connect(APPLICATION, &Application::currentCatChanged, this, &Backend::catChanged);
    connect(APPLICATION->settings()->getSetting("TheCat").get(), &Setting::SettingChanged, this, &Backend::catChanged);
}

QObject* Backend::instances() const
{
    return m_instances;
}

void Backend::setCurrentPage(const QString& page)
{
    if (m_currentPage == page)
        return;
    m_currentPage = page;
    emit currentPageChanged();
}

int Backend::instanceCount() const
{
    return APPLICATION->instances()->count();
}

QStringList Backend::groups() const
{
    QStringList groups = APPLICATION->instances()->getGroups();
    groups.removeAll(QString());
    groups.sort(Qt::CaseInsensitive);
    return groups;
}

QString Backend::totalPlayTime() const
{
    qint64 total = 0;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++)
        total += list->at(i)->totalTimePlayed();
    return playTimeText(total);
}

int Backend::runningCount() const
{
    int running = 0;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++)
        running += list->at(i)->isRunning() ? 1 : 0;
    return running;
}

QString Backend::accountName() const
{
    auto account = APPLICATION->accounts()->defaultAccount();
    return account ? account->profileName() : QString();
}

QString Backend::accountType() const
{
    auto account = APPLICATION->accounts()->defaultAccount();
    if (!account)
        return tr("Not signed in");
    return account->accountType() == AccountType::MSA ? tr("Microsoft account") : tr("Offline account");
}

QString Backend::catImage() const
{
    if (!APPLICATION->settings()->get("TheCat").toBool())
        return {};
    const QString cat = APPLICATION->themeManager()->getCatPack();
    if (cat.isEmpty())
        return {};
    return cat.startsWith(":") ? "qrc" + cat : QUrl::fromLocalFile(cat).toString();
}

QString Backend::displayName() const
{
    return BuildConfig.LAUNCHER_DISPLAYNAME;
}

MinecraftInstance* Backend::instanceById(const QString& id) const
{
    return APPLICATION->instances()->getInstanceById(id);
}

void Backend::navigate(const QString& page)
{
    m_shell->showPage(page);
}

void Backend::openInstance(const QString& id, const QString& tab)
{
    if (auto* instance = instanceById(id))
        m_shell->showInstance(instance, tab);
}

void Backend::browse(const QString& query)
{
    m_shell->browse(query);
}

QVariantList Backend::recentInstances(int count) const
{
    QList<MinecraftInstance*> instances;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++)
        instances.append(list->at(i));
    std::stable_sort(instances.begin(), instances.end(),
                     [](MinecraftInstance* a, MinecraftInstance* b) { return a->lastLaunch() > b->lastLaunch(); });
    QVariantList result;
    for (auto* instance : instances) {
        if (result.size() >= count)
            break;
        result.append(instanceInfo(instance));
    }
    return result;
}

QVariantMap Backend::instance(const QString& id) const
{
    return instanceInfo(instanceById(id));
}

QString Backend::glyph(const QString& name, const QString& color) const
{
    QString hex = color;
    hex.remove('#');
    return QString("image://wraith/glyph/%1/%2").arg(name, hex);
}

QAction* Backend::action(const char* name) const
{
    return m_window->findChild<QAction*>(name);
}

MinecraftInstance* Backend::select(const QString& id) const
{
    m_window->wraithSelectInstance(id);
    return instanceById(id);
}

void Backend::launch(const QString& id)
{
    if (auto* instance = instanceById(id)) {
        if (!instance->isRunning())
            APPLICATION->launch(instance);
    }
}

void Backend::stop(const QString& id)
{
    if (auto* instance = instanceById(id))
        APPLICATION->kill(instance);
}

void Backend::openFolder(const QString& id)
{
    if (auto* instance = instanceById(id))
        DesktopServices::openPath(instance->instanceRoot(), true);
}

void Backend::rename(const QString& id)
{
    auto* instance = select(id);
    if (!instance)
        return;
    bool ok = false;
    const QString name = QInputDialog::getText(m_shell, tr("Rename instance"), tr("New name:"), QLineEdit::Normal, instance->name(), &ok);
    if (ok && !name.trimmed().isEmpty() && name != instance->name())
        m_window->wraithRenameInstance(instance, name.trimmed());
}

void Backend::changeIcon(const QString& id)
{
    if (select(id))
        action("actionChangeInstIcon")->trigger();
}

void Backend::changeGroup(const QString& id)
{
    if (select(id))
        action("actionChangeInstGroup")->trigger();
}

void Backend::duplicate(const QString& id)
{
    if (select(id))
        action("actionCopyInstance")->trigger();
}

void Backend::remove(const QString& id)
{
    auto* instance = select(id);
    if (!instance)
        return;
    const QString name = instance->name();
    action("actionDeleteInstance")->trigger();
    if (instanceById(id))
        return;  // cancelled, or it is still running
    auto* undo = action("actionUndoTrashInstance");
    if (undo && undo->isEnabled())
        m_shell->toast(tr("Moved \"%1\" to the trash").arg(name), "info", tr("Undo"), [undo] { undo->trigger(); });
    else
        m_shell->toast(tr("Deleted \"%1\"").arg(name));
}

void Backend::popup(QMenu* menu)
{
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(QCursor::pos());
}

void Backend::showInstanceMenu(const QString& id)
{
    auto* instance = select(id);
    if (!instance)
        return;
    auto* menu = new QMenu(m_shell);
    menu->addSection(instance->name());
    if (instance->isRunning())
        menu->addAction(tr("Stop"), this, [this, id] { stop(id); });
    else
        menu->addAction(tr("Play"), this, [this, id] { launch(id); });
    menu->addAction(tr("Open"), this, [this, id] { openInstance(id); });
    menu->addAction(tr("Open folder"), this, [this, id] { openFolder(id); });
    menu->addSeparator();
    menu->addAction(tr("Rename..."), this, [this, id] { rename(id); });
    menu->addAction(tr("Change icon..."), this, [this, id] { changeIcon(id); });
    menu->addAction(tr("Change group..."), this, [this, id] { changeGroup(id); });
    menu->addSeparator();
    menu->addAction(tr("Duplicate..."), this, [this, id] { duplicate(id); });
    if (auto* exportMenu = action("actionExportInstance")->menu())
        menu->addMenu(exportMenu)->setText(tr("Export"));
    menu->addAction(action("actionCreateInstanceShortcut"));
    menu->addSeparator();
    auto* deleteAction = menu->addAction(tr("Delete..."), this, [this, id] { remove(id); });
    deleteAction->setEnabled(!instance->isRunning());
    popup(menu);
}

void Backend::showLaunchMenu(const QString& id)
{
    auto* instance = instanceById(id);
    if (!instance)
        return;
    auto* menu = new QMenu(m_shell);
    instance->populateLaunchMenu(menu);
    if (instance->isRunning()) {
        menu->addSeparator();
        menu->addAction(tr("Restart"), this, [instance] {
            // kill, then launch again once the instance has really stopped
            auto connection = std::make_shared<QMetaObject::Connection>();
            *connection = connect(instance, &BaseInstance::runningStatusChanged, instance, [instance, connection](bool running) {
                if (!running) {
                    QObject::disconnect(*connection);
                    QTimer::singleShot(0, instance, [instance] { APPLICATION->launch(instance); });
                }
            });
            APPLICATION->kill(instance);
        });
    }
    popup(menu);
}

void Backend::showGroupMenu(const QString& group)
{
    if (group.isEmpty())
        return;
    auto* menu = new QMenu(m_shell);
    menu->addSection(group);
    menu->addAction(tr("New instance in this group"), this, [this, group] { newInstance(group); });
    menu->addAction(tr("Rename group..."), this, [this, group] {
        bool ok = false;
        const QString name = QInputDialog::getText(m_shell, tr("Rename group"), tr("New name:"), QLineEdit::Normal, group, &ok);
        if (ok && !name.trimmed().isEmpty())
            APPLICATION->instances()->renameGroup(group, name.trimmed());
    });
    menu->addAction(tr("Delete group"), this, [group] { APPLICATION->instances()->deleteGroup(group); });
    popup(menu);
}

QVariantList Backend::friendServers()
{
    using WraithServer::FriendServer;
    QVariantList result;
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++) {
        auto* instance = list->at(i);
        if (!QDir(FS::PathCombine(instance->instanceRoot(), "wraith-server")).exists())
            continue;
        auto* server = FriendServer::forInstance(instance);
        connect(server, &FriendServer::stateChanged, this, &Backend::serversChanged, Qt::UniqueConnection);
        if (!server->property("wraithToasts").toBool()) {
            // tell the user when a server comes up or goes down, wherever they are in the launcher
            server->setProperty("wraithToasts", true);
            auto last = std::make_shared<FriendServer::State>(server->state());
            connect(server, &FriendServer::stateChanged, this, [this, server, instance, last] {
                const auto now = server->state();
                if (now == FriendServer::State::Running && *last != now)
                    m_shell->toast(tr("%1 server is online on port %2").arg(instance->name()).arg(server->port()), "success");
                else if (now == FriendServer::State::Stopped && *last != FriendServer::State::SettingUp && *last != now)
                    m_shell->toast(tr("%1 server stopped").arg(instance->name()));
                else if (now == FriendServer::State::Stopped && *last == FriendServer::State::SettingUp)
                    m_shell->toast(tr("%1 server is set up").arg(instance->name()), "success");
                *last = now;
            });
        }
        QString state;
        switch (server->state()) {
            case FriendServer::State::NotSetUp:
                state = tr("Not set up");
                break;
            case FriendServer::State::SettingUp:
                state = tr("Setting up");
                break;
            case FriendServer::State::Stopped:
                state = tr("Offline");
                break;
            case FriendServer::State::Starting:
                state = tr("Starting");
                break;
            case FriendServer::State::Running:
                state = tr("Online");
                break;
            case FriendServer::State::Stopping:
                state = tr("Stopping");
                break;
        }
        auto info = instanceInfo(instance);
        info.insert("loader", FriendServer::loaderName(server->loader()));
        info.insert("minecraft", server->minecraftVersion());
        info.insert("port", server->port());
        info.insert("state", state);
        info.insert("online", server->state() == FriendServer::State::Running);
        info.insert("serverRunning", server->isRunning());
        info.insert("busy", server->state() == FriendServer::State::SettingUp || server->state() == FriendServer::State::Starting ||
                                server->state() == FriendServer::State::Stopping);
        info.insert("ready", server->state() != FriendServer::State::NotSetUp && server->state() != FriendServer::State::SettingUp);
        result.append(info);
    }
    return result;
}

void Backend::startServer(const QString& id)
{
    auto* instance = instanceById(id);
    if (!instance)
        return;
    auto* server = WraithServer::FriendServer::forInstance(instance);
    if (!server->eulaAccepted()) {
        // the EULA is accepted on the server's own page, never implicitly
        openInstance(id, "friend");
        m_shell->toast(tr("Accept the Minecraft EULA to start the server."), "warning");
        return;
    }
    server->start();
}

void Backend::stopServer(const QString& id)
{
    if (auto* instance = instanceById(id))
        WraithServer::FriendServer::forInstance(instance)->stop();
}

void Backend::newServer(const QString& id)
{
    openInstance(id, "friend");
}

void Backend::newInstance(const QString& group)
{
    m_shell->showNewInstance(group);
}

void Backend::advancedNewInstance(const QString& pageId)
{
    m_window->wraithNewInstanceDialog({}, pageId);
}

void Backend::importInstance()
{
    const QString file = QFileDialog::getOpenFileName(m_shell, tr("Import instance"), QString(),
                                                      tr("Instances and modpacks (*.zip *.mrpack *.7z *.tar.gz);;All files (*)"));
    if (!file.isEmpty())
        m_window->processURLs({ QUrl::fromLocalFile(file) });
}

void Backend::showAccountMenu()
{
    if (auto* menu = m_window->findChild<QMenu*>("accountsMenu"))
        menu->popup(QCursor::pos());
}

void Backend::showMoreMenu()
{
    auto* menu = new QMenu(m_shell);
    if (auto* folders = m_window->findChild<QMenu*>("foldersMenu"))
        menu->addMenu(folders)->setText(tr("Folders"));
    if (auto* help = m_window->findChild<QMenu*>("helpMenu"))
        menu->addMenu(help)->setText(tr("Help"));
    menu->addSeparator();
    menu->addAction(action("actionUndoTrashInstance"));
    if (auto* update = action("actionCheckUpdate"); update && update->isVisible())
        menu->addAction(update);
    menu->addAction(action("actionCAT"));
    menu->addSeparator();
    menu->addAction(tr("About %1").arg(BuildConfig.LAUNCHER_DISPLAYNAME), this, [this] { action("actionAbout")->trigger(); });
    menu->addAction(tr("Quit"), APPLICATION, &Application::quit);
    popup(menu);
}

void Backend::toast(const QString& text, const QString& kind)
{
    m_shell->toast(text, kind);
}

void Backend::openUrl(const QString& url)
{
    DesktopServices::openUrl(QUrl(url));
}

void Backend::dropUrls(const QVariantList& urls)
{
    QList<QUrl> list;
    for (const auto& url : urls)
        list.append(url.toUrl());
    if (!list.isEmpty())
        m_window->processURLs(list);
}

}  // namespace Wraith
