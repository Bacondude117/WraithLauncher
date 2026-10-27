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
#include <QQuickImageProvider>
#include <QStringList>
#include <QVariantList>

class MainWindow;
class MinecraftInstance;
class QMenu;

namespace Wraith {

class InstanceModel;
class Shell;

/** Serves instance icons (image://wraith/icon/<key>) and tinted UI glyphs (image://wraith/glyph/<name>/<color>). */
class ImageProvider : public QQuickImageProvider {
   public:
    ImageProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};

/**
 * The QML side's view of the launcher: instance data, navigation and every action, implemented with the existing
 * MainWindow actions and Application calls so behaviour stays identical to Prism's.
 */
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentPage READ currentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QObject* instances READ instances CONSTANT)
    Q_PROPERTY(int instanceCount READ instanceCount NOTIFY instancesUpdated)
    Q_PROPERTY(int revision READ revision NOTIFY instancesUpdated)
    Q_PROPERTY(QStringList groups READ groups NOTIFY instancesUpdated)
    Q_PROPERTY(QString totalPlayTime READ totalPlayTime NOTIFY instancesUpdated)
    Q_PROPERTY(int runningCount READ runningCount NOTIFY instancesUpdated)
    Q_PROPERTY(QString accountName READ accountName NOTIFY accountChanged)
    Q_PROPERTY(QString accountType READ accountType NOTIFY accountChanged)
    Q_PROPERTY(QString catImage READ catImage NOTIFY catChanged)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)

   public:
    Backend(Shell* shell, MainWindow* window);

    QString currentPage() const { return m_currentPage; }
    void setCurrentPage(const QString& page);
    QObject* instances() const;
    int instanceCount() const;
    int revision() const { return m_revision; }
    QStringList groups() const;
    QString totalPlayTime() const;
    int runningCount() const;
    QString accountName() const;
    QString accountType() const;
    QString catImage() const;
    QString displayName() const;

    MinecraftInstance* instanceById(const QString& id) const;

    // navigation
    Q_INVOKABLE void navigate(const QString& page);
    Q_INVOKABLE void openInstance(const QString& id, const QString& tab = QString());
    Q_INVOKABLE void browse(const QString& query = QString());

    // data
    Q_INVOKABLE QVariantList recentInstances(int count) const;
    Q_INVOKABLE QVariantMap instance(const QString& id) const;
    Q_INVOKABLE QString glyph(const QString& name, const QString& color) const;

    // instance actions
    Q_INVOKABLE void launch(const QString& id);
    Q_INVOKABLE void stop(const QString& id);
    Q_INVOKABLE void openFolder(const QString& id);
    Q_INVOKABLE void rename(const QString& id);
    Q_INVOKABLE void changeIcon(const QString& id);
    Q_INVOKABLE void changeGroup(const QString& id);
    Q_INVOKABLE void duplicate(const QString& id);
    Q_INVOKABLE void remove(const QString& id);
    Q_INVOKABLE void showInstanceMenu(const QString& id);
    Q_INVOKABLE void showLaunchMenu(const QString& id);
    Q_INVOKABLE void showGroupMenu(const QString& group);

    // global actions
    Q_INVOKABLE void newInstance(const QString& group = QString());
    Q_INVOKABLE void advancedNewInstance(const QString& pageId = QString());
    Q_INVOKABLE void importInstance();
    Q_INVOKABLE void showAccountMenu();
    Q_INVOKABLE void showMoreMenu();
    Q_INVOKABLE void toast(const QString& text, const QString& kind = QString());
    Q_INVOKABLE void openUrl(const QString& url);
    Q_INVOKABLE void dropUrls(const QVariantList& urls);

    MainWindow* window() const { return m_window; }
    QAction* action(const char* name) const;
    /** Selects the instance in the (hidden) classic view so MainWindow's actions apply to it. */
    MinecraftInstance* select(const QString& id) const;

   signals:
    void currentPageChanged();
    void instancesUpdated();
    void accountChanged();
    void catChanged();

   private:
    void popup(QMenu* menu);

    Shell* m_shell;
    MainWindow* m_window;
    InstanceModel* m_instances;
    QString m_currentPage = "home";
    int m_revision = 0;
};

}  // namespace Wraith
