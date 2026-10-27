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
#include "WraithNewInstance.h"

#include <QEvent>
#include <QQuickItem>
#include <QQuickWidget>

#include "Application.h"
#include "Version.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"
#include "minecraft/VanillaInstanceCreationTask.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"

#include "WraithBackend.h"
#include "WraithShell.h"

namespace Wraith {

namespace {
QString loaderUid(const QString& loader)
{
    if (loader == "Fabric")
        return "net.fabricmc.fabric-loader";
    if (loader == "Quilt")
        return "org.quiltmc.quilt-loader";
    if (loader == "Forge")
        return "net.minecraftforge";
    if (loader == "NeoForge")
        return "net.neoforged";
    return {};
}
}  // namespace

NewInstanceSheet::NewInstanceSheet(Shell* shell) : QObject(shell), m_shell(shell) {}

void NewInstanceSheet::open(const QString& group)
{
    m_group = group.isEmpty() ? APPLICATION->settings()->get("LastUsedGroupForNewInstance").toString() : group;
    emit groupChanged();
    if (!m_view) {
        m_view = m_shell->createQuick("NewInstanceSheet.qml", m_shell);
        // a translucent overlay over the whole shell
        m_view->setAttribute(Qt::WA_AlwaysStackOnTop);
        m_view->setClearColor(Qt::transparent);
        m_shell->installEventFilter(this);
    }
    m_view->setGeometry(m_shell->rect());
    m_view->show();
    m_view->raise();
    m_view->setFocus();
    if (auto* root = m_view->rootObject())
        QMetaObject::invokeMethod(root, "opened");
    if (m_versions.isEmpty())
        loadVersions();
}

bool NewInstanceSheet::eventFilter(QObject* watched, QEvent* event)
{
    // keep the overlay covering the shell
    if (watched == m_shell && event->type() == QEvent::Resize && m_view)
        m_view->setGeometry(m_shell->rect());
    return QObject::eventFilter(watched, event);
}

void NewInstanceSheet::close()
{
    if (m_view)
        m_view->hide();
}

void NewInstanceSheet::loadVersions()
{
    auto list = APPLICATION->metadataIndex()->get("net.minecraft");
    auto fill = [this, list] {
        // newest first, like the version picker in Prism's dialog
        QList<std::pair<qint64, QVariantMap>> sorted;
        for (int i = 0; i < list->count(); i++) {
            const auto index = list->index(i);
            auto version = std::dynamic_pointer_cast<Meta::Version>(list->at(i));
            sorted.append({ version ? version->rawTime() : 0,
                            QVariantMap{ { "version", list->data(index, BaseVersionList::VersionRole).toString() },
                                         { "type", list->data(index, BaseVersionList::TypeRole).toString() } } });
        }
        std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        m_versions.clear();
        for (const auto& entry : sorted)
            m_versions.append(entry.second);
        m_loading = false;
        emit versionsChanged();
    };
    if (list->isLoaded()) {
        fill();
        return;
    }
    m_loading = true;
    emit versionsChanged();
    m_loadTask = list->getLoadTask();
    connect(m_loadTask.get(), &Task::finished, this, fill);
    if (!m_loadTask->isRunning())
        m_loadTask->start();
}

void NewInstanceSheet::selectLoader(const QString& loader, const QString& minecraft)
{
    m_loader = loader;
    m_minecraft = minecraft;
    m_loaderVersion = nullptr;
    const QString uid = loaderUid(loader);
    if (uid.isEmpty() || minecraft.isEmpty()) {
        m_loaderVersionText.clear();
        m_loaderAvailable = true;
        emit loaderChanged();
        return;
    }
    if ((loader == "Fabric" || loader == "Quilt") && Version(minecraft) < Version("1.14")) {
        m_loaderVersionText = tr("%1 needs Minecraft 1.14 or newer").arg(loader);
        m_loaderAvailable = false;
        emit loaderChanged();
        return;
    }
    auto list = APPLICATION->metadataIndex()->get(uid);
    if (list->isLoaded()) {
        resolveLoader();
        return;
    }
    m_loaderVersionText = tr("Finding the latest %1...").arg(loader);
    m_loaderAvailable = false;
    emit loaderChanged();
    m_loaderTask = list->getLoadTask();
    connect(m_loaderTask.get(), &Task::finished, this, [this, loader, minecraft] {
        if (loader == m_loader && minecraft == m_minecraft)
            resolveLoader();
    });
    if (!m_loaderTask->isRunning())
        m_loaderTask->start();
}

void NewInstanceSheet::resolveLoader()
{
    auto list = APPLICATION->metadataIndex()->get(loaderUid(m_loader));
    if (m_loader == "Fabric" || m_loader == "Quilt") {
        // these loaders work with any supported Minecraft version
        m_loaderVersion = list->getRecommended();
        if (!m_loaderVersion && list->count() > 0)
            m_loaderVersion = list->at(0);
    } else {
        Meta::Version::Ptr version = list->getRecommendedForParent("net.minecraft", m_minecraft);
        if (!version)
            version = list->getLatestForParent("net.minecraft", m_minecraft);
        m_loaderVersion = version;
    }
    m_loaderAvailable = m_loaderVersion != nullptr;
    m_loaderVersionText = m_loaderVersion ? tr("%1 %2 (recommended)").arg(m_loader, m_loaderVersion->descriptor())
                                          : tr("No %1 version for Minecraft %2").arg(m_loader, m_minecraft);
    emit loaderChanged();
}

void NewInstanceSheet::create(const QString& name, const QString& minecraft, const QString& loader, const QString& group)
{
    auto list = APPLICATION->metadataIndex()->get("net.minecraft");
    auto version = list->findVersion(minecraft);
    if (!version)
        return;
    VanillaCreationTask* task = nullptr;
    const bool withLoader = !loaderUid(loader).isEmpty() && m_loaderVersion && loader == m_loader && minecraft == m_minecraft;
    if (withLoader)
        task = new VanillaCreationTask(version, loaderUid(loader), m_loaderVersion);
    else
        task = new VanillaCreationTask(version);
    const QString fallback = withLoader ? QString("%1 %2").arg(loader, minecraft) : minecraft;
    task->setName(name.trimmed().isEmpty() ? fallback : name.trimmed());
    task->setGroup(group.trimmed());
    task->setIcon("default");
    APPLICATION->settings()->set("LastUsedGroupForNewInstance", group.trimmed());
    close();
    m_shell->window()->wraithInstallFromTask(task);
}

void NewInstanceSheet::advanced(const QString& group)
{
    close();
    m_shell->window()->wraithNewInstanceDialog(group);
}

void NewInstanceSheet::importFile()
{
    close();
    m_shell->backend()->importInstance();
}

}  // namespace Wraith
