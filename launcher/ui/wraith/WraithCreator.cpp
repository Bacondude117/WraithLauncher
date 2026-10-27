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

#include "WraithCreator.h"

#include <QAction>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFutureWatcher>
#include <QtConcurrentRun>

#include "creator/ModCheck.h"
#include "creator/TextDiff.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "snapshots/SnapshotDiff.h"
#include "snapshots/SnapshotStore.h"
#include "ui/MainWindow.h"

#include "WraithBackend.h"
#include "WraithShell.h"
#include "WraithSnapshots.h"

namespace Wraith {

using namespace Creator;

namespace {
QString severityName(Issue::Severity s)
{
    switch (s) {
        case Issue::Severity::Error:
            return "error";
        case Issue::Severity::Warning:
            return "warning";
        default:
            return "info";
    }
}

QVariantMap checkItem(const QString& state, const QString& title, const QString& detail)
{
    return { { "state", state }, { "title", title }, { "detail", detail } };
}
}  // namespace

CreatorModel::CreatorModel(Shell* shell, MinecraftInstance* instance, QObject* parent) : QObject(parent), m_shell(shell), m_instance(instance) {}

QString CreatorModel::loaderName() const
{
    auto* profile = m_instance->getPackProfile();
    if (!profile->getComponentVersion("net.neoforged").isEmpty())
        return "neoforge";
    if (!profile->getComponentVersion("net.minecraftforge").isEmpty())
        return "forge";
    if (!profile->getComponentVersion("org.quiltmc.quilt-loader").isEmpty())
        return "quilt";
    if (!profile->getComponentVersion("net.fabricmc.fabric-loader").isEmpty())
        return "fabric";
    return {};
}

QString CreatorModel::minecraftVersion() const
{
    return m_instance->getPackProfile()->getComponentVersion("net.minecraft");
}

void CreatorModel::startChecks(std::function<void(const QVariantMap&)> done)
{
    if (!m_instance || m_checking)
        return;
    m_checking = true;
    emit checkingChanged();
    const QString modsDir = m_instance->modsRoot();
    const QString loader = loaderName();
    const QString minecraft = minecraftVersion();
    struct Output {
        QList<Issue> issues;
        int files = 0;
        int unknown = 0;
    };
    auto future = QtConcurrent::run([modsDir, loader, minecraft] {
        Output out;
        QList<ModInfo> mods;
        const auto files = QDir(modsDir).entryInfoList({ "*.jar", "*.jar.disabled", "*.zip", "*.zip.disabled" }, QDir::Files);
        for (const auto& f : files) {
            out.files++;
            const auto read = readModFile(f.absoluteFilePath());
            if (read.isEmpty())
                out.unknown++;
            mods.append(read);
        }
        out.issues = checkMods(mods, loader, minecraft);
        return out;
    });
    auto* watcher = new QFutureWatcher<Output>(this);
    connect(watcher, &QFutureWatcher<Output>::finished, this, [this, watcher, done] {
        const Output out = watcher->result();
        watcher->deleteLater();
        QVariantList issues;
        int errors = 0;
        for (const auto& i : out.issues) {
            errors += i.severity == Issue::Severity::Error;
            issues.append(QVariantMap{ { "severity", severityName(i.severity) },
                                       { "title", i.title },
                                       { "detail", i.detail },
                                       { "suggestion", i.suggestion },
                                       { "files", i.files } });
        }
        m_checking = false;
        emit checkingChanged();
        done({ { "issues", issues }, { "errors", errors }, { "files", out.files }, { "unknown", out.unknown }, { "loader", loaderName() } });
    });
    watcher->setFuture(future);
}

void CreatorModel::runChecks()
{
    startChecks([this](const QVariantMap& result) { emit checksFinished(result); });
}

void CreatorModel::prepareExport(const QString& format)
{
    startChecks([this, format](const QVariantMap& result) {
        QVariantList items;
        const int errors = result.value("errors").toInt();
        const int issues = static_cast<int>(result.value("issues").toList().size());
        items.append(errors > 0 ? checkItem("error", tr("%1 mod problem(s) found").arg(errors), tr("See Health check above. Players would hit them too."))
                     : issues > 0 ? checkItem("warn", tr("%1 warning(s) from the mod check").arg(issues), tr("See Health check above."))
                                  : checkItem("ok", tr("Dependencies and conflicts look fine"), QString()));

        const QDir game(m_instance->gameRoot());
        const QDir mods(m_instance->modsRoot());
        // mods the platforms know (downloaded through the launcher) are referenced; others are copied into the pack
        const int total = static_cast<int>(mods.entryList({ "*.jar" }, QDir::Files).size());
        const int indexed = static_cast<int>(QDir(mods.filePath(".index")).entryList({ "*.toml" }, QDir::Files).size());
        if (total > indexed)
            items.append(checkItem("warn", tr("%1 of %2 mods weren't downloaded through the launcher").arg(total - indexed).arg(total),
                                   format == "curseforge"
                                       ? tr("CurseForge exports can only reference mods hosted on CurseForge; others are included as files. "
                                            "Check their licences allow redistribution.")
                                       : tr("Modrinth packs reference mods by hash where possible; the rest are included as files. "
                                            "Check their licences allow redistribution.")));
        else
            items.append(checkItem("ok", tr("Every mod has download information"), QString()));

        const int disabled = static_cast<int>(mods.entryList({ "*.disabled" }, QDir::Files).size());
        if (disabled > 0)
            items.append(checkItem("info", tr("%1 disabled mod(s)").arg(disabled), tr("Untick them in the export dialog if they shouldn't ship.")));

        QStringList personal;
        for (const char* name : { "options.txt", "servers.dat", "saves", "screenshots", "logs", "crash-reports", "journeymap", "XaeroWaypoints" })
            if (game.exists(name))
                personal.append(name);
        if (!personal.isEmpty())
            items.append(checkItem("warn", tr("Personal files in the instance"),
                                   tr("%1. Untick them in the export dialog unless you mean to ship them.").arg(personal.join(", "))));

        QStringList large;
        QDirIterator it(game.absolutePath(), QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo f(it.next());
            if (f.size() > 50LL * 1024 * 1024 && !f.absoluteFilePath().contains("/saves/"))
                large.append(game.relativeFilePath(f.absoluteFilePath()));
        }
        if (!large.isEmpty())
            items.append(checkItem("warn", tr("Large files"), tr("Over 50 MB: %1").arg(large.mid(0, 5).join(", "))));

        const bool hasSnapshot = !SnapshotService::storeFor(m_instance).list().isEmpty();
        items.append(hasSnapshot ? checkItem("ok", tr("Snapshot available for changelogs"), QString())
                                 : checkItem("info", tr("No snapshot yet"),
                                             tr("Take a snapshot named after this release, so the next release can get a changelog.")));
        emit checklistReady(format, items);
    });
}

void CreatorModel::openExport(const QString& format)
{
    if (!m_instance)
        return;
    m_shell->backend()->select(m_instance->id());
    const char* name = format == "curseforge" ? "actionExportInstanceFlamePack" : "actionExportInstanceMrPack";
    if (auto* action = m_shell->window()->findChild<QAction*>(name))
        action->trigger();
}

void CreatorModel::snapshotForRelease(const QString& label)
{
    Snapshots::Options options;
    options.label = label.trimmed().isEmpty() ? tr("Release") : label.trimmed();
    options.trigger = tr("Manual");
    auto result = SnapshotService::takeBlocking(m_instance, options, m_shell->window());
    if (result)
        m_shell->toast(tr("Snapshot \"%1\" saved").arg(options.label), "success");
    else
        m_shell->toast(tr("Couldn't take a snapshot: %1").arg(result.error()), "error");
}

QVariantList CreatorModel::snapshotChoices() const
{
    QVariantList choices;
    for (const auto& s : SnapshotService::storeFor(m_instance).list())
        choices.append(QVariantMap{ { "id", s.id },
                                    { "title", (s.label.isEmpty() ? s.trigger : s.label) + "  ·  " +
                                                   QLocale().toString(s.created, QLocale::ShortFormat) } });
    return choices;
}

QString CreatorModel::changelog(const QString& fromId, const QString& toId)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto from = store.get(fromId);
    if (!from)
        return tr("Pick a snapshot to start from.");
    auto name = [](const Snapshots::Snapshot& s) { return s.label.isEmpty() ? QLocale().toString(s.created, QLocale::ShortFormat) : s.label; };
    Snapshots::FileMap to;
    QString title;
    if (auto other = store.get(toId)) {
        to = other->files;
        title = tr("%1 %2 → %3").arg(m_instance->name(), name(*from), name(*other));
    } else {
        to = store.scanCurrent(from->includesWorlds);
        title = tr("%1: changes since %2").arg(m_instance->name(), name(*from));
    }
    const QString game = QDir::fromNativeSeparators(QDir(m_instance->instanceRoot()).relativeFilePath(m_instance->gameRoot()));
    return Snapshots::markdownChangelog(title, Snapshots::Store::diff(from->files, to), game);
}

