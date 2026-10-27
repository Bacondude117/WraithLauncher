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

#include "WraithExtras.h"

#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QUrl>

#include <memory>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
#include "advanced/ForeignImportTask.h"
#include "minecraft/MinecraftInstance.h"
#include "net/ApiRequest.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"

#include "WraithBackend.h"
#include "WraithInstanceModel.h"
#include "WraithShell.h"

namespace Wraith {

// ---------------------------------------------------------------- import from other launchers

ImportModel::ImportModel(Shell* shell) : QObject(shell), m_shell(shell) {}

void ImportModel::scan()
{
    if (m_locations.isEmpty())
        m_locations = Advanced::defaultLocations();
    m_found.clear();
    const QString ownData = QDir(APPLICATION->dataRoot()).canonicalPath();
    for (const auto& location : m_locations) {
        if (QDir(location.root).canonicalPath() == ownData)
            continue;  // Wraith's own folder
        m_found.append(Advanced::scanLauncher(location));
    }
    emit changed();
}

QStringList ImportModel::launchers() const
{
    QStringList names;
    for (const auto& l : m_locations)
        if (!names.contains(l.launcher))
            names.append(l.launcher);
    return names;
}

QVariantList ImportModel::instances() const
{
    QVariantList list;
    for (const auto& i : m_found) {
        QString loader = i.loader.isEmpty() ? QString("Vanilla") : i.loader;
        loader[0] = loader[0].toUpper();
        if (loader == "Neoforge")
            loader = "NeoForge";
        if (!i.prismFormat && i.minecraft.isEmpty())
            loader = tr("Unknown");
        list.append(QVariantMap{ { "launcher", i.launcher },
                                 { "name", i.name },
                                 { "path", QDir::toNativeSeparators(i.path) },
                                 { "minecraft", i.minecraft },
                                 { "loader", loader },
                                 // without a Minecraft version a non-Prism instance can't be recreated
                                 { "importable", i.prismFormat || !i.minecraft.isEmpty() } });
    }
    return list;
}

void ImportModel::addFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(m_shell->window(), tr("Another launcher's data folder (with instances/, Instances/ or profiles/)"));
    if (folder.isEmpty())
        return;
    // try every format on the folder the user picked
    for (const char* launcher : { "Prism Launcher", "CurseForge", "Modrinth App", "ATLauncher", "GDLauncher", "Minecraft Launcher" })
        m_locations.append({ launcher, folder });
    scan();
}

void ImportModel::importSelected(const QVariantList& indexes)
{
    int started = 0;
    for (const auto& v : indexes) {
        const int index = v.toInt();
        if (index < 0 || index >= m_found.size())
            continue;
        const auto source = m_found.at(index);
        if (!source.prismFormat && source.minecraft.isEmpty())
            continue;
        auto* task = new ForeignImportTask(source);
        task->setName(source.name);
        task->setIcon("default");
        task->setGroup(APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString());
        m_shell->window()->wraithInstallFromTask(task);  // shows progress; returns when done
        started++;
    }
    if (started > 0)
        m_shell->toast(tr("Copied %n instance(s); the other launcher's files are unchanged.", "", started), "success");
}

// ---------------------------------------------------------------- screenshot gallery

GalleryModel::GalleryModel(Shell* shell) : QAbstractListModel(shell), m_shell(shell) {}

QHash<int, QByteArray> GalleryModel::roleNames() const
{
    return { { PathRole, "path" }, { UrlRole, "url" }, { InstanceRole, "instance" }, { InstanceIdRole, "instanceId" }, { DateRole, "date" },
             { NameRole, "fileName" } };
}

QVariant GalleryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const auto& s = m_all.at(m_visible.at(index.row()));
    switch (role) {
        case PathRole:
            return s.path;
        case UrlRole:
            return QUrl::fromLocalFile(s.path);
        case InstanceRole:
            return s.instance;
        case InstanceIdRole:
            return s.instanceId;
        case DateRole:
            return relativeTime(s.date.toMSecsSinceEpoch());
        case NameRole:
            return QFileInfo(s.path).fileName();
        default:
            return {};
    }
}

void GalleryModel::refresh()
{
    m_all.clear();
    auto* list = APPLICATION->instances();
    for (int i = 0; i < list->count(); i++) {
        auto* instance = list->at(i);
        const QDir dir(QDir(instance->gameRoot()).filePath("screenshots"));
        for (const auto& f : dir.entryInfoList({ "*.png", "*.jpg", "*.jpeg" }, QDir::Files))
            m_all.append({ f.absoluteFilePath(), instance->name(), instance->id(), f.lastModified() });
    }
    std::sort(m_all.begin(), m_all.end(), [](const Shot& a, const Shot& b) { return a.date > b.date; });
    rebuild();
}

void GalleryModel::rebuild()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_all.size(); i++)
        if (m_filter.isEmpty() || m_all[i].instance == m_filter)
            m_visible.append(i);
    endResetModel();
    emit changed();
}

