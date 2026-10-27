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
#include "WraithInstancePage.h"

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QQuickItem>
#include <QQuickWidget>
#include <QScrollArea>
#include <QShortcut>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "InstanceList.h"
#include "InstancePageProvider.h"
#include "minecraft/MinecraftInstance.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"
#include "ui/pages/instance/InstanceSettingsPage.h"
#include "ui/pages/instance/ManagedPackPage.h"
#include "minecraft/mod/DataPackFolderModel.h"
#include "minecraft/mod/ModFolderModel.h"
#include "minecraft/mod/ResourcePackFolderModel.h"
#include "minecraft/mod/ShaderPackFolderModel.h"
#include "minecraft/mod/TexturePackFolderModel.h"

#include "WraithBackend.h"
#include "WraithListModels.h"
#include "WraithPageHost.h"
#include "WraithStyle.h"
#include "WraithWidgets.h"

namespace Wraith {

namespace {
const QStringList s_contentPages = { "mods", "resourcepacks", "shaderpacks", "datapacks", "texturepacks", "coremods", "nilmods" };

QString tabForPage(const QString& id)
{
    if (s_contentPages.contains(id))
        return "content";
    if (id == "worlds" || id == "screenshots" || id == "servers")
        return id;
    if (id == "console" || id == "logs")
        return "logs";
    if (id == "friend_server")
        return "friend";
    return "settings";
}

QString pageLabel(BasePage* page)
{
    static const QHash<QString, QString> s_labels = {
        { "console", QObject::tr("Game log") },
        { "logs", QObject::tr("Older logs") },
        { "settings", QObject::tr("Game & Java") },
        { "version", QObject::tr("Versions & components") },
        { "managed_pack", QObject::tr("Modpack") },
        { "notes", QObject::tr("Notes") },
        { "mods", QObject::tr("Mods") },
        { "coremods", QObject::tr("Core mods") },
        { "nilmods", QObject::tr("Nil mods") },
        { "resourcepacks", QObject::tr("Resource packs") },
        { "texturepacks", QObject::tr("Texture packs") },
        { "shaderpacks", QObject::tr("Shaders") },
        { "datapacks", QObject::tr("Data packs") },
    };
    return s_labels.value(page->id(), page->displayName());
}

QWidget* padded(QWidget* inner, QWidget* parent, int top = 20)
{
    auto* wrapper = new QWidget(parent);
    wrapper->setObjectName("wraithHost");
    wrapper->setAttribute(Qt::WA_StyledBackground);
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(32, top, 32, 24);
    inner->setParent(wrapper);
    layout->addWidget(inner);
    return wrapper;
}
}  // namespace

InstancePage::InstancePage(Shell* shell, MinecraftInstance* instance, QWidget* parent)
    : QWidget(parent), m_shell(shell), m_instance(instance)
{
    setObjectName("wraithInstancePage");
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_header = shell->createQuick("InstanceHeader.qml", this);
    m_header->setFixedHeight(300);
    layout->addWidget(m_header);

    m_tabs = new QStackedWidget(this);
    layout->addWidget(m_tabs, 1);

    // the same pages Prism's instance window has, grouped into tabs
    InstancePageProvider provider(instance);
    QHash<QString, QList<BasePage*>> byTab;
    for (auto* page : provider.getPages())
        byTab[tabForPage(page->id())].append(page);

    struct Tab {
        QString key;
        QString label;
    };
    const QList<Tab> tabs = { { "content", tr("Content") }, { "worlds", tr("Worlds") },   { "screenshots", tr("Screenshots") },
                              { "servers", tr("Servers") }, { "logs", tr("Logs") },       { "friend", tr("Friend Server") },
                              { "settings", tr("Settings") } };
    QVariantList tabModel;
    for (const auto& tab : tabs) {
        const auto pages = byTab.value(tab.key);
        QWidget* widget = nullptr;
        if (tab.key == "content")
            widget = buildContentTab(pages);
        else if (tab.key == "worlds" && pages.size() == 1)
            widget = buildWorldsTab(pages.first());
        else if (tab.key == "settings")
            widget = buildSettingsTab(pages);
        else if (!pages.isEmpty())
            widget = padded(hostTab(tab.key, pages), m_tabs);
        if (!widget)
            continue;
        m_tabs->addWidget(widget);
        m_tabWidgets.insert(tab.key, widget);
        tabModel.append(QVariantMap{ { "key", tab.key }, { "label", tab.label } });
    }

    if (auto* root = m_header->rootObject()) {
        root->setProperty("instanceId", instance->id());
        root->setProperty("tabs", tabModel);
        connect(root, SIGNAL(tabSelected(QString)), this, SLOT(onTabSelected(QString)));
    }

    // the modpack page closes the instance window after an update; there is none here
    if (auto* host = m_hosts.value("settings"))
        if (auto* managed = dynamic_cast<ManagedPackPage*>(host->getPage("managed_pack")))
            managed->setInstanceWindow(nullptr);

    showTab("content");

    // Esc goes back to the library
    new QShortcut(QKeySequence(Qt::Key_Escape), this, this, [shell] { shell->showPage("library"); }, Qt::WidgetWithChildrenShortcut);

    // Prism's instance shortcuts, for this instance
    auto* backend = shell->backend();
    const QString id = instance->id();
    auto shortcut = [this](const QKeySequence& keys, std::function<void()> run) {
        new QShortcut(keys, this, this, std::move(run), Qt::WidgetWithChildrenShortcut);
    };
    shortcut(QKeySequence(tr("Ctrl+K")), [backend, id] { backend->stop(id); });
    shortcut(QKeySequence(tr("Ctrl+G")), [backend, id] { backend->changeGroup(id); });
    shortcut(QKeySequence(tr("Ctrl+D")), [backend, id] { backend->duplicate(id); });
    shortcut(QKeySequence(tr("Ctrl+I")), [this] { showTab("settings"); });
    shortcut(QKeySequence(Qt::Key_F2), [backend, id] { backend->rename(id); });
    shortcut(QKeySequence(tr("Ctrl+Return")), [backend, id] { backend->launch(id); });
    // Delete stays with the lists on this page (mods, worlds); in the library it deletes the focused instance

    // like Prism's console window: show the game log when the game starts, if the console setting asks for it
    connect(instance, &BaseInstance::runningStatusChanged, this, [this](bool running) {
        if (running && isVisible() && m_instance->settings()->get("ShowConsole").toBool())
            selectTab("console");
    });
}

InstancePage::~InstancePage()
{
    for (auto* host : m_hosts)
        host->deactivate();
}

PageHost* InstancePage::hostTab(const QString& tab, const QList<BasePage*>& pages)
{
    auto* host = new PageHost(this);
    for (auto* page : pages)
        host->addPage(page, pageLabel(page));
    m_hosts.insert(tab, host);
    return host;
}

QWidget* InstancePage::buildContentTab(const QList<BasePage*>& pages)
{
    auto* stack = new QStackedWidget(m_tabs);
    m_simpleStacks.insert("content", stack);

    auto* list = m_shell->createQuick("ContentList.qml", stack);
    stack->addWidget(list);

    // full original pages, behind "Advanced"
    auto* host = hostTab("content", pages);
    auto* back = new QPushButton(tr("Back to simple view"), host);
    back->setCursor(Qt::PointingHandCursor);
    connect(back, &QPushButton::clicked, this, [this, stack, host] {
        host->applyAll();
        host->deactivate();
        stack->setCurrentIndex(0);
        showTab(m_tab);
    });
    host->addHeaderWidget(back);
    stack->addWidget(padded(host, stack));

    QVariantList types;
    for (auto* page : pages) {
        if (!page->shouldDisplay())
            continue;
        ResourceFolderModel* model = nullptr;
        const QString id = page->id();
        if (id == "mods")
            model = m_instance->loaderModList();
        else if (id == "coremods")
            model = m_instance->coreModList();
        else if (id == "nilmods")
            model = m_instance->nilModList();
        else if (id == "resourcepacks")
            model = m_instance->resourcePackList();
        else if (id == "texturepacks")
            model = m_instance->texturePackList();
        else if (id == "shaderpacks")
            model = m_instance->shaderPackList();
        else if (id == "datapacks")
            model = m_instance->dataPackList();
        if (!model)
            continue;
        types.append(QVariantMap{ { "key", id }, { "label", pageLabel(page) }, { "model", QVariant::fromValue<QObject*>(new ResourceListModel(model, list)) } });
    }
    if (auto* root = list->rootObject()) {
        root->setProperty("types", types);
        connect(root, SIGNAL(action(QString, QString)), this, SLOT(onContentAction(QString, QString)));
    }
    return stack;
}

void InstancePage::onContentAction(const QString& action, const QString& pageId)
{
    auto* host = m_hosts.value("content");
    if (!host)
        return;
    if (action == "advanced") {
        showAdvanced(m_simpleStacks.value("content"), host, pageId);
        return;
    }
    // the rest are the original page's own actions (download dialog, add file, update check)
    static const QHash<QString, const char*> s_actions = {
        { "browse", "actionDownloadItem" }, { "add", "actionAddItem" }, { "updates", "actionUpdateItem" }, { "folder", "actionViewFolder" }
    };
    auto* page = dynamic_cast<QWidget*>(host->getPage(pageId));
    if (!page || !s_actions.contains(action))
        return;
    if (auto* qaction = page->findChild<QAction*>(s_actions.value(action)))
        qaction->trigger();
}

QWidget* InstancePage::buildWorldsTab(BasePage* page)
{
    auto* stack = new QStackedWidget(m_tabs);
    m_simpleStacks.insert("worlds", stack);

    auto* list = m_shell->createQuick("WorldsList.qml", stack);
    stack->addWidget(list);

    auto* host = hostTab("worlds", { page });
    auto* back = new QPushButton(tr("Back to simple view"), host);
    back->setCursor(Qt::PointingHandCursor);
    connect(back, &QPushButton::clicked, this, [this, stack, host] {
        host->deactivate();
        stack->setCurrentIndex(0);
        showTab(m_tab);
    });
    host->addHeaderWidget(back);
    stack->addWidget(padded(host, stack));

    if (auto* root = list->rootObject()) {
        root->setProperty("worlds", QVariant::fromValue<QObject*>(new WorldListModel(m_instance, list)));
        root->setProperty("instanceId", m_instance->id());
        // "Advanced" opens the full world tools (add, rename, copy, data packs, external tools, reset icon)
        auto* relay = new QAction(list);
        connect(relay, &QAction::triggered, this, [this, stack, host] { showAdvanced(stack, host, "worlds"); });
        QObject::connect(root, SIGNAL(advanced()), relay, SLOT(trigger()));
    }
    return stack;
}

QWidget* InstancePage::buildSettingsTab(const QList<BasePage*>& pages)
{
    auto* stack = new QStackedWidget(m_tabs);
    m_simpleStacks.insert("settings", stack);

    // simple view
    auto* scroll = new QScrollArea(stack);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget(scroll);
    body->setObjectName("wraithHost");
    auto* column = new QVBoxLayout(body);
    column->setContentsMargins(32, 22, 32, 32);
    column->setSpacing(18);
    column->setAlignment(Qt::AlignTop);

    auto* essentials = new Widgets::Card(tr("Essentials"), body);
    auto* name = new QLineEdit(m_instance->name(), essentials);
    name->setMinimumWidth(300);
    connect(name, &QLineEdit::editingFinished, this, [this, name] {
        const QString text = name->text().trimmed();
        if (!text.isEmpty() && text != m_instance->name())
            m_shell->window()->wraithRenameInstance(m_instance, text);
        name->setText(m_instance->name());
    });
    essentials->addRow(tr("Name"), tr("Shown in your library."), name);

    auto* group = new QComboBox(essentials);
    group->setEditable(true);
    group->setMinimumWidth(300);
    group->addItem(QString());
    group->addItems(m_shell->backend()->groups());
    group->setCurrentText(APPLICATION->instances()->getInstanceGroup(m_instance->id()));
    group->lineEdit()->setPlaceholderText(tr("No group"));
    connect(group->lineEdit(), &QLineEdit::editingFinished, this, [this, group] {
        APPLICATION->instances()->setInstanceGroup(m_instance->id(), group->currentText().trimmed());
    });
    connect(group, &QComboBox::activated, this, [this, group] {
        APPLICATION->instances()->setInstanceGroup(m_instance->id(), group->currentText().trimmed());
    });
    essentials->addRow(tr("Group"), tr("Library filter; type a new name to create a group."), group);

    auto settings = m_instance->settings();
    auto* memoryBox = new QWidget(essentials);
    auto* memoryRow = new QHBoxLayout(memoryBox);
    memoryRow->setContentsMargins(0, 0, 0, 0);
    memoryRow->setSpacing(14);
    auto* custom = new Widgets::Switch(memoryBox);
    custom->setChecked(settings->get("OverrideMemory").toBool());
    auto* slider = Widgets::memorySlider(settings->get("MaxMemAlloc").toInt(), [this](int mib) {
        refreshSettingsPage();
        m_instance->settings()->set("OverrideMemory", true);
        m_instance->settings()->set("MaxMemAlloc", mib);
        if (m_instance->settings()->get("MinMemAlloc").toInt() > mib)
            m_instance->settings()->set("MinMemAlloc", mib);
        if (auto* page = dynamic_cast<InstanceSettingsPage*>(m_hosts.value("settings")->getPage("settings")))
            page->loadSettings();
    }, memoryBox);
    slider->setEnabled(custom->isChecked());
    connect(custom, &Widgets::Switch::toggled, this, [this, slider](bool on) {
        slider->setEnabled(on);
        refreshSettingsPage();
        m_instance->settings()->set("OverrideMemory", on);
        if (auto* page = dynamic_cast<InstanceSettingsPage*>(m_hosts.value("settings")->getPage("settings")))
            page->loadSettings();
    });
    memoryRow->addWidget(slider);
    memoryRow->addWidget(custom);
    essentials->addRow(tr("Memory"), tr("Maximum memory for this instance. Off uses the global setting."), memoryBox);

    auto* javaButton = new QPushButton(tr("Java options..."), essentials);
    javaButton->setCursor(Qt::PointingHandCursor);
    const QString javaPath = settings->get("OverrideJavaLocation").toBool() ? settings->get("JavaPath").toString() : QString();
    essentials->addRow(tr("Java"), javaPath.isEmpty() ? tr("Uses the global Java setting (picked automatically).") : javaPath, javaButton);
    column->addWidget(essentials);

    auto* actions = new Widgets::Card(tr("Instance"), body);
    auto* buttons = new QWidget(actions);
    auto* buttonRow = new QHBoxLayout(buttons);
    buttonRow->setContentsMargins(0, 8, 0, 4);
    buttonRow->setSpacing(10);
    auto addButton = [&](const QString& text, std::function<void()> onClick, const QString& objectName = QString()) {
        auto* button = new QPushButton(text, buttons);
        button->setObjectName(objectName);
        button->setCursor(Qt::PointingHandCursor);
        connect(button, &QPushButton::clicked, this, onClick);
        buttonRow->addWidget(button);
        return button;
    };
    const QString id = m_instance->id();
    auto* backend = m_shell->backend();
    addButton(tr("Open folder"), [backend, id] { backend->openFolder(id); });
    addButton(tr("Duplicate..."), [backend, id] { backend->duplicate(id); });
    auto* exportButton = addButton(tr("Export"), [] {});
    if (auto* exportMenu = backend->action("actionExportInstance")->menu()) {
        exportButton->setMenu(exportMenu);
        connect(exportMenu, &QMenu::aboutToShow, this, [backend, id] { backend->select(id); });
    }
    addButton(tr("Create shortcut..."), [backend, id] {
        backend->select(id);
        backend->action("actionCreateInstanceShortcut")->trigger();
    });
    buttonRow->addStretch(1);
    addButton(tr("Delete instance..."), [backend, id] { backend->remove(id); }, "wraithDanger");
    actions->addWidget(buttons);
    column->addWidget(actions);

    // advanced: every original page that isn't elsewhere
    auto* advancedTitle = Widgets::title(tr("Advanced"), body, 13);
    column->addSpacing(6);
    column->addWidget(advancedTitle);
    column->addWidget(Widgets::muted(tr("Every option of the instance, as in the full editor."), body));
    auto* host = hostTab("settings", pages);
    auto* advanced = new Widgets::Card(QString(), body);
    static const QHash<QString, QString> s_descriptions = {
        { "settings", QObject::tr("Java installation and arguments, memory, window size, console, account, auto-join, mod loaders, "
                                  "native libraries, performance, custom commands, environment variables") },
        { "version", QObject::tr("Minecraft and loader versions, libraries, agents, jar mods and component JSON") },
        { "managed_pack", QObject::tr("Update or change the version of the modpack this instance came from") },
        { "notes", QObject::tr("Your own notes about this instance") },
    };
    for (auto* page : pages) {
        if (!page->shouldDisplay())
            continue;
        const QString pageId = page->id();
        advanced->addWidget(Widgets::disclosure(pageLabel(page), s_descriptions.value(pageId, page->displayName()),
                                                [this, stack, host, pageId] { showAdvanced(stack, host, pageId); }, advanced));
    }
    column->addWidget(advanced);
    connect(javaButton, &QPushButton::clicked, this, [this, stack, host] { showAdvanced(stack, host, "settings"); });

    scroll->setWidget(body);
    stack->addWidget(scroll);

    auto* back = new QPushButton(tr("Back to settings"), host);
    back->setCursor(Qt::PointingHandCursor);
    connect(back, &QPushButton::clicked, this, [stack, host] {
        host->applyAll();
        host->deactivate();
        stack->setCurrentIndex(0);
    });
    host->addHeaderWidget(back);
    stack->addWidget(padded(host, stack));
    return stack;
}

void InstancePage::refreshSettingsPage()
{
    // keep edits made in the full settings page before a simple control writes the same settings
    auto* host = m_hosts.value("settings");
    if (auto* page = host ? host->getPage("settings") : nullptr; page && host->wasOpened(page))
        page->apply();
}

void InstancePage::showAdvanced(QStackedWidget* stack, PageHost* host, const QString& pageId)
{
    stack->setCurrentIndex(1);
    host->activate();
    host->selectPage(pageId);
}

void InstancePage::showTab(const QString& tab)
{
    auto* widget = m_tabWidgets.value(tab);
    if (!widget)
        return;
    if (m_tab != tab) {
        if (auto* previous = m_hosts.value(m_tab)) {
            previous->applyAll();
            previous->deactivate();
        }
        m_tab = tab;
    }
    m_tabs->setCurrentWidget(widget);
    // tabs without a simple view show their pages directly
    auto* host = m_hosts.value(tab);
    auto* stack = m_simpleStacks.value(tab);
    if (host && (!stack || stack->currentIndex() == 1))
        host->activate();
    // simple Qt Quick lists load their folder when shown
    if (stack && stack->currentIndex() == 0) {
        if (auto* quick = qobject_cast<QQuickWidget*>(stack->widget(0)); quick && quick->rootObject())
            QMetaObject::invokeMethod(quick->rootObject(), "refresh");
    }
    if (auto* root = m_header->rootObject())
        root->setProperty("currentTab", tab);
}

void InstancePage::onTabSelected(const QString& tab)
{
    showTab(tab);
}

void InstancePage::selectTab(const QString& tabOrPage)
{
    if (tabOrPage.isEmpty())
        return;
    if (m_tabWidgets.contains(tabOrPage)) {
        showTab(tabOrPage);
        return;
    }
    // a Prism page id, e.g. from "Edit instance > Mods" or the console opening on launch
    const QString tab = tabForPage(tabOrPage);
    showTab(tab);
    auto* host = m_hosts.value(tab);
    if (!host)
        return;
    if (auto* stack = m_simpleStacks.value(tab)) {
        if (tab == "content" && tabOrPage == "mods")
            return;  // the simple mod list is the right place
        showAdvanced(stack, host, tabOrPage);
    } else {
        host->selectPage(tabOrPage);
    }
}

void InstancePage::pageShown()
{
    showTab(m_tab.isEmpty() ? "content" : m_tab);
}

bool InstancePage::pageLeaving()
{
    for (auto* host : m_hosts) {
        if (!host->applyAll())
            return false;
    }
    if (auto* host = m_hosts.value(m_tab))
        host->deactivate();
    return true;
}

void InstancePage::scriptCall(const QString& function, const QString& argument)
{
    if (function == "tab")
        selectTab(argument);
    else if (function == "advanced") {
        const QString tab = tabForPage(argument);
        showTab(tab);
        if (auto* stack = m_simpleStacks.value(tab))
            showAdvanced(stack, m_hosts.value(tab), argument);
    }
}

}  // namespace Wraith
