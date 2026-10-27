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

#include "WraithSnapshots.h"

#include <QClipboard>
#include <QDir>
#include <QEventLoop>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTimer>
#include <QtConcurrentRun>

#include <atomic>
#include <memory>

#include "Application.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/SettingsObject.h"
#include "snapshots/SnapshotDiff.h"
#include "snapshots/SnapshotHooks.h"
#include "ui/MainWindow.h"

#include "WraithBackend.h"
#include "WraithInstanceModel.h"
#include "WraithShell.h"

namespace Wraith {

using namespace Snapshots;

// ---------------------------------------------------------------- service

namespace SnapshotService {

Store storeFor(BaseInstance* instance)
{
    return Store(instance->instanceRoot(), instance->gameRoot());
}

int ringSize()
{
    return APPLICATION->settings()->get("WraithSnapshotRingSize").toInt();
}

QString describeSize(qint64 bytes)
{
    if (bytes < 1024 * 1024)
        return QObject::tr("%1 KB").arg(std::max<qint64>(1, bytes / 1024));
    if (bytes < 1024LL * 1024 * 1024)
        return QObject::tr("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
    return QObject::tr("%1 GB").arg(QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2));
}

Result<Snapshot> takeBlocking(BaseInstance* instance, const Options& options, QWidget* parent)
{
    // The snapshot runs on a worker thread; the change that asked for it waits, but the window keeps painting and a
    // progress dialog appears if it takes more than a moment (the first snapshot of a big instance).
    auto done = std::make_shared<std::atomic<int>>(0);
    auto total = std::make_shared<std::atomic<int>>(0);
    Store store = storeFor(instance);
    auto future = QtConcurrent::run([store, options, done, total]() mutable {
        return store.create(options, [done, total](int d, int t) {
            done->store(d);
            total->store(t);
        });
    });

    QProgressDialog progress(QObject::tr("Taking a snapshot of %1...").arg(instance->name()), QString(), 0, 100, parent);
    progress.setWindowTitle(QObject::tr("Snapshot"));
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setCancelButton(nullptr);
    QTimer tick;
    QObject::connect(&tick, &QTimer::timeout, &progress, [&progress, done, total] {
        const int t = total->load();
        progress.setValue(t > 0 ? done->load() * 100 / t : 0);
    });
    tick.start(100);

    QFutureWatcher<Result<Snapshot>> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<Result<Snapshot>>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    if (!future.isFinished())
        loop.exec();
    tick.stop();
    progress.reset();
    return future.result();
}

void install(Shell* shell)
{
    setBeforeChangeHandler([shell](BaseInstance* instance, Change change) {
        auto settings = APPLICATION->settings();
        if (!settings->get("WraithSnapshotsAuto").toBool())
            return;
        // only instances in the library (not ones being created or imported in a staging folder)
        if (APPLICATION->instances()->getInstanceById(instance->id()) != instance)
            return;
        // one snapshot before a burst of changes: not again for the same instance within five minutes
        static QHash<QString, qint64> s_lastTaken;
        static bool s_running = false;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        if (s_running || now - s_lastTaken.value(instance->id(), 0) < 300)
            return;
        s_lastTaken.insert(instance->id(), now);
        s_running = true;

        const QString trigger = change == Change::Content ? QObject::tr("Before a content change")
                                : change == Change::Modpack ? QObject::tr("Before a modpack update")
                                                            : QObject::tr("Before a Minecraft or loader change");
        Options options;
        options.trigger = trigger;
        options.automatic = true;
        options.includeWorlds = settings->get("WraithSnapshotIncludeWorlds").toBool();
        auto result = takeBlocking(instance, options, shell->window());
        s_running = false;
        if (!result) {
            shell->toast(QObject::tr("Couldn't take a snapshot before the change: %1").arg(result.error()), "warning");
            return;
        }
        storeFor(instance).prune(ringSize());
        const QString id = instance->id();
        shell->toast(QObject::tr("Snapshot saved: %1").arg(trigger), "info", QObject::tr("View"),
                     [shell, id] { shell->backend()->openInstance(id, "snapshots"); });
    });
}

}  // namespace SnapshotService

// ---------------------------------------------------------------- list model

SnapshotListModel::SnapshotListModel(Shell* shell, MinecraftInstance* instance, QObject* parent)
    : QAbstractListModel(parent), m_shell(shell), m_instance(instance)
{}

QHash<int, QByteArray> SnapshotListModel::roleNames() const
{
    return { { IdRole, "snapshotId" },     { TitleRole, "title" },         { TriggerRole, "trigger" },
             { CreatedRole, "createdText" }, { CreatedFullRole, "createdFull" }, { AutomaticRole, "automatic" },
             { PinnedRole, "pinned" },       { WorldsRole, "includesWorlds" }, { FilesRole, "fileCount" },
             { SizeRole, "sizeText" } };
}

QVariant SnapshotListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_snapshots.size())
        return {};
    const auto& s = m_snapshots.at(index.row());
    switch (role) {
        case IdRole:
            return s.id;
        case TitleRole:
            return s.label.isEmpty() ? s.trigger : s.label;
        case TriggerRole:
            return s.trigger;
        case CreatedRole:
            return relativeTime(s.created.toMSecsSinceEpoch());
        case CreatedFullRole:
            return QLocale().toString(s.created, QLocale::ShortFormat);
        case AutomaticRole:
            return s.automatic;
        case PinnedRole:
            return s.pinned;
        case WorldsRole:
            return s.includesWorlds;
        case FilesRole:
            return static_cast<int>(s.files.size());
        case SizeRole:
            return SnapshotService::describeSize(s.totalSize());
        default:
            return {};
    }
}

