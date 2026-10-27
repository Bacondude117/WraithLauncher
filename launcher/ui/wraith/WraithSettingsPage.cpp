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
#include "WraithSettingsPage.h"

#include <QAction>
#include <QButtonGroup>
#include <QComboBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Application.h"
#include "BuildConfig.h"
#include "security/SecretStore.h"
#include "settings/SettingsObject.h"
#include "ui/MainWindow.h"
#include "ui/pages/BasePageProvider.h"
#include "ui/themes/ITheme.h"
#include "ui/themes/ThemeManager.h"

#include "WraithBackend.h"
#include "WraithPageHost.h"
#include "WraithStyle.h"
#include "WraithWidgets.h"

namespace Wraith {

namespace {
BasePage* createGlobalPage(const QString& id)
{
    BasePage* found = nullptr;
    for (auto* page : APPLICATION->globalSettingsPages()->getPages()) {
        if (!found && page->id() == id)
            found = page;
        else
            delete dynamic_cast<QWidget*>(page);
    }
    return found;
}

QString pageLabel(BasePage* page)
{
    static const QHash<QString, QString> s_labels = {
        { "launcher-settings", QObject::tr("Launcher") },  { "language-settings", QObject::tr("Language") },
        { "appearance-settings", QObject::tr("Appearance") }, { "minecraft-settings", QObject::tr("Game") },
        { "java-settings", QObject::tr("Java") },          { "accounts", QObject::tr("Accounts") },
        { "proxy-settings", QObject::tr("Proxy") },        { "apis", QObject::tr("APIs & services") },
        { "external-tools", QObject::tr("External tools") },
    };
    return s_labels.value(page->id(), page->displayName());
}

QWidget* scrollable(QWidget* body, QWidget* parent)
{
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(body);
    return scroll;
}

QWidget* switchFor(const QString& setting, std::function<void(bool)> onChange, QWidget* parent)
{
    auto* toggle = new Widgets::Switch(parent);
    toggle->setChecked(APPLICATION->settings()->get(setting).toBool());
    QObject::connect(toggle, &Widgets::Switch::toggled, toggle, onChange);
    return toggle;
}
}  // namespace

SettingsPage::SettingsPage(Shell* shell, QWidget* parent) : QWidget(parent), m_shell(shell)
{
    setObjectName("wraithHost");
    setAttribute(Qt::WA_StyledBackground);

    m_categories = {
        { "general", tr("General"), "settings", { "launcher-settings", "language-settings" } },
        { "appearance", tr("Appearance"), "palette", { "appearance-settings" } },
        { "game", tr("Game"), "gamepad", { "minecraft-settings" } },
        { "java", tr("Java & memory"), "cpu", { "java-settings" } },
        { "accounts", tr("Accounts"), "users", { "accounts" }, true },
        { "network", tr("Network"), "network", { "proxy-settings", "apis" }, true },
        { "privacy", tr("Privacy & security"), "shield", {} },
        { "advanced", tr("Advanced"), "sliders", { "external-tools" } },
        { "about", tr("About"), "info", {} },
    };

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_nav = new QWidget(this);
    m_nav->setFixedWidth(240);
    m_navLayout = new QVBoxLayout(m_nav);
    m_navLayout->setContentsMargins(20, 32, 16, 20);
    m_navLayout->setSpacing(4);
    m_navLayout->addWidget(Widgets::title(tr("Settings"), m_nav, 19));
    m_navLayout->addSpacing(14);
    m_navGroup = new QButtonGroup(this);
    for (const auto& category : m_categories) {
        auto* button = new QPushButton(Widgets::literal(category.label), m_nav);
        button->setObjectName("wraithNav");
        button->setCheckable(true);
        button->setCursor(Qt::PointingHandCursor);
        button->setIcon(glyphIcon(category.icon, Color::textMuted, Color::accent));
        button->setIconSize(QSize(18, 18));
        m_navGroup->addButton(button);
        m_navLayout->addWidget(button);
        m_navButtons.insert(category.key, button);
        connect(button, &QPushButton::clicked, this, [this, key = category.key] { showCategory(key); });
    }
    m_navLayout->addStretch(1);
    layout->addWidget(m_nav);

    auto* line = new QFrame(this);
    line->setFixedWidth(1);
    line->setStyleSheet(QString("background-color: %1;").arg(Color::border));
    layout->addWidget(line);

    m_stack = new QStackedWidget(this);
    layout->addWidget(m_stack, 1);
}

void SettingsPage::build()
{
    // fresh pages every time Settings opens, so they show the current values
    QHash<QString, QList<BasePage*>> pagesByCategory;
    for (auto* page : APPLICATION->globalSettingsPages()->getPages()) {
        QString key = "advanced";  // pages added upstream later still get a place
        for (const auto& category : m_categories) {
            if (category.pageIds.contains(page->id()))
                key = category.key;
        }
        pagesByCategory[key].append(page);
    }

    for (const auto& category : m_categories) {
        auto* stack = new QStackedWidget(m_stack);
        PageHost* host = nullptr;
        const auto pages = pagesByCategory.value(category.key);
        if (!pages.isEmpty()) {
            host = new PageHost(stack);
            for (auto* page : pages)
                host->addPage(page, pageLabel(page));
            m_hosts.insert(category.key, host);
        }
        if (!category.pagesFirst)
            stack->addWidget(scrollable(buildSimple(category, host, stack), stack));
        if (host) {
            auto* wrapper = new QWidget(stack);
            auto* wrapperLayout = new QVBoxLayout(wrapper);
            wrapperLayout->setContentsMargins(32, 32, 32, 24);
            wrapperLayout->setSpacing(10);
            wrapperLayout->addWidget(Widgets::title(category.label, wrapper, 18));
            if (!category.pagesFirst) {
                auto* back = new QPushButton(Widgets::literal(tr("Back to %1").arg(category.label)), host);
                back->setCursor(Qt::PointingHandCursor);
                connect(back, &QPushButton::clicked, this, [host, stack] {
                    host->applyAll();
                    host->deactivate();
                    stack->setCurrentIndex(0);
                });
                host->addHeaderWidget(back);
            }
            host->setParent(wrapper);
            wrapperLayout->addWidget(host, 1);
            stack->addWidget(wrapper);
        }
        m_stack->addWidget(stack);
        m_categoryStacks.insert(category.key, stack);
    }
    m_built = true;
}

void SettingsPage::clear()
{
    for (auto* stack : m_categoryStacks) {
        m_stack->removeWidget(stack);
        stack->deleteLater();
    }
    m_categoryStacks.clear();
    m_hosts.clear();
    m_built = false;
}

QWidget* SettingsPage::buildSimple(const Category& category, PageHost* host, QStackedWidget* stack)
{
    auto* body = new QWidget(stack);
    auto* column = new QVBoxLayout(body);
    column->setContentsMargins(32, 32, 32, 32);
    column->setSpacing(18);
    column->setAlignment(Qt::AlignTop);
    column->addWidget(Widgets::title(category.label, body, 18));

    auto settings = APPLICATION->settings();
    auto* backend = m_shell->backend();
    const QString key = category.key;
    auto* card = new Widgets::Card(QString(), body);

    if (key == "general") {
        auto* sorting = new QComboBox(card);
        sorting->addItem(tr("Name"), "Name");
        sorting->addItem(tr("Last played"), "LastLaunch");
        sorting->addItem(tr("Playtime"), "Playtime");
        sorting->setCurrentIndex(std::max(0, sorting->findData(settings->get("InstSortMode"))));
        sorting->setMinimumWidth(200);
        connect(sorting, &QComboBox::activated, this, [this, sorting] {
            // the library model stores the choice in InstSortMode itself
            write("general", [this, sorting] { m_shell->backend()->instances()->setProperty("sortMode", sorting->currentData()); });
        });
        card->addRow(tr("Sort instances by"), tr("Order of the library."), sorting);
        card->addRow(tr("Hide the launcher while playing"), tr("Shows it again when the game closes."),
                     switchFor("CloseAfterLaunch", [this](bool on) { write("game", [on] { APPLICATION->settings()->set("CloseAfterLaunch", on); }); }, card));
        card->addRow(tr("Quit the launcher when the game closes"), QString(),
                     switchFor("QuitAfterGameStop", [this](bool on) { write("game", [on] { APPLICATION->settings()->set("QuitAfterGameStop", on); }); }, card));
        card->addRow(tr("Automatic snapshots"),
                     tr("Save a snapshot before mods, a modpack, Minecraft or a loader change, so you can go back (instance → Snapshots)."),
                     switchFor("WraithSnapshotsAuto", [](bool on) { APPLICATION->settings()->set("WraithSnapshotsAuto", on); }, card));
        auto* ring = new QComboBox(card);
        for (int size : { 5, 10, 20, 50 })
            ring->addItem(tr("%1 snapshots").arg(size), size);
        ring->addItem(tr("All snapshots"), 0);
        ring->setCurrentIndex(std::max(0, ring->findData(settings->get("WraithSnapshotRingSize"))));
        connect(ring, &QComboBox::activated, this, [ring] { APPLICATION->settings()->set("WraithSnapshotRingSize", ring->currentData()); });
        card->addRow(tr("Snapshots to keep per instance"), tr("The oldest are removed first. Pinned snapshots are always kept."), ring);
        card->addRow(tr("Download game files when creating an instance"), tr("Otherwise they download on first launch."),
                     switchFor("DownloadGameFilesDuringInstanceCreation", [this](bool on) {
                         write("general", [on] { APPLICATION->settings()->set("DownloadGameFilesDuringInstanceCreation", on); });
                     }, card));
    } else if (key == "appearance") {
        auto* theme = new QComboBox(card);
        for (auto* entry : APPLICATION->themeManager()->getValidApplicationThemes())
            theme->addItem(entry->name(), entry->id());
        theme->setCurrentIndex(std::max(0, theme->findData(settings->get("ApplicationTheme"))));
        theme->setMinimumWidth(200);
        connect(theme, &QComboBox::activated, this, [this, theme] {
            write("appearance", [theme] {
                APPLICATION->themeManager()->setApplicationTheme(theme->currentData().toString());
                APPLICATION->settings()->set("ApplicationTheme", theme->currentData());
            });
        });
        card->addRow(tr("Dialog theme"), tr("Colours of dialogs and the classic layout. Wraith's own pages always use the Wraith look."), theme);
        card->addRow(tr("Background cat"), tr("A cat behind your library. Packs and opacity are under Advanced."),
                     switchFor("TheCat", [this](bool on) { write("appearance", [on] { APPLICATION->settings()->set("TheCat", on); }); }, card));
        card->addRow(tr("Use the graphics card for the interface"),
                     tr("Smoother animations, but about 150 MB more memory. Takes effect after a restart."),
                     switchFor("WraithGpuInterface", [this](bool on) {
                         APPLICATION->settings()->set("WraithGpuInterface", on);
                         m_shell->toast(tr("Restart Wraith Launcher to apply."), "info");
                     }, card));
    } else if (key == "game") {
        auto setter = [this](const char* name) {
            return [this, name](bool on) { write("game", [name, on] { APPLICATION->settings()->set(name, on); }); };
        };
        card->addRow(tr("Show the game log when the game starts"), QString(), switchFor("ShowConsole", setter("ShowConsole"), card));
        card->addRow(tr("Show the game log when the game crashes"), QString(), switchFor("ShowConsoleOnError", setter("ShowConsoleOnError"), card));
        card->addRow(tr("Start Minecraft maximized"), QString(), switchFor("LaunchMaximized", setter("LaunchMaximized"), card));
        card->addRow(tr("Track playtime"), tr("Shown on instance cards and pages."), switchFor("RecordGameTime", setter("RecordGameTime"), card));
    } else if (key == "java") {
        card->addRow(tr("Memory"), tr("Maximum memory for instances that don't set their own."),
                     Widgets::memorySlider(settings->get("MaxMemAlloc").toInt(), [this](int mib) {
                         write("java", [mib] {
                             APPLICATION->settings()->set("MaxMemAlloc", mib);
                             if (APPLICATION->settings()->get("MinMemAlloc").toInt() > mib)
                                 APPLICATION->settings()->set("MinMemAlloc", mib);
                         });
                     }, card));
        card->addRow(tr("Download Java automatically"), tr("Gets the right Java for each Minecraft version."),
                     switchFor("AutomaticJavaDownload", [this](bool on) {
                         write("java", [on] {
                             APPLICATION->settings()->set("AutomaticJavaDownload", on);
                             APPLICATION->settings()->set("AutomaticJavaSwitch", on);
                         });
                     }, card));
    } else if (key == "privacy") {
        auto setter = [](const char* name) { return [name](bool on) { APPLICATION->settings()->set(name, on); }; };
        auto* notice = Widgets::muted(tr("Wraith has no telemetry, no tracking, no ads and no Wraith accounts or servers. It only "
                                         "contacts the services you use: Mojang and Microsoft for the game and sign-in, the mod "
                                         "and modpack sites you search, and the metadata and Java servers Prism Launcher uses. "
                                         "PRIVACY.md lists every address."),
                                      card);
        notice->setWordWrap(true);
        card->addWidget(notice);
        card->addRow(tr("List popular modpacks when Browse opens"), tr("Off: Browse waits until you search."),
                     switchFor("WraithBrowseLoadPopular", setter("WraithBrowseLoadPopular"), card));
        card->addRow(tr("Tell me about modpack updates"),
                     tr("Off by default. Checks Modrinth and CurseForge for new versions of your modpack instances at start and every "
                        "6 hours; it never updates anything itself. Takes effect after a restart."),
                     switchFor("WraithUpdateNotifications", setter("WraithUpdateNotifications"), card));
        card->addRow(tr("Refresh the translation list on start"), tr("Off: it is only fetched when the language page opens."),
                     switchFor("WraithRefreshTranslationsOnStart", setter("WraithRefreshTranslationsOnStart"), card));
        card->addRow(tr("Refresh version metadata when launching"), tr("Checks for new Minecraft and loader versions before a game starts."),
                     switchFor("MetaRefreshOnLaunch", setter("MetaRefreshOnLaunch"), card));
        card->addRow(tr("Allow unencrypted (http) downloads"),
                     tr("Off: every download uses HTTPS, and a server without HTTPS fails instead of downloading in the clear."),
                     switchFor("WraithAllowInsecureHttp", setter("WraithAllowInsecureHttp"), card));
        card->addRow(tr("Account sign-in tokens"),
                     Wraith::SecretStore::available() ? tr("Encrypted with your Windows login (DPAPI).")
                                                      : tr("Stored in a file only your user can read. Encryption isn't available on this "
                                                           "system yet."),
                     new QWidget(card));
    } else if (key == "advanced") {
        card->addRow(tr("Classic layout"), tr("Use Prism Launcher's toolbar-and-grid layout instead of Wraith's. Takes effect after a restart."),
                     switchFor("WraithClassicLayout", [this](bool on) {
                         APPLICATION->settings()->set("WraithClassicLayout", on);
                         m_shell->toast(tr("Restart Wraith Launcher to switch layouts."), "info");
                     }, card));
        auto* folders = new QPushButton(tr("Open folder..."), card);
        folders->setCursor(Qt::PointingHandCursor);
        connect(folders, &QPushButton::clicked, this, [this] {
            if (auto* menu = m_shell->window()->findChild<QMenu*>("foldersMenu"))
                menu->popup(QCursor::pos());
        });
        card->addRow(tr("Folders"), tr("Instances, mods, icons, Java, skins, themes, logs."), folders);
        auto* clearCache = new QPushButton(tr("Clear"), card);
        clearCache->setCursor(Qt::PointingHandCursor);
        connect(clearCache, &QPushButton::clicked, this, [this, backend] {
            backend->action("actionClearMetadata")->trigger();
            m_shell->toast(tr("Metadata cache cleared"), "success");
        });
        card->addRow(tr("Metadata cache"), tr("Forces version lists to download again."), clearCache);
        auto* logs = new QPushButton(tr("View logs"), card);
        logs->setCursor(Qt::PointingHandCursor);
        connect(logs, &QPushButton::clicked, this, [backend] { backend->action("actionViewLog")->trigger(); });
        card->addRow(tr("Launcher logs"), tr("Current and previous launcher logs."), logs);
    } else if (key == "about") {
        auto* about = new QWidget(card);
        auto* aboutLayout = new QVBoxLayout(about);
        aboutLayout->setContentsMargins(0, 4, 0, 8);
        aboutLayout->setSpacing(6);
        auto* name = Widgets::title(BuildConfig.LAUNCHER_DISPLAYNAME, about, 16);
        aboutLayout->addWidget(name);
        aboutLayout->addWidget(Widgets::muted(tr("Version %1").arg(BuildConfig.printableVersionString()), about));
        auto* notice = new QLabel(tr("Wraith Launcher is a fork of Prism Launcher, made by the Prism Launcher contributors. It is not "
                                     "affiliated with or endorsed by the Prism Launcher project. Licensed under the GNU GPL v3.\n\n"
                                     "Free, with no ads, paywalls or locked features, and no telemetry."),
                                  about);
        notice->setWordWrap(true);
        aboutLayout->addSpacing(6);
        aboutLayout->addWidget(notice);
        card->addWidget(about);
        auto* credits = new QPushButton(tr("Credits and licenses..."), card);
        credits->setCursor(Qt::PointingHandCursor);
        connect(credits, &QPushButton::clicked, this, [backend] { backend->action("actionAbout")->trigger(); });
        card->addRow(tr("Credits"), tr("Everyone who made Prism Launcher, PolyMC and MultiMC, and the libraries used."), credits);
    }
    column->addWidget(card);

    if (host) {
        column->addSpacing(6);
        column->addWidget(Widgets::title(tr("Advanced"), body, 13));
        auto* advanced = new Widgets::Card(QString(), body);
        static const QHash<QString, QString> s_descriptions = {
            { "launcher-settings", QObject::tr("Folders, instance renaming, updates, mod metadata and dependencies, console history, "
                                               "download and task limits") },
            { "language-settings", QObject::tr("Interface language and translations") },
            { "appearance-settings", QObject::tr("Icon theme, cat packs, opacity and scaling, console font, theme folders") },
            { "minecraft-settings", QObject::tr("Window size, console, global data packs, playtime, default account, auto-join, mod loaders, "
                                                "native libraries, performance, custom commands, environment variables") },
            { "java-settings", QObject::tr("Java installations and downloads, detection, memory limits, PermGen, Java arguments") },
            { "external-tools", QObject::tr("Text editor, profilers (JProfiler, VisualVM) and world tools") },
        };
        for (auto* page : host->pages()) {
            const QString pageId = page->id();
            advanced->addWidget(Widgets::disclosure(pageLabel(page), s_descriptions.value(pageId, page->displayName()),
                                                    [this, key, pageId] { showAdvanced(key, pageId); }, advanced));
        }
        column->addWidget(advanced);
    }
    return body;
}

void SettingsPage::write(const QString& categoryKey, const std::function<void()>& change)
{
    auto* host = m_hosts.value(categoryKey);
    QList<BasePage*> reopen;
    if (host) {
        for (auto* page : host->pages()) {
            if (host->wasOpened(page)) {
                page->apply();
                reopen.append(page);
            }
        }
    }
    change();
    for (auto* page : reopen) {
        if (auto* fresh = createGlobalPage(page->id()))
            host->replacePage(page, fresh);
    }
}

void SettingsPage::showCategory(const QString& key)
{
    auto* stack = m_categoryStacks.value(key);
    if (!stack)
        return;
    if (m_current != key) {
        if (auto* previous = m_hosts.value(m_current)) {
            previous->applyAll();
            previous->deactivate();
        }
        m_current = key;
    }
    m_stack->setCurrentWidget(stack);
    m_navButtons.value(key)->setChecked(true);
    const bool pagesFirst = std::any_of(m_categories.begin(), m_categories.end(), [&](const Category& c) { return c.key == key && c.pagesFirst; });
    if (auto* host = m_hosts.value(key); host && (pagesFirst || stack->currentIndex() == 1))
        host->activate();
}

void SettingsPage::showAdvanced(const QString& key, const QString& pageId)
{
    showCategory(key);
    auto* stack = m_categoryStacks.value(key);
    auto* host = m_hosts.value(key);
    if (!stack || !host)
        return;
    stack->setCurrentIndex(stack->count() - 1);
    host->activate();
    host->selectPage(pageId);
}

void SettingsPage::selectCategory(const QString& category)
{
    if (!m_built)
        pageShown();
    for (const auto& entry : m_categories) {
        if (entry.key == category) {
            showCategory(category);
            return;
        }
    }
    // a Prism page id, e.g. "accounts" from the launch dialog or "java-settings" from an instance's settings
    for (const auto& entry : m_categories) {
        if (entry.pageIds.contains(category)) {
            if (entry.pagesFirst) {
                showCategory(entry.key);
                if (auto* host = m_hosts.value(entry.key))
                    host->selectPage(category);
            } else {
                showAdvanced(entry.key, category);
            }
            return;
        }
    }
    showCategory("general");
}

void SettingsPage::pageShown()
{
    if (m_built)
        return;
    emit APPLICATION->globalSettingsAboutToOpen();
    build();
    const QString current = m_current.isEmpty() ? QString("general") : m_current;
    m_current.clear();
    showCategory(current);
}

bool SettingsPage::pageLeaving()
{
    if (!m_built)
        return true;
    for (auto* host : m_hosts) {
        if (!host->applyAll())
            return false;
        host->deactivate();
    }
    emit APPLICATION->globalSettingsApplied();
    clear();
    return true;
}

}  // namespace Wraith
