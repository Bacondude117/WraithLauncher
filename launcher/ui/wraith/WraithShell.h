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

#include <QHash>
#include <QPointer>
#include <QWidget>
#include <functional>

class MainWindow;
class MinecraftInstance;
class QQmlEngine;
class QQuickWidget;
class QStackedWidget;
class QFrame;

namespace Wraith {

class Backend;
class InstancePage;
class SettingsPage;

/** A page of the shell that wants to know when it is shown or left (to apply settings, refresh, ...). */
class ShellPage {
   public:
    virtual ~ShellPage() = default;
    virtual void pageShown() {}
    /** Return false to stay on the page (e.g. invalid settings). */
    virtual bool pageLeaving() { return true; }
};

/**
 * The Wraith interface: a sidebar and a stack of pages that replaces Prism's toolbars, grid and dialogs.
 * MainWindow keeps running underneath as the controller for every action.
 */
class Shell : public QWidget {
    Q_OBJECT
   public:
    explicit Shell(MainWindow* window, QWidget* parent = nullptr);
    ~Shell() override;

    Backend* backend() const { return m_backend; }
    QQmlEngine* engine() const { return m_engine; }
    MainWindow* window() const { return m_window; }

    /** Hosts a QML file from qrc:/wraith/qml/ in a widget that shares the shell's engine. */
    QQuickWidget* createQuick(const QString& file, QWidget* parent);

    void showPage(const QString& name);
    void showInstance(MinecraftInstance* instance, const QString& tab = QString());
    void browse(const QString& query);
    void showNewInstance(const QString& group = QString());
    void showSettings(const QString& category = QString());
    void toast(const QString& text,
               const QString& kind = QString(),
               const QString& actionText = QString(),
               std::function<void()> action = {});

    /** Applies pending changes of the visible page (instance settings, global settings). */
    bool leaveCurrentPage();

   protected:
    void resizeEvent(QResizeEvent* event) override;

   private:
    QWidget* page(const QString& name);
    void switchTo(QWidget* widget, const QString& name);
    void layoutToasts();
    void runUiScript(const QString& path);

    MainWindow* m_window;
    QQmlEngine* m_engine;
    Backend* m_backend;
    QQuickWidget* m_sidebar;
    QStackedWidget* m_stack;
    QHash<QString, QWidget*> m_pages;
    QHash<QString, QPointer<InstancePage>> m_instancePages;
    QList<QPointer<QFrame>> m_toasts;
};

}  // namespace Wraith
