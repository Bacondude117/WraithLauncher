# Wraith Launcher: UI design

## Goal
An app-like launcher that a Prism user would not recognise from screenshots. It is dark, calm and card-based,
uses a slim labelled sidebar, and has no toolbars or menu bar. The backend (instances, loaders, downloads,
accounts, Java, every advanced option) is Prism's, unchanged.

## Technical approach

**A hybrid Qt Quick shell on top of the unchanged C++ backend.**

- `MainWindow` stays as the *controller*. Its actions and slots (launch, delete with trash/undo, export, copy,
  shortcuts, accounts menu, URL/drag-drop import, instance creation tasks) are battle-tested and are what
  upstream fixes land in. When the new UI is active, its classic central widget, toolbars, menu bar and status
  bar are hidden (not deleted), and the window's central widget becomes the **Wraith shell**. The classic layout
  can still be turned back on (Settings → Advanced → Classic layout), so nothing is lost.
- The **shell** (`launcher/ui/wraith/`) is a widget with a sidebar and a page stack. The primary screens are written
  in **QML** (Qt Quick + Qt Quick Controls Basic + QtQuick.Effects) and hosted in `QQuickWidget`s that share one
  `QQmlEngine`: Sidebar, Home, Library, Browse, Servers, the instance header, and the Content and Worlds lists.
  QML gives real hover/press states, animated transitions, blurred cover art and smooth scrolling, which Widgets
  resist.
- **Power-user surfaces reuse the original Widgets pages** (`BasePage` subclasses: mod manager, components, logs,
  per-instance overrides, global settings pages). They are embedded *inside* the shell (never as dialogs) by
  `WraithPageHost`, which implements `BasePageContainer`. They are restyled by the Wraith stylesheet: their side
  toolbars move to a top action bar and pick up the Wraith palette, radii and spacing. This is the progressive
  disclosure: the simple Wraith view is first, and "Advanced" opens the full original page in place.
- **A C++ bridge** (`WraithBackend`) exposes QML-friendly models and invokables: instance model with
  name/icon/loader/version/playtime/last-played/running roles, sort/filter proxies, unified search, friend servers,
  account, toasts and navigation. Instance icons are served by an image provider (`image://wraith/icon/<key>`).
- **Upstream friendliness:** all new code is in new files. Upstream files only get small hooks: MainWindow (hide
  classic chrome, a few public wrappers), Application (route instance windows and global settings into the
  shell), and CMake (Qt Quick modules, QML resources, deployment of QML imports).
- **Testing hook:** when `WRAITH_UI_SCRIPT` points to a script file, the shell runs it after start-up (navigate,
  open an instance, type a search, wait, screenshot via `QWidget::grab`, quit). It is used to produce the
  screenshots in `/screenshots`, and it does nothing unless that variable is set.

## Navigation

```
┌──────────┬──────────────────────────────────────────────┐
│  ghost   │  Page title                 [search] [+ New] │
│  Wraith  │                                              │
│          │  page content                                │
│ ⌂ Home   │                                              │
│ ▦ Library│                                              │
│ ⌕ Browse │                                              │
│ ⇄ Servers│                                              │
│          │                                              │
│          │                                              │
│ ⚙ Settings                                              │
│ ⋯ More   │                                     [toast]  │
│ (avatar) │                                              │
└──────────┴──────────────────────────────────────────────┘
```

- **Sidebar** (220 px): logo and wordmark; Home, Library, Browse, Servers; a flexible gap; Settings and More
  (folders, help, about, update, cat); the account chip (avatar initial, name, "Microsoft"/"Offline") opens the
  account menu. The active item has a filled pill and an accent bar. Running instances get a small "Playing"
  indicator on Library.
- **Instance page** is pushed from Home/Library/Servers and has a back button ("← Library").
- Keyboard: Ctrl+N new instance, Ctrl+, settings, Ctrl+F focus search, Esc back.

## Screens

### Home
- Greeting ("Welcome back") and total playtime.
- **Continue playing** hero: the most recently played instance on a wide card. Blurred cover, icon, name,
  loader · version, "Last played 2 hours ago", and a big Play button.
- **Recent** row: up to 6 more recently played instances as cards.
- **Quick actions**: New instance, Browse modpacks, Import, Start a friend server.
- **Popular modpacks**: the first results of the unified browser (an empty query sorted by popularity), each linking
  to Browse.
- Empty state (no instances): an illustration-style ghost, "Your library is empty", plus New instance and Browse
  modpacks buttons.

### Library
- Header: title, instance count, search field, and New instance.
- Filter bar: loader chips (All, Vanilla, Fabric, Quilt, Forge, NeoForge, Other), a group dropdown (with rename and
  delete group), and a sort dropdown (Name, Last played, Playtime, uses the global sorting setting).
- Grid of **instance cards** (220×250): cover (blurred, darkened icon art with a gradient), icon, name, loader badge
  and version badge, and "Played 3h · 2 days ago". On hover the card lifts, its border brightens and a round
  Play button fades in. A running instance shows a green "Playing" badge and the button becomes Stop. The ⋯ button
  and right-click open the instance menu.
