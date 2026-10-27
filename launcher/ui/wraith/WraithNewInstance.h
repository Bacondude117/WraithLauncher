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
#pragma once

#include <QObject>
#include <QVariantList>

#include "BaseVersion.h"
#include "tasks/Task.h"

class QQuickWidget;

namespace Wraith {

class Shell;

/**
 * The simple "New instance" sheet: name, Minecraft version, mod loader (its recommended version is picked
 * automatically) and group. Everything else (import, modpack pages, picking loader versions) is in Prism's full
 * dialog, one click away from the sheet.
 */
class NewInstanceSheet : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList versions READ versions NOTIFY versionsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY versionsChanged)
    Q_PROPERTY(QString loaderVersion READ loaderVersion NOTIFY loaderChanged)
    Q_PROPERTY(bool loaderAvailable READ loaderAvailable NOTIFY loaderChanged)
    Q_PROPERTY(QString group READ group NOTIFY groupChanged)

   public:
    explicit NewInstanceSheet(Shell* shell);

    void open(const QString& group);

    QVariantList versions() const { return m_versions; }
    bool loading() const { return m_loading; }
    QString loaderVersion() const { return m_loaderVersionText; }
    bool loaderAvailable() const { return m_loaderAvailable; }
    QString group() const { return m_group; }

    /** Pick the recommended loader version for this Minecraft version (loads the loader's version list if needed). */
    Q_INVOKABLE void selectLoader(const QString& loader, const QString& minecraft);
    Q_INVOKABLE void create(const QString& name, const QString& minecraft, const QString& loader, const QString& group);
    Q_INVOKABLE void advanced(const QString& group);
    Q_INVOKABLE void importFile();
    Q_INVOKABLE void close();

   protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

   signals:
    void versionsChanged();
    void loaderChanged();
    void groupChanged();

   private:
    void loadVersions();
    void resolveLoader();

    Shell* m_shell;
    QQuickWidget* m_view = nullptr;
    QVariantList m_versions;
    bool m_loading = false;
    Task::Ptr m_loadTask;
    Task::Ptr m_loaderTask;
    QString m_loader;
    QString m_minecraft;
    BaseVersion::Ptr m_loaderVersion;
    QString m_loaderVersionText;
    bool m_loaderAvailable = true;
    QString m_group;
};

}  // namespace Wraith
