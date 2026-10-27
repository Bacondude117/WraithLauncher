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

#include "WraithLogs.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QGuiApplication>

#include "Application.h"
#include "DesktopServices.h"
#include "GZip.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"

#include "WraithBackend.h"
#include "WraithShell.h"
#include "WraithSnapshots.h"

namespace Wraith {

using namespace Advanced;

// ---------------------------------------------------------------- crash watcher

namespace CrashWatcher {

QString crashFileFor(MinecraftInstance* instance)
{
    const QDir reports(QDir(instance->gameRoot()).filePath("crash-reports"));
    const auto newest = reports.entryInfoList({ "crash-*.txt" }, QDir::Files, QDir::Time);
    if (!newest.isEmpty() && newest.first().lastModified().secsTo(QDateTime::currentDateTime()) < 300)
        return newest.first().absoluteFilePath();
    return QDir(instance->gameRoot()).filePath("logs/latest.log");
}

namespace {
void watch(Shell* shell, BaseInstance* base)
{
    auto* instance = qobject_cast<MinecraftInstance*>(base);
    if (!instance || instance->property("wraithCrashWatched").toBool())
        return;
    instance->setProperty("wraithCrashWatched", true);
    QObject::connect(instance, &BaseInstance::runningStatusChanged, shell, [shell, instance](bool running) {
        if (running)
            return;
        // the launch task marks the crash right around the same moment; look a little later
        QTimer::singleShot(800, shell, [shell, instance] {
            if (!instance->hasCrashed())
                return;
            const QString file = crashFileFor(instance);
            QFile f(file);
            QString text;
            if (f.open(QIODevice::ReadOnly))
                text = QString::fromUtf8(f.readAll().right(4 * 1024 * 1024));
            const auto findings = analyzeCrash(text);
            instance->setProperty("wraithCrashFile", file);
            const QString id = instance->id();
            const QString headline = findings.isEmpty() ? QObject::tr("%1 crashed").arg(instance->name())
                                                        : QObject::tr("%1 crashed: %2").arg(instance->name(), findings.first().title);
            shell->toast(headline, "error", QObject::tr("What happened"), [shell, id] { shell->backend()->openInstance(id, "logs"); });
        });
    });
}
}  // namespace

void install(Shell* shell)
{
    auto* list = APPLICATION->instances();
    auto watchAll = [shell, list] {
        for (int i = 0; i < list->count(); i++)
            watch(shell, list->at(i));
    };
    watchAll();
    QObject::connect(list, &QAbstractItemModel::rowsInserted, shell, watchAll);
    QObject::connect(list, &QAbstractItemModel::modelReset, shell, watchAll);
}

}  // namespace CrashWatcher

// ---------------------------------------------------------------- log viewer

LogViewerModel::LogViewerModel(Shell* shell, MinecraftInstance* instance, QObject* parent)
    : QAbstractListModel(parent), m_shell(shell), m_instance(instance)
{
    // latest.log grows while the game runs: reload shortly after it changes
    m_reload.setSingleShot(true);
    m_reload.setInterval(800);
    connect(&m_reload, &QTimer::timeout, this, &LogViewerModel::load);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_reload.start(); });
}

QHash<int, QByteArray> LogViewerModel::roleNames() const
{
    return { { TextRole, "text" }, { LevelRole, "level" }, { LineRole, "lineNumber" }, { MatchRole, "matches" } };
}

QVariant LogViewerModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_visible.size())
        return {};
    const int lineIndex = m_visible.at(index.row());
    const auto& line = m_lines.at(lineIndex);
    switch (role) {
        case TextRole:
            return line.text;
        case LevelRole:
            return levelName(line.level);
        case LineRole:
            return lineIndex + 1;
        case MatchRole:
            return !m_search.isEmpty() && line.text.contains(m_search, Qt::CaseInsensitive);
        default:
            return {};
    }
}

bool LogViewerModel::hasSnapshots() const
{
    return m_instance && !SnapshotService::storeFor(m_instance).list().isEmpty();
}

void LogViewerModel::refresh()
{
    if (!m_instance)
        return;
    const QDir game(m_instance->gameRoot());
    m_files.clear();
    auto addFiles = [this](const QDir& dir, const QStringList& patterns, const QString& kind) {
        for (const auto& f : dir.entryInfoList(patterns, QDir::Files, QDir::Time))
            m_files.append(QVariantMap{ { "path", f.absoluteFilePath() },
                                        { "name", (kind == "crash" ? tr("Crash: ") : QString()) + f.fileName() },
                                        { "kind", kind } });
    };
    addFiles(QDir(game.filePath("crash-reports")), { "crash-*.txt" }, "crash");
    addFiles(QDir(game.filePath("logs")), { "latest.log", "debug.log" }, "log");
    addFiles(QDir(game.filePath("logs")), { "*.log.gz" }, "log");
    emit filesChanged();

    const QString pending = m_instance->property("wraithCrashFile").toString();
    if (!pending.isEmpty()) {
        m_instance->setProperty("wraithCrashFile", QVariant());
        open(pending);
    } else if (m_path.isEmpty() || !QFile::exists(m_path)) {
        open(game.filePath("logs/latest.log"));
    } else {
        load();
    }
}