- Empty states: no instances / no search results.

### Instance page
- Header: a wide banner of the blurred cover; on it the icon (click to change), the name (click to rename), and
  badges (loader, Minecraft version, group). Stats: playtime and last played. Actions: big Play (▾ launch menu) or
  Stop, Open folder, ⋯ (duplicate, export, shortcut, change group, delete).
- Tabs (underlined, animated indicator): **Content, Worlds, Screenshots, Servers, Logs, Friend Server, Settings**.
  - Content: a type selector (Mods, Resource packs, Shaders, Data packs, Texture packs, plus Core/Nil mods when
    present); search; rows with icon, name, version, file, enable switch and remove; Browse content, Add file,
    Check for updates, Open folder; **Advanced** opens the full mod manager.
  - Worlds: world cards (icon, name, game mode, last played, seed); Play (quick-join), Open folder; Advanced opens
    the full world tools.
  - Logs: Game log / Older logs selector, hosting the original console pages.
  - Settings: **Essentials** (memory, Java, window size, group) and **Advanced** (Game & Java overrides, Versions &
    components, Modpack, Notes).

### Browse
- Search field across the top; a left **filter panel** (Sources with per-source result counts and checkboxes, sort:
  Relevance/Downloads, and a CurseForge-key notice); results as cards (icon, title, author, summary, source tags,
  downloads).
- Clicking a card opens a **detail panel** with a larger icon, summary, sources and links. Install:
  - Modrinth: a version list fetched from Modrinth; Install creates the instance directly, with progress shown
    as a toast-style task card.
  - Other sources: "Install with <source>..." opens that source's page in the advanced dialog, with the search
    pre-filled.
- "Advanced: per-source browsers" link opens the original per-source pages.

### Servers
- Header with New server; a card per friend server: icon, name, loader · version · port, status pill
  (Online/Offline/Starting), Start/Stop and Manage (the instance's Friend Server tab). There is an empty state with
  an explanation.

### Settings
- Category list on the left: General, Appearance, Game, Java & memory, Accounts, Content, Storage, Network,
  Advanced, About.
- Each category shows simple rows first (label, description, control on the right) and then an **Advanced**
  disclosure that embeds the matching original page(s). Changes in embedded pages are applied when you leave the
  category or the Settings page.

### Toasts
- Bottom-right stack, max 3, auto-dismiss after 4 s (errors 8 s), optional action button (Undo, Open). Used for:
  instance created/copied/deleted (Undo), launch failed, server started/stopped, settings saved, install started.

## Visual system

### Colour tokens (dark)
| Token | Value | Use |
|---|---|---|
| `bg` | `#0e1016` | window background |
| `sidebar` | `#0a0c11` | sidebar |
| `surface` | `#161922` | cards, panels |
| `surfaceHover` | `#1d2130` | hovered cards |
| `surfaceRaised` | `#222738` | menus, popups, inputs |
| `border` | `#262b3a` | 1 px borders |
| `borderStrong` | `#353b50` | hovered borders |
| `text` | `#e8eaf2` | primary text |
| `textMuted` | `#9aa1b5` | secondary text |
| `textFaint` | `#636a80` | hints |
| `accent` | `#7fdbea` | pale cyan: primary actions, focus, active nav |
| `accentText` | `#0b1418` | text on accent |
| `violet` | `#a58cff` | secondary accent, links, tags |
| `success` | `#4fd18b` | online, playing |
| `warning` | `#f2c45a` | |
| `danger` | `#ff6b7d` | stop, delete |

Loader badge colours: Fabric `#dbb68f`, Quilt `#c58cff`, Forge `#7fa7ff`, NeoForge `#f5935c`, Vanilla `#7fd88f`.

### Spacing, shape, type
- 4 px grid: spacing 4/8/12/16/24/32; page margins 32 px; card padding 16 px.
- Radii: 6 (chips, inputs), 10 (buttons), 14 (cards), 20 (hero, sheets), pills fully round.
- Type: the system UI font ("Segoe UI Variable"/"Segoe UI" on Windows). Page title 26 px semibold, section 17 px
  semibold, card title 15 px semibold, body 13 px, caption 11.5 px. Muted colour for secondary text instead of
  smaller sizes.
- Elevation: cards are flat with a 1 px border; hover raises them 2 px with a soft shadow; popups get a 24 px shadow.

### Motion
- 140 ms hover/press colour changes, 220 ms page enter (fade + 12 px rise), 180 ms tab indicator slide, toast slide-in
  250 ms with OutCubic easing. Nothing loops except loading spinners.

## What deliberately stays original
Task progress, network-failure and blocked-mod dialogs, the first-run wizard, and the remaining specialised dialogs
marked 🟡 in FEATURES.md. They are restyled with the Wraith palette and open from Wraith controls; redesigning each
is follow-up work.
