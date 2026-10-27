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
#include <QPointer>

#include "snapshots/SnapshotStore.h"

class BaseInstance;
class MinecraftInstance;

namespace Wraith {

class Shell;

/**
 * Snapshot actions with UI: taking a snapshot while showing progress (the change waits for it), automatic snapshots
 * before changes (installed as the Snapshots::beforeChange handler), and settings.
 */
namespace SnapshotService {
/** Install the automatic-snapshot handler. Called once by the shell. */
void install(Shell* shell);
/** Take a snapshot, keeping the UI responsive and showing progress if it takes a moment. */
Result<Snapshots::Snapshot> takeBlocking(BaseInstance* instance, const Snapshots::Options& options, QWidget* parent);
Snapshots::Store storeFor(BaseInstance* instance);
int ringSize();
QString describeSize(qint64 bytes);
}  // namespace SnapshotService

/** The snapshots of one instance, for the instance page's Snapshots tab (QML). */
class SnapshotListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(bool autoEnabled READ autoEnabled WRITE setAutoEnabled NOTIFY settingsChanged)
    Q_PROPERTY(int ringSize READ ringSize WRITE setRingSize NOTIFY settingsChanged)
    Q_PROPERTY(bool includeWorldsByDefault READ includeWorldsByDefault WRITE setIncludeWorldsByDefault NOTIFY settingsChanged)
    Q_PROPERTY(QString storageText READ storageText NOTIFY listChanged)
    Q_PROPERTY(int count READ count NOTIFY listChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

   public:
    enum Roles {
        IdRole = Qt::UserRole + 600,
        TitleRole,
        TriggerRole,
        CreatedRole,
        CreatedFullRole,
        AutomaticRole,
        PinnedRole,
        WorldsRole,
        FilesRole,
        SizeRole
    };

    SnapshotListModel(Shell* shell, MinecraftInstance* instance, QObject* parent);

    int rowCount(const QModelIndex& parent = {}) const override { return parent.isValid() ? 0 : static_cast<int>(m_snapshots.size()); }
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool autoEnabled() const;
    void setAutoEnabled(bool on);
    int ringSize() const;
    void setRingSize(int size);
    bool includeWorldsByDefault() const;
    void setIncludeWorldsByDefault(bool on);
    QString storageText() const { return m_storageText; }
    int count() const { return rowCount(); }
    bool busy() const { return m_busy; }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void take(const QString& label, bool includeWorlds);
    Q_INVOKABLE void setPinned(const QString& id, bool pinned);
    Q_INVOKABLE void rename(const QString& id);
    Q_INVOKABLE void remove(const QString& id);
    /** { title, groups: [{ area, items: [{ path, name, kind }] }], counts, mods: [...] } restore would apply. */
    Q_INVOKABLE QVariantMap restorePreview(const QString& id);
    Q_INVOKABLE void restore(const QString& id);
    /** Differences going from `olderId` to `newerId`; same shape as restorePreview. */
    Q_INVOKABLE QVariantMap compare(const QString& olderId, const QString& newerId);
    /** Markdown changelog between two snapshots (or a snapshot and "now" when newerId is empty), copied or saved. */
    Q_INVOKABLE QString changelog(const QString& olderId, const QString& newerId);
    Q_INVOKABLE void copyText(const QString& text);
    Q_INVOKABLE void saveText(const QString& text, const QString& suggestedName);
    Q_INVOKABLE void exportFolder(const QString& id);
    Q_INVOKABLE void exportZip(const QString& id);

   signals:
    void listChanged();
    void settingsChanged();
    void busyChanged();

   private:
    QVariantMap describe(const Snapshots::Diff& diff, const QString& title) const;
    QString gameDir() const;
    void setBusy(bool busy);

    Shell* m_shell;
    QPointer<MinecraftInstance> m_instance;
    QList<Snapshots::Snapshot> m_snapshots;
    QString m_storageText;
    bool m_busy = false;
};

}  // namespace Wraith