QVariantList CreatorModel::configChanges(const QString& fromId, const QString& toId)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto from = store.get(fromId);
    if (!from)
        return {};
    auto other = store.get(toId);
    const Snapshots::FileMap to = other ? other->files : store.scanCurrent(from->includesWorlds);
    const QString game = QDir::fromNativeSeparators(QDir(m_instance->instanceRoot()).relativeFilePath(m_instance->gameRoot()));
    QVariantList out;
    for (const auto& c : Snapshots::groupedChanges(Snapshots::Store::diff(from->files, to), game)) {
        if (c.area == Snapshots::Area::Mods || c.area == Snapshots::Area::Worlds)
            continue;
        out.append(QVariantMap{ { "path", c.path }, { "kind", c.kind }, { "area", Snapshots::areaName(c.area) } });
    }
    return out;
}

QVariantList CreatorModel::fileDiff(const QString& fromId, const QString& toId, const QString& path)
{
    auto store = SnapshotService::storeFor(m_instance);
    auto from = store.get(fromId);
    if (!from)
        return {};
    auto contentAt = [&](const std::optional<Snapshots::Snapshot>& s) -> QByteArray {
        if (s)
            return s->files.contains(path) ? store.contents(s->files.value(path).hash) : QByteArray();
        QFile file(QDir(m_instance->instanceRoot()).filePath(path));
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
    };
    const QByteArray before = contentAt(from);
    const QByteArray after = contentAt(store.get(toId));
    if (!looksLikeText(before) || !looksLikeText(after))
        return { QVariantMap{ { "kind", "hunk" }, { "text", tr("Binary file: contents differ") } } };
    QVariantList out;
    for (const auto& line : unifiedDiff(QString::fromUtf8(before), QString::fromUtf8(after), path).split('\n')) {
        if (line.startsWith("---") || line.startsWith("+++"))
            continue;
        const QString kind = line.startsWith("@@") ? "hunk" : line.startsWith('+') ? "added" : line.startsWith('-') ? "removed" : "same";
        out.append(QVariantMap{ { "kind", kind }, { "text", line } });
    }
    return out;
}