bool SnapshotListModel::autoEnabled() const
{
    return APPLICATION->settings()->get("WraithSnapshotsAuto").toBool();
}

void SnapshotListModel::setAutoEnabled(bool on)
{
    APPLICATION->settings()->set("WraithSnapshotsAuto", on);
    emit settingsChanged();
}

int SnapshotListModel::ringSize() const
{
    return SnapshotService::ringSize();
}

void SnapshotListModel::setRingSize(int size)
{
    APPLICATION->settings()->set("WraithSnapshotRingSize", std::max(0, size));
    emit settingsChanged();
}

bool SnapshotListModel::includeWorldsByDefault() const
{
    return APPLICATION->settings()->get("WraithSnapshotIncludeWorlds").toBool();
}

void SnapshotListModel::setIncludeWorldsByDefault(bool on)
{
    APPLICATION->settings()->set("WraithSnapshotIncludeWorlds", on);
    emit settingsChanged();
}

QString SnapshotListModel::gameDir() const
{
    return QDir::fromNativeSeparators(QDir(m_instance->instanceRoot()).relativeFilePath(m_instance->gameRoot()));
}

void SnapshotListModel::setBusy(bool busy)
{
    m_busy = busy;
    emit busyChanged();
}

void SnapshotListModel::refresh()
{
    if (!m_instance)
        return;
    auto store = SnapshotService::storeFor(m_instance);
    beginResetModel();
    m_snapshots = store.list();
    endResetModel();
    m_storageText = m_snapshots.isEmpty() ? QString() : tr("%1 on disk").arg(SnapshotService::describeSize(store.storageBytes()));
    emit listChanged();
}

void SnapshotListModel::take(const QString& label, bool includeWorlds)
{
    if (!m_instance || m_busy)
        return;
    setBusy(true);
    Options options;
    options.label = label.trimmed();
    options.trigger = tr("Manual");
    options.includeWorlds = includeWorlds;
    auto result = SnapshotService::takeBlocking(m_instance, options, m_shell->window());
    setBusy(false);
    if (!result) {
        m_shell->toast(tr("Couldn't take a snapshot: %1").arg(result.error()), "error");
        return;
    }
    SnapshotService::storeFor(m_instance).prune(SnapshotService::ringSize());
    refresh();
    m_shell->toast(tr("Snapshot saved"), "success");
}

void SnapshotListModel::setPinned(const QString& id, bool pinned)
{
    SnapshotService::storeFor(m_instance).setPinned(id, pinned);
    refresh();
}