void LogViewerModel::open(const QString& path)
{
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    m_path = path;
    if (path.endsWith("latest.log") && QFile::exists(path))
        m_watcher.addPath(path);
    load();
}

void LogViewerModel::load()
{
    QByteArray data;
    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        data = file.readAll();
        if (m_path.endsWith(".gz")) {
            QByteArray unzipped;
            if (GZip::unzip(data, unzipped))
                data = unzipped;
        }
    }
    // very large logs: keep the last 200k lines
    QStringList raw = QString::fromUtf8(data).split('\n');
    if (raw.size() > 200000)
        raw = raw.mid(raw.size() - 200000);
    m_lines.clear();
    m_lines.reserve(raw.size());
    LogLevel previous = LogLevel::Unknown;
    m_errors = 0;
    m_warnings = 0;
    for (auto& text : raw) {
        if (text.endsWith('\r'))
            text.chop(1);
        const LogLevel level = lineLevel(text, previous);
        previous = level == LogLevel::Unknown ? previous : level;
        // a plain line after an error block ends it
        if (level == LogLevel::Unknown && !text.startsWith('\t') && !text.trimmed().startsWith("at "))
            previous = LogLevel::Unknown;
        m_lines.append({ text, level });
        if (level == LogLevel::Error || level == LogLevel::Fatal)
            m_errors++;
        else if (level == LogLevel::Warn)
            m_warnings++;
    }
    m_findings.clear();
    const bool crashReport = m_path.contains("crash-reports");
    if (crashReport || m_errors > 0) {
        for (const auto& f : analyzeCrash(QString::fromUtf8(data))) {
            // in an ordinary log only the findings that point at real problems are worth a card
            if (!crashReport && f.id == "generic")
                continue;
            m_findings.append(QVariantMap{ { "title", f.title },
                                           { "explanation", f.explanation },
                                           { "suggestion", f.suggestion },
                                           { "evidence", f.evidence },
                                           { "mods", f.mods },
                                           { "memory", f.memory },
                                           { "java", f.java },
                                           { "rollback", f.rollback } });
        }
    }
    rebuild();
}

void LogViewerModel::rebuild()
{
    beginResetModel();
    m_visible.clear();
    for (int i = 0; i < m_lines.size(); i++) {
        const auto level = m_lines[i].level;
        if (m_levelFilter == "error" && level != LogLevel::Error && level != LogLevel::Fatal)
            continue;
        if (m_levelFilter == "warn" && level != LogLevel::Warn && level != LogLevel::Error && level != LogLevel::Fatal)
            continue;
        if (!m_search.isEmpty() && !m_lines[i].text.contains(m_search, Qt::CaseInsensitive))
            continue;
        m_visible.append(i);
    }
    endResetModel();
    emit contentChanged();
}

void LogViewerModel::setLevelFilter(const QString& filter)
{
    if (m_levelFilter == filter)
        return;
    m_levelFilter = filter;
    rebuild();
}

void LogViewerModel::setSearch(const QString& text)
{
    if (m_search == text)
        return;
    m_search = text;
    rebuild();
}

int LogViewerModel::nextError(int fromRow) const
{
    for (int row = std::max(0, fromRow + 1); row < m_visible.size(); row++) {
        const auto level = m_lines[m_visible[row]].level;
        if (level == LogLevel::Error || level == LogLevel::Fatal)
            return row;
    }
    return -1;
}

void LogViewerModel::copyVisible()
{
    QStringList text;
    for (int i : m_visible)
        text.append(m_lines[i].text);
    QGuiApplication::clipboard()->setText(text.join('\n'));
    m_shell->toast(tr("Copied %1 line(s)").arg(m_visible.size()), "success");
}

void LogViewerModel::openFolder()
{
    DesktopServices::openPath(QFileInfo(m_path).absolutePath(), true);
}

void LogViewerModel::showSnapshots()
{
    m_shell->backend()->openInstance(m_instance->id(), "snapshots");
}

void LogViewerModel::showSettings()
{
    m_shell->backend()->openInstance(m_instance->id(), "settings");
}

}  // namespace Wraith