void CreatorModel::testLaunch()
{
    if (!m_instance || m_instance->isRunning())
        return;
    Snapshots::Options options;
    options.trigger = tr("Before a test launch");
    options.automatic = true;
    auto result = SnapshotService::takeBlocking(m_instance, options, m_shell->window());
    if (!result) {
        m_shell->toast(tr("Couldn't take a snapshot before the test: %1").arg(result.error()), "error");
        return;
    }
    m_testSnapshot = result->id;
    // when this test session ends, say what changed and offer a way back; nothing is rolled back automatically
    auto connection = std::make_shared<QMetaObject::Connection>();
    *connection = connect(m_instance, &BaseInstance::runningStatusChanged, this, [this, connection](bool running) {
        if (running)
            return;
        disconnect(*connection);
        auto store = SnapshotService::storeFor(m_instance);
        auto before = store.get(m_testSnapshot);
        if (!before)
            return;
        const auto diff = store.restorePreview(*before);
        const QString id = m_instance->id();
        if (diff.isEmpty())
            m_shell->toast(tr("Test run finished; nothing in the instance changed."), "success");
        else
            m_shell->toast(tr("Test run finished; %1 file(s) changed. You can review or roll back in Snapshots.").arg(diff.count()), "info",
                           tr("Review"), [this, id] { m_shell->backend()->openInstance(id, "snapshots"); });
    });
    m_shell->backend()->launch(m_instance->id());
}

int CreatorModel::pinnedCount() const
{
    int pinned = 0;
    if (auto model = m_instance->loaderModList())
        for (auto* resource : model->allResources())
            pinned += resource->lockUpdate() ? 1 : 0;
    return pinned;
}

}  // namespace Wraith
