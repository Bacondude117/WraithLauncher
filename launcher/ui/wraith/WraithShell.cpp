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
#include "WraithShell.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWidget>
#include <QScreen>
#include <QStackedWidget>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>

#include "Application.h"
#include "InstanceList.h"
#include "minecraft/MinecraftInstance.h"
#include "ui/MainWindow.h"

#include "WraithBackend.h"
#include "WraithBrowse.h"
#include "WraithInstancePage.h"
#include "WraithNewInstance.h"
#include "WraithSettingsPage.h"
#include "WraithStyle.h"

namespace Wraith {

namespace {
constexpr int s_sidebarWidth = 228;
constexpr int s_toastWidth = 360;
}  // namespace

Shell::Shell(MainWindow* window, QWidget* parent) : QWidget(parent), m_window(window)
{
    setObjectName("wraithShell");
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet(shellStyleSheet() + QString("#wraithShell { background-color: %1; }").arg(Color::bg));

    static bool s_styleSet = false;
    if (!s_styleSet) {
        QQuickStyle::setStyle("Basic");
        s_styleSet = true;
    }
    m_engine = new QQmlEngine(this);
    m_engine->addImageProvider("wraith", new ImageProvider());
    m_backend = new Backend(this, window);
    m_engine->rootContext()->setContextProperty("backend", m_backend);
    m_engine->rootContext()->setContextProperty("browseModel", new BrowseModel(this));
    m_newInstance = new NewInstanceSheet(this);
    m_engine->rootContext()->setContextProperty("newInstance", m_newInstance);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_sidebar = createQuick("Sidebar.qml", this);
    m_sidebar->setFixedWidth(s_sidebarWidth);
    m_sidebar->setClearColor(QColor(Color::sidebar));
    layout->addWidget(m_sidebar);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);

    showPage("home");

    const QString script = qEnvironmentVariable("WRAITH_UI_SCRIPT");
    if (!script.isEmpty())
        QTimer::singleShot(1500, this, [this, script] { runUiScript(script); });
}

Shell::~Shell() = default;

QQuickWidget* Shell::createQuick(const QString& file, QWidget* parent)
{
    auto* view = new QQuickWidget(m_engine, parent);
    view->setResizeMode(QQuickWidget::SizeRootObjectToView);
    view->setClearColor(QColor(Color::bg));
    view->setSource(QUrl("qrc:/wraith/qml/" + file));
    for (const auto& error : view->errors())
        qWarning() << "Wraith QML:" << error.toString();
    return view;
}

QWidget* Shell::page(const QString& name)
{
    if (auto* existing = m_pages.value(name))
        return existing;
    QWidget* created = nullptr;
    if (name == "home")
        created = createQuick("HomePage.qml", m_stack);
    else if (name == "library")
        created = createQuick("LibraryPage.qml", m_stack);
    else if (name == "browse")
        created = createQuick("BrowsePage.qml", m_stack);
    else if (name == "servers")
        created = createQuick("ServersPage.qml", m_stack);
    else if (name == "settings")
        created = new SettingsPage(this, m_stack);
    if (!created)
        return nullptr;
    m_stack->addWidget(created);
    m_pages.insert(name, created);
    return created;
}

bool Shell::leaveCurrentPage()
{
    if (auto* current = dynamic_cast<ShellPage*>(m_stack->currentWidget()))
        return current->pageLeaving();
    return true;
}

void Shell::switchTo(QWidget* widget, const QString& name)
{
    if (!widget)
        return;
    if (m_stack->currentWidget() != widget) {
        if (!leaveCurrentPage())
            return;
        m_stack->setCurrentWidget(widget);
    }
    m_backend->setCurrentPage(name);
    if (auto* shellPage = dynamic_cast<ShellPage*>(widget))
        shellPage->pageShown();
    if (auto* quick = qobject_cast<QQuickWidget*>(widget)) {
        if (auto* root = quick->rootObject())
            QMetaObject::invokeMethod(root, "pageShown");
        quick->setFocus();
    }
}

void Shell::showPage(const QString& name)
{
    if (name == "instance")
        return;  // needs an instance; see showInstance()
    switchTo(page(name), name);
}

void Shell::showInstance(MinecraftInstance* instance, const QString& tab)
{
    if (!instance)
        return;
    auto page = m_instancePages.value(instance->id());
    if (!page) {
        page = new InstancePage(this, instance, m_stack);
        m_stack->addWidget(page);
        m_instancePages.insert(instance->id(), page);
        connect(instance, &BaseInstance::statusChanged, page, [this, id = instance->id()](BaseInstance::Status, BaseInstance::Status status) {
            if (status != BaseInstance::Status::Gone)
                return;
            if (auto gone = m_instancePages.take(id)) {
                if (m_stack->currentWidget() == gone)
                    showPage("library");
                gone->deleteLater();
            }
        });
    }
    switchTo(page, "instance");
    page->selectTab(tab);
}