QStringList GalleryModel::instanceNames() const
{
    QStringList names;
    for (const auto& s : m_all)
        if (!names.contains(s.instance))
            names.append(s.instance);
    names.sort(Qt::CaseInsensitive);
    return names;
}

void GalleryModel::setInstanceFilter(const QString& name)
{
    if (m_filter == name)
        return;
    m_filter = name;
    rebuild();
}

void GalleryModel::openFile(int row)
{
    if (row >= 0 && row < m_visible.size())
        DesktopServices::openPath(m_all.at(m_visible.at(row)).path);
}

void GalleryModel::showInFolder(int row)
{
    if (row >= 0 && row < m_visible.size())
        DesktopServices::openPath(QFileInfo(m_all.at(m_visible.at(row)).path).absolutePath());
}

void GalleryModel::copyImage(int row)
{
    if (row < 0 || row >= m_visible.size())
        return;
    QGuiApplication::clipboard()->setImage(QImage(m_all.at(m_visible.at(row)).path));
    m_shell->toast(tr("Image copied"), "success");
}

void GalleryModel::remove(int row)
{
    if (row < 0 || row >= m_visible.size())
        return;
    const QString path = m_all.at(m_visible.at(row)).path;
    if (QMessageBox::question(m_shell->window(), tr("Delete screenshot"), tr("Delete %1?").arg(QFileInfo(path).fileName()),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    if (!FS::trash(path) && !QFile::remove(path)) {
        m_shell->toast(tr("Couldn't delete %1").arg(path), "error");
        return;
    }
    refresh();
}

// ---------------------------------------------------------------- modpack update notifications

UpdateNotifier::UpdateNotifier(Shell* shell) : QObject(shell), m_shell(shell)
{
    m_timer.setInterval(6 * 60 * 60 * 1000);
    connect(&m_timer, &QTimer::timeout, this, [this] { checkNow(false); });
    if (APPLICATION->settings()->get("WraithUpdateNotifications").toBool()) {
        QTimer::singleShot(15000, this, [this] { checkNow(true); });
        m_timer.start();
    }
}

void UpdateNotifier::checkNow(bool announce)
{
    if (m_job)
        return;
    struct Pending {
        QPointer<BaseInstance> instance;
        QString installed;
        QString type;
        QByteArray* response;
    };
    auto pending = std::make_shared<QList<Pending>>();
    m_job = makeShared<NetJob>("Wraith::PackUpdates", APPLICATION->network());
    auto* list = APPLICATION->instances();
    const bool curseForge = APPLICATION->capabilities() & Application::SupportsFlame;
    for (int i = 0; i < list->count(); i++) {
        auto* instance = list->at(i);
        if (!instance->isManagedPack() || instance->getManagedPackID().isEmpty())
            continue;
        const QString type = instance->getManagedPackType();
        QUrl url;
        if (type == "modrinth")
            url = QUrl(BuildConfig.MODRINTH_PROD_URL + QString("/project/%1/version").arg(instance->getManagedPackID()));
        else if (type == "flame" && curseForge)
            url = QUrl(BuildConfig.FLAME_BASE_URL + QString("/mods/%1/files?pageSize=1").arg(instance->getManagedPackID()));
        else
            continue;
        auto [action, response] = Net::ApiRequest::makeByteArray(url);
        m_job->addNetAction(action);
        pending->append({ instance, instance->getManagedPackVersionID(), type, response });
    }
    if (pending->isEmpty()) {
        m_job.reset();
        return;
    }
    connect(m_job.get(), &Task::finished, this, [this, pending, announce] {
        m_job.reset();
        QStringList updated;
        for (const auto& p : *pending) {
            if (!p.instance)
                continue;
            QString newestId;
            QString newestName;
            if (p.type == "modrinth") {
                const auto versions = QJsonDocument::fromJson(*p.response).array();
                if (!versions.isEmpty()) {
                    newestId = versions.first().toObject().value("id").toString();
                    newestName = versions.first().toObject().value("version_number").toString();
                }
            } else {
                const auto files = QJsonDocument::fromJson(*p.response).object().value("data").toArray();
                if (!files.isEmpty()) {
                    newestId = QString::number(files.first().toObject().value("id").toInteger());
                    newestName = files.first().toObject().value("displayName").toString();
                }
            }
            const bool available = !newestId.isEmpty() && newestId != p.installed;
            p.instance->setProperty("wraithUpdateAvailable", available ? newestName : QString());
            if (available)
                updated.append(p.instance->name());
        }
        if (auto* model = qobject_cast<InstanceModel*>(m_shell->backend()->instances()))
            model->tagsChanged();
        if (announce && !updated.isEmpty())
            m_shell->toast(tr("Modpack updates available: %1. Update from the instance's Settings → Modpack.").arg(updated.join(", ")), "info",
                           tr("Library"), [this] { m_shell->showPage("library"); });
    });
    m_job->start();
}

}  // namespace Wraith
