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
#include <QTimer>
#include <QVariantList>

#include "advanced/LauncherImport.h"
#include "net/NetJob.h"

namespace Wraith {

class Shell;

/** "Import from another launcher": instances found in other launchers' folders (read only). */
class ImportModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList instances READ instances NOTIFY changed)
    Q_PROPERTY(QStringList launchers READ launchers NOTIFY changed)

   public:
    explicit ImportModel(Shell* shell);
    QVariantList instances() const;
    QStringList launchers() const;

    Q_INVOKABLE void scan();
    /** Copy the chosen instances (indexes into `instances`) into Wraith, one after another. */
    Q_INVOKABLE void importSelected(const QVariantList& indexes);
    Q_INVOKABLE void addFolder();

   signals:
    void changed();

   private:
    Shell* m_shell;
    QList<Advanced::LauncherLocation> m_locations;
    QList<Advanced::ForeignInstance> m_found;
};

/** Every screenshot of every instance, newest first, for the gallery page. */
class GalleryModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QStringList instanceNames READ instanceNames NOTIFY changed)
    Q_PROPERTY(QString instanceFilter READ instanceFilter WRITE setInstanceFilter NOTIFY changed)

   public:
    enum Roles { PathRole = Qt::UserRole + 800, UrlRole, InstanceRole, InstanceIdRole, DateRole, NameRole };
    explicit GalleryModel(Shell* shell);

    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : static_cast<int>(m_visible.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return rowCount(); }
    QStringList instanceNames() const;
    QString instanceFilter() const { return m_filter; }
    void setInstanceFilter(const QString& name);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void openFile(int row);
    Q_INVOKABLE void showInFolder(int row);
    Q_INVOKABLE void copyImage(int row);
    /** Asks first; the file goes to the system trash when possible. */
    Q_INVOKABLE void remove(int row);

   signals:
    void changed();

   private:
    void rebuild();
    struct Shot {
        QString path;
        QString instance;
        QString instanceId;
        QDateTime date;
    };
    Shell* m_shell;
    QList<Shot> m_all;
    QList<int> m_visible;
    QString m_filter;
};

/**
 * Optional modpack update notifications (off by default): asks Modrinth/CurseForge whether a newer version of each
 * modpack instance exists, marks those instances in the library and says so once. Never updates anything itself.
 */
class UpdateNotifier : public QObject {
    Q_OBJECT
   public:
    explicit UpdateNotifier(Shell* shell);
    Q_INVOKABLE void checkNow(bool announce = true);

   private:
    Shell* m_shell;
    QTimer m_timer;
    NetJob::Ptr m_job;
};

}  // namespace Wraith