void Shell::browse(const QString& query)
{
    auto* browsePage = qobject_cast<QQuickWidget*>(page("browse"));
    switchTo(browsePage, "browse");
    if (browsePage && browsePage->rootObject() && !query.isNull())
        QMetaObject::invokeMethod(browsePage->rootObject(), "search", Q_ARG(QVariant, query));
}

void Shell::showNewInstance(const QString& group)
{
    m_newInstance->open(group);
}

void Shell::showSettings(const QString& category)
{
    auto* settings = static_cast<SettingsPage*>(page("settings"));
    switchTo(settings, "settings");
    if (!category.isEmpty())
        settings->selectCategory(category);
}

void Shell::toast(const QString& text, const QString& kind, const QString& actionText, std::function<void()> action)
{
    auto* toast = new QFrame(this);
    toast->setObjectName("wraithToast");
    toast->setFixedWidth(s_toastWidth);
    auto* row = new QHBoxLayout(toast);
    row->setContentsMargins(14, 10, 8, 10);
    row->setSpacing(10);

    const char* dotColor = kind == "error" ? Color::danger : kind == "success" ? Color::success : kind == "warning" ? Color::warning : Color::accent;
    auto* dot = new QLabel(toast);
    dot->setFixedSize(8, 8);
    dot->setStyleSheet(QString("background-color: %1; border-radius: 4px;").arg(dotColor));
    row->addWidget(dot, 0, Qt::AlignVCenter);

    auto* label = new QLabel(text, toast);
    label->setWordWrap(true);
    row->addWidget(label, 1);

    if (!actionText.isEmpty() && action) {
        auto* button = new QPushButton(actionText, toast);
        button->setCursor(Qt::PointingHandCursor);
        row->addWidget(button);
        connect(button, &QPushButton::clicked, toast, [toast, action] {
            action();
            toast->close();
        });
    }
    auto* close = new QToolButton(toast);
    close->setText(QString::fromUtf8("✕"));
    close->setCursor(Qt::PointingHandCursor);
    row->addWidget(close);
    connect(close, &QToolButton::clicked, toast, &QWidget::close);

    toast->setAttribute(Qt::WA_DeleteOnClose);
    connect(toast, &QObject::destroyed, this, [this] { QTimer::singleShot(0, this, &Shell::layoutToasts); });

    // only the newest three stay
    m_toasts.removeAll(nullptr);
    while (m_toasts.size() >= 3) {
        if (auto oldest = m_toasts.takeFirst())
            oldest->close();
    }
    m_toasts.append(toast);
    toast->adjustSize();
    toast->show();
    toast->raise();
    layoutToasts();

    // slide in from the right
    const QPoint target = toast->pos();
    auto* slide = new QPropertyAnimation(toast, "pos", toast);
    slide->setDuration(250);
    slide->setEasingCurve(QEasingCurve::OutCubic);
    slide->setStartValue(target + QPoint(40, 0));
    slide->setEndValue(target);
    slide->start(QAbstractAnimation::DeleteWhenStopped);

    const int lifetime = kind == "error" ? 8000 : action ? 7000 : 4000;
    QTimer::singleShot(lifetime, toast, [toast] {
        auto* effect = new QGraphicsOpacityEffect(toast);
        toast->setGraphicsEffect(effect);
        auto* fade = new QPropertyAnimation(effect, "opacity", toast);
        fade->setDuration(220);
        fade->setStartValue(1.0);
        fade->setEndValue(0.0);
        QObject::connect(fade, &QPropertyAnimation::finished, toast, &QWidget::close);
        fade->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

void Shell::layoutToasts()
{
    m_toasts.removeAll(nullptr);
    int bottom = height() - 24;
    for (int i = m_toasts.size() - 1; i >= 0; i--) {
        auto toast = m_toasts.at(i);
        toast->adjustSize();
        bottom -= toast->height();
        toast->move(width() - s_toastWidth - 24, bottom);
        toast->raise();
        bottom -= 10;
    }
}

void Shell::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    layoutToasts();
}

// QWidget::grab()/render() trip an overflow assert in Qt's debug build once the window hosts QQuickWidgets, so
// screenshots start from the screen contents of the window and then paint every visible Qt Quick view from its
// own framebuffer on top (which also keeps third-party overlays drawn into the swap chain out of the picture).
static void paintQuickViews(QPainter& painter, QWidget* widget, QWidget* window)
{
    if (!widget->isVisible())
        return;
    if (auto* quick = qobject_cast<QQuickWidget*>(widget)) {
        painter.drawImage(QRect(quick->mapTo(window, QPoint()), quick->size()), quick->grabFramebuffer());
        return;
    }
    for (auto* child : widget->children()) {
        auto* childWidget = qobject_cast<QWidget*>(child);
        if (childWidget && !childWidget->isWindow() && !childWidget->objectName().startsWith("wraithToast"))
            paintQuickViews(painter, childWidget, window);
    }
}

static QImage captureWindow(QWidget* window)
{
    window->raise();
    window->activateWindow();
    const QPoint topLeft = window->mapToGlobal(QPoint());
    QPixmap shot = window->screen()->grabWindow(0, topLeft.x(), topLeft.y(), window->width(), window->height());
    {
        QPainter painter(&shot);
        paintQuickViews(painter, window, window);
        // toasts float above the Qt Quick views: take them from the screen again
        for (auto* toast : window->findChildren<QFrame*>("wraithToast")) {
            if (!toast->isVisible())
                continue;
            const QPoint pos = toast->mapTo(window, QPoint());
            painter.drawPixmap(QRect(pos, toast->size()),
                               window->screen()->grabWindow(0, topLeft.x() + pos.x(), topLeft.y() + pos.y(), toast->width(), toast->height()));
        }
    }
    return shot.toImage();
}

// A tiny command runner used to drive the UI for screenshots: WRAITH_UI_SCRIPT=<file>, one command per line.
//   nav <page> | instance <name>[|tab] | settings <category> | call <page> <function> [argument] | size <w> <h>
//   newinstance [group] | newinstance-loader <loader> <mc> | newinstance-create <name>|<mc>|<loader>|<group>
//   wait <ms> | shot <file.png> | toast <text> | dump | quit
void Shell::runUiScript(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Wraith UI script not found:" << path;
        return;
    }
    auto lines = std::make_shared<QStringList>(QString::fromUtf8(file.readAll()).split('\n'));
    const QString baseDir = QFileInfo(path).absolutePath();
    auto step = std::make_shared<std::function<void()>>();
    *step = [this, lines, baseDir, step] {
        int delay = 400;
        while (!lines->isEmpty()) {
            const QString line = lines->takeFirst().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;
            const QString command = line.section(' ', 0, 0);
            const QString rest = line.section(' ', 1);
            qDebug() << "Wraith UI script:" << line;
            if (command == "nav") {
                showPage(rest);
            } else if (command == "instance") {
                auto* list = APPLICATION->instances();
                const QString name = rest.section('|', 0, 0).trimmed();
                for (int i = 0; i < list->count(); i++) {
                    if (list->at(i)->name() == name || list->at(i)->id() == name)
                        showInstance(list->at(i), rest.section('|', 1).trimmed());
                }
            } else if (command == "settings") {
                showSettings(rest);
            } else if (command == "newinstance") {
                showNewInstance(rest);
            } else if (command == "newinstance-loader") {
                m_newInstance->selectLoader(rest.section(' ', 0, 0), rest.section(' ', 1, 1));
            } else if (command == "newinstance-create") {
                m_newInstance->create(rest.section('|', 0, 0), rest.section('|', 1, 1), rest.section('|', 2, 2), rest.section('|', 3, 3));
            } else if (command == "call") {
                const QString pageName = rest.section(' ', 0, 0);
                const QString function = rest.section(' ', 1, 1);
                const QString argument = rest.section(' ', 2);
                QObject* target = nullptr;
                if (pageName == "sidebar")
                    target = m_sidebar->rootObject();
                else if (auto* quick = qobject_cast<QQuickWidget*>(page(pageName)))
                    target = quick->rootObject();
                else if (auto* instance = qobject_cast<InstancePage*>(m_stack->currentWidget()); instance && pageName == "instance")
                    instance->scriptCall(function, argument);
                if (target)
                    QMetaObject::invokeMethod(target, function.toUtf8().constData(), Q_ARG(QVariant, argument));
            } else if (command == "size") {
                m_window->resize(rest.section(' ', 0, 0).toInt(), rest.section(' ', 1, 1).toInt());
            } else if (command == "shotpage") {
                if (auto* quick = qobject_cast<QQuickWidget*>(m_stack->currentWidget()))
                    quick->grabFramebuffer().save(QDir::isAbsolutePath(rest) ? rest : QDir(baseDir).filePath(rest));
            } else if (command == "shotsidebar") {
                m_sidebar->grabFramebuffer().save(QDir::isAbsolutePath(rest) ? rest : QDir(baseDir).filePath(rest));
            } else if (command == "dump") {
                qDebug() << "Wraith UI script: window" << m_window->size() << "shell" << geometry() << "stack" << m_stack->geometry()
                         << "page" << (m_stack->currentWidget() ? m_stack->currentWidget()->geometry() : QRect());
            } else if (command == "toast") {
                toast(rest, "success", tr("Undo"), [] {});
            } else if (command == "wait") {
                delay = rest.toInt();
                break;
            } else if (command == "shot") {
                const QString target = QDir::isAbsolutePath(rest) ? rest : QDir(baseDir).filePath(rest);
                QDir().mkpath(QFileInfo(target).absolutePath());
                if (!captureWindow(m_window).save(target))
                    qWarning() << "Wraith UI script: could not save" << target;
            } else if (command == "quit") {
                QTimer::singleShot(200, APPLICATION, &Application::quit);
                return;
            }
        }
        if (!lines->isEmpty())
            QTimer::singleShot(delay, this, *step);
    };
    (*step)();
}

}  // namespace Wraith
