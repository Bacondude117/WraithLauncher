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

#include <QObject>
#include <QPointer>
#include <QVariantList>

class MinecraftInstance;

namespace Wraith {

class Shell;

/**
 * The instance page's "Creator" tab (modpack creator tools): dependency/conflict check, pre-export checklist for
 * .mrpack and CurseForge exports, changelogs and config diffs between snapshots, and test launches. Everything is
 * local; checks only suggest.
 */
class CreatorModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool checking READ checking NOTIFY checkingChanged)

   public:
    CreatorModel(Shell* shell, MinecraftInstance* instance, QObject* parent);

    bool checking() const { return m_checking; }

    /** Run the dependency/conflict check in the background; answers with checksFinished. */
    Q_INVOKABLE void runChecks();
    /** Build the pre-export checklist (runs the check too); answers with checklistReady. */
    Q_INVOKABLE void prepareExport(const QString& format);
    /** Open Prism's export dialog for "mrpack" or "curseforge". */
    Q_INVOKABLE void openExport(const QString& format);
    Q_INVOKABLE void snapshotForRelease(const QString& label);

    /** [{ id, title }] newest first, for the from/to pickers ("" = the instance now). */
    Q_INVOKABLE QVariantList snapshotChoices() const;
    Q_INVOKABLE QString changelog(const QString& fromId, const QString& toId);
    /** Changed text files outside the mods folder: [{ path, kind }]. */
    Q_INVOKABLE QVariantList configChanges(const QString& fromId, const QString& toId);
    /** [{ kind: "same"|"added"|"removed"|"hunk", text }] for one file. */
    Q_INVOKABLE QVariantList fileDiff(const QString& fromId, const QString& toId, const QString& path);

    /** Snapshot, then launch; when the game closes, offer to review or roll back the changes. */
    Q_INVOKABLE void testLaunch();
    Q_INVOKABLE int pinnedCount() const;

   signals:
    void checkingChanged();
    void checksFinished(const QVariantMap& result);
    void checklistReady(const QString& format, const QVariantList& items);

   private:
    void startChecks(std::function<void(const QVariantMap&)> done);
    QString loaderName() const;
    QString minecraftVersion() const;

    Shell* m_shell;
    QPointer<MinecraftInstance> m_instance;
    bool m_checking = false;
    QString m_testSnapshot;
};

}  // namespace Wraith