void SnapshotListModel::rename(const QString& id)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return;
    bool ok = false;
    const QString label = QInputDialog::getText(m_shell->window(), tr("Name snapshot"), tr("Name (leave empty to show what triggered it):"),
                                                QLineEdit::Normal, snapshot->label, &ok);
    if (ok) {
        store.setLabel(id, label);
        refresh();
    }
}

void SnapshotListModel::remove(const QString& id)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return;
    const QString title = snapshot->label.isEmpty() ? snapshot->trigger : snapshot->label;
    const auto answer = QMessageBox::question(
        m_shell->window(), tr("Delete snapshot"),
        tr("Delete the snapshot \"%1\" from %2?\n\nYour instance isn't changed; only this saved copy is removed.")
            .arg(title, QLocale().toString(snapshot->created, QLocale::ShortFormat)),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    store.remove(id);
    refresh();
    m_shell->toast(tr("Snapshot deleted"));
}

QVariantMap SnapshotListModel::describe(const Diff& diff, const QString& title) const
{
    const QString game = gameDir();
    QVariantList groups;
    QMap<int, QVariantList> byArea;

    // mods, paired into updated/enabled/disabled
    for (const auto& m : modChanges(diff, game)) {
        static const char* s_kinds[] = { "added", "removed", "updated", "enabled", "disabled" };
        QString detail;
        if (m.kind == ModChange::Kind::Updated)
            detail = tr("was %1").arg(displayName(m.oldFile));
        byArea[static_cast<int>(Area::Mods)].append(QVariantMap{ { "name", m.name }, { "detail", detail }, { "kind", s_kinds[static_cast<int>(m.kind)] } });
    }
    for (const auto& c : groupedChanges(diff, game)) {
        const bool modFile = c.area == Area::Mods && c.path.section('/', game.isEmpty() ? 1 : 2).count('/') == 0;
        if (modFile)
            continue;  // already listed above
        auto& items = byArea[static_cast<int>(c.area)];
        if (items.size() < 300)
            items.append(QVariantMap{ { "name", c.path.section('/', -1) }, { "detail", c.path }, { "kind", c.kind } });
    }
    for (auto it = byArea.begin(); it != byArea.end(); ++it)
        groups.append(QVariantMap{ { "area", areaName(static_cast<Area>(it.key())) }, { "items", it.value() } });

    return { { "title", title },
             { "groups", groups },
             { "total", diff.count() },
             { "added", static_cast<int>(diff.added.size()) },
             { "removed", static_cast<int>(diff.removed.size()) },
             { "changed", static_cast<int>(diff.changed.size()) } };
}

QVariantMap SnapshotListModel::restorePreview(const QString& id)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return {};
    auto preview = describe(store.restorePreview(*snapshot), snapshot->label.isEmpty() ? snapshot->trigger : snapshot->label);
    preview.insert("id", id);
    preview.insert("includesWorlds", snapshot->includesWorlds);
    preview.insert("running", m_instance->isRunning());
    return preview;
}

void SnapshotListModel::restore(const QString& id)
{
    if (!m_instance || m_busy)
        return;
    if (m_instance->isRunning()) {
        m_shell->toast(tr("Close the game before restoring a snapshot."), "warning");
        return;
    }
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return;
    setBusy(true);
    // the current state is saved first, so a restore can always be undone
    Options before;
    before.trigger = tr("Before restoring a snapshot");
    before.automatic = true;
    before.includeWorlds = snapshot->includesWorlds;
    auto saved = SnapshotService::takeBlocking(m_instance, before, m_shell->window());
    if (!saved) {
        setBusy(false);
        m_shell->toast(tr("Didn't restore: couldn't save the current state first (%1).").arg(saved.error()), "error");
        return;
    }
    auto restored = store.restore(*snapshot);
    setBusy(false);
    // the component list (Minecraft and loader versions) lives in memory too
    [[maybe_unused]] auto reloaded = m_instance->getPackProfile()->reload(Net::Mode::Offline);
    SnapshotService::storeFor(m_instance).prune(SnapshotService::ringSize());
    refresh();
    if (!restored) {
        m_shell->toast(tr("Restore stopped: %1. The state from before is in the snapshot list.").arg(restored.error()), "error");
        return;
    }
    m_shell->toast(tr("Restored \"%1\"").arg(snapshot->label.isEmpty() ? snapshot->trigger : snapshot->label), "success");
}

