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

#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QPointer>
#include <QTimer>
#include <QVariantList>

#include "advanced/CrashAnalyzer.h"

class MinecraftInstance;

namespace Wraith {

class Shell;

/** Watches running instances and, when one crashes, explains the crash in a toast that leads to the log viewer. */
namespace CrashWatcher {
void install(Shell* shell);
/** The newest crash report written in the last few minutes, else logs/latest.log. */
QString crashFileFor(MinecraftInstance* instance);
}  // namespace CrashWatcher

/**
 * The Logs tab's viewer: any log or crash report of the instance, with level filters, search, jumping between errors,
 * live updates of latest.log, and the crash analysis with suggestions (memory, Java, roll back).
 */
class LogViewerModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QVariantList files READ files NOTIFY filesChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY contentChanged)
    Q_PROPERTY(QString levelFilter READ levelFilter WRITE setLevelFilter NOTIFY contentChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY contentChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY contentChanged)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY contentChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY contentChanged)
    Q_PROPERTY(QVariantList findings READ findings NOTIFY contentChanged)
    Q_PROPERTY(bool hasSnapshots READ hasSnapshots NOTIFY contentChanged)

   public:
    enum Roles { TextRole = Qt::UserRole + 700, LevelRole, LineRole, MatchRole };

    LogViewerModel(Shell* shell, MinecraftInstance* instance, QObject* parent);

    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : static_cast<int>(m_visible.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariantList files() const { return m_files; }
    QString currentFile() const { return m_path; }
    QString levelFilter() const { return m_levelFilter; }
    void setLevelFilter(const QString& filter);
    QString search() const { return m_search; }
    void setSearch(const QString& text);
    int errorCount() const { return m_errors; }
    int warningCount() const { return m_warnings; }
    int totalCount() const { return static_cast<int>(m_lines.size()); }
    QVariantList findings() const { return m_findings; }
    bool hasSnapshots() const;

    /** Reload the file list; opens a pending crash file, otherwise keeps the current file (or latest.log). */
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void open(const QString& path);
    /** The next visible row after `fromRow` that is an error, or -1. */
    Q_INVOKABLE int nextError(int fromRow) const;
    Q_INVOKABLE void copyVisible();
    Q_INVOKABLE void openFolder();
    Q_INVOKABLE void showSnapshots();
    Q_INVOKABLE void showSettings();

   signals:
    void filesChanged();
    void contentChanged();

   private:
    void load();
    void rebuild();

    struct Line {
        QString text;
        Advanced::LogLevel level;
    };
    Shell* m_shell;
    QPointer<MinecraftInstance> m_instance;
    QVariantList m_files;
    QString m_path;
    QList<Line> m_lines;
    QList<int> m_visible;
    QString m_levelFilter = "all";
    QString m_search;
    int m_errors = 0;
    int m_warnings = 0;
    QVariantList m_findings;
    QFileSystemWatcher m_watcher;
    QTimer m_reload;
};

}  // namespace Wraith