QVariantMap SnapshotListModel::compare(const QString& olderId, const QString& newerId)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto older = store.get(olderId);
    auto newer = store.get(newerId);
    if (!older || !newer)
        return {};
    if (older->created > newer->created)
        std::swap(older, newer);
    auto name = [](const Snapshot& s) { return s.label.isEmpty() ? s.trigger : s.label; };
    auto result = describe(Store::diff(older->files, newer->files), tr("%1 → %2").arg(name(*older), name(*newer)));
    result.insert("olderId", older->id);
    result.insert("newerId", newer->id);
    return result;
}

QString SnapshotListModel::changelog(const QString& olderId, const QString& newerId)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto older = store.get(olderId);
    if (!older)
        return {};
    const QString olderName = older->label.isEmpty() ? QLocale().toString(older->created, QLocale::ShortFormat) : older->label;
    FileMap to;
    QString title;
    if (auto newer = store.get(newerId)) {
        to = newer->files;
        title = tr("%1 changelog: %2 to %3")
                    .arg(m_instance->name(), olderName,
                         newer->label.isEmpty() ? QLocale().toString(newer->created, QLocale::ShortFormat) : newer->label);
    } else {
        to = store.scanCurrent(older->includesWorlds);
        title = tr("%1 changelog since %2").arg(m_instance->name(), olderName);
    }
    return markdownChangelog(title, Store::diff(older->files, to), gameDir());
}

void SnapshotListModel::copyText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
    m_shell->toast(tr("Copied to the clipboard"), "success");
}

void SnapshotListModel::saveText(const QString& text, const QString& suggestedName)
{
    const QString path = QFileDialog::getSaveFileName(m_shell->window(), tr("Save changelog"),
                                                      QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(suggestedName),
                                                      tr("Markdown (*.md);;Text (*.txt)"));
    if (path.isEmpty())
        return;
    QSaveFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text.toUtf8());
        if (file.commit()) {
            m_shell->toast(tr("Saved %1").arg(QFileInfo(path).fileName()), "success");
            return;
        }
    }
    m_shell->toast(tr("Couldn't save %1").arg(path), "error");
}

void SnapshotListModel::exportFolder(const QString& id)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return;
    const QString parent = QFileDialog::getExistingDirectory(m_shell->window(), tr("Export snapshot to folder"),
                                                             QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    if (parent.isEmpty())
        return;
    const QString folder =
        QDir(parent).filePath(QString("%1 snapshot %2").arg(m_instance->name(), snapshot->created.toString("yyyy-MM-dd HH-mm-ss")));
    if (QFileInfo::exists(folder)) {
        m_shell->toast(tr("%1 already exists").arg(folder), "warning");
        return;
    }
    auto result = store.exportToFolder(*snapshot, folder);
    if (result)
        m_shell->toast(tr("Exported to %1").arg(QDir::toNativeSeparators(folder)), "success");
    else
        m_shell->toast(tr("Export failed: %1").arg(result.error()), "error");
}

void SnapshotListModel::exportZip(const QString& id)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto snapshot = store.get(id);
    if (!snapshot)
        return;
    const QString suggested = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                  .filePath(QString("%1 snapshot %2.zip").arg(m_instance->name(), snapshot->created.toString("yyyy-MM-dd HH-mm-ss")));
    const QString path = QFileDialog::getSaveFileName(m_shell->window(), tr("Export snapshot as zip"), suggested, tr("Zip archive (*.zip)"));
    if (path.isEmpty())
        return;
    auto result = store.exportToZip(*snapshot, path);
    if (result)
        m_shell->toast(tr("Exported %1").arg(QFileInfo(path).fileName()), "success");
    else
        m_shell->toast(tr("Export failed: %1").arg(result.error()), "error");
}

}  // namespace Wraith
