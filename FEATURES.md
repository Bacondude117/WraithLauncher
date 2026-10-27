# Wraith Launcher: feature map

This file lists every screen, dialog, action and option in the Prism-derived UI, and where it lives in the new
Wraith interface. Nothing is removed. Anything without a simple Wraith control is one click away under an
**Advanced** section, which hosts the original page inside the new shell (never as a separate window).

Status:
- ✅ Verified in the running app, by screenshot (`/screenshots`) or by a scripted run.
- ☑️ Implemented and wired to the original action or setting, but not exercised in this session (it usually needs
  a game launch or account).
- 🟡 Opens the original dialog, restyled. It is reachable, but redesigning it is follow-up work.

Navigation:
- The **sidebar** has Home, Library, Browse, Servers and Settings. The account chip and a "More" menu sit at the
  bottom.
- The **instance page** opens from any instance card.
- If someone prefers Prism's layout, **Settings → Advanced → Classic layout** brings it back (after a restart).

## 1. Main window

| Prism | Wraith | Status |
|---|---|---|
| Instance grid (groups, icons, names) | **Library**: card grid with cover art, loader and version badges, search, loader filter chips, group filter, and sort | ✅ |
| Double-click an instance to launch (or edit, per setting) | The card's **Play** button (on hover) launches; clicking the card opens the instance page. The double-click setting only applies to the classic layout | ✅ |
| Instance context menu (all instance actions, rename, change icon) | Right-click a card, or use its ⋯ button: Play/Stop, Open, Open folder, Rename, Change icon, Change group, Duplicate, Export, Create shortcut, Delete | ☑️ |
| Group context menu (create instance in group, rename group, delete group) | Library: pick a group in the group filter, then its ⋯ menu offers New instance in this group, Rename group and Delete group | ☑️ |
| Collapsible groups | Group filter dropdown, plus the group shown on the instance page | ✅ |
| Drag and drop zips/URLs onto the window to import | Drop onto Home or Library (Prism's import flow) | ☑️ |
| Cat background (and its opacity/fit settings) | Drawn behind Home and Library when enabled (Settings → Appearance) | ☑️ |
| News toolbar / More News | This fork configures no news feed, so there is nothing to show. The classic layout keeps the news bar for builds that set a feed | ✅ |
| Status bar: selected instance description, total playtime | The instance page header shows playtime and last played; Home shows total playtime | ✅ |

### Main toolbar and menu bar

| Prism action | Wraith | Status |
|---|---|---|
| Add Instance (Ctrl+N) | **New instance** sheet from Home, Library, Ctrl+N | ✅ |
| Folders menu: Launcher Data, Launcher Logs, Instances, Instance Icons, Mods (Quick Access), Auto Downloaded Java, Skins, Widget Themes, Icon Themes, Cat Packs | Sidebar **More → Folders** (the same menu), and Settings → Advanced → Folders | ☑️ |
| Settings (Ctrl+,) | Sidebar **Settings**; Ctrl+, opens it (Prism's settings dialog is routed to the page) | ✅ |
| Help menu: Clear Metadata Cache, Report a Bug, View logs, Install to PATH (macOS), Matrix/Discord/Reddit, More News, Check for updates, Wiki, About | Sidebar **More → Help** (the same menu); Settings → Advanced (clear cache, logs) and Settings → About | ☑️ |
| Update... | Sidebar More menu (hidden while the updater is disabled, as in Prism) | ☑️ |
| Meow (cat toggle) | Settings → Appearance → Background cat; also the More menu | ☑️ |
| Accounts menu: switch account, No Default, Manage Accounts, Manage Skins | Sidebar **account chip** opens the same menu; Settings → Accounts | ☑️ |
| Edit menu: Undo Last Instance Deletion (Ctrl+Z) | An **Undo** button on the toast that confirms a deletion; More → Undo | ☑️ |
| View menu: Themes, Status Bar, Lock Toolbars, toolbar toggles | Themes: Settings → Appearance. The toolbar and status bar exist only in the classic layout | ✅ |
| Close Window (Ctrl+W), Quit (Ctrl+Q) | The same shortcuts; More → Quit | ☑️ |
| Replace toolbar with menubar (setting) | Classic layout only; in Settings → General → Advanced → Launcher | ✅ |
| Konami code | Classic layout | ☑️ |

### Instance toolbar (right column)

| Prism action | Wraith | Status |
|---|---|---|
| Change icon (click the big icon) | Instance header: click the icon; ⋯ menu | ☑️ |
| Rename (editable label) | Instance header: click the name; ⋯ menu; Settings tab → Name. Prism's folder-renaming rules still apply | ☑️ |
| Launch (+ launch menu: offline, demo, profilers) | **Play** on cards, Home and the instance header. The ▾ next to Play holds the launch menu | ☑️ |
| Kill (Ctrl+K) | Play turns into **Stop** while the game runs; the sidebar shows "N playing" | ☑️ |
| Edit... (Ctrl+I) | Opens the instance page (Prism's "edit instance" requests are routed there too) | ✅ |
| Change Group... (Ctrl+G) | ⋯ menu; Settings tab → Group | ☑️ |
| Folder | Instance header folder button; Settings tab → Open folder | ☑️ |
| Export (Prism zip / Modrinth mrpack / CurseForge zip) | ⋯ menu → Export; Settings tab → Export | ☑️ |
| Copy... (Ctrl+D) | ⋯ menu → Duplicate; Settings tab | ☑️ |
| Delete | ⋯ menu → Delete; Settings tab → Delete instance. Afterwards a toast offers Undo | ☑️ |
| Create Shortcut | ⋯ menu; Settings tab | ☑️ |

## 2. Instance window (Edit Instance) → instance page

The instance page is a full page in the shell. Its header has the cover art, icon, name, badges, playtime, last
played, Play/Stop with the ▾ launch menu, Open folder and ⋯. Below it are tabs. Esc goes back to the library.

| Prism page | Wraith | Status |
|---|---|---|
| Minecraft Log (console: keep updating, wrap, color, copy, upload, clear, find, bottom) | **Logs** tab → Game log (the original console) | ✅ |
| Other Logs | **Logs** tab → Older logs | ✅ |
| Version (components: change version, move up/down, remove, customize, edit, revert, install loader, add to/replace Minecraft.jar, add agents, add empty, import components, open .minecraft/libraries, reload, download all) | **Settings** tab → Advanced → Versions & components | ✅ |
| Modpack (managed pack: update/change version) | **Settings** tab → Advanced → Modpack (shown for modpack instances only) | ☑️ |
| Mods (add file, remove, enable/disable, view configs, view folder, download, check for updates, reset update metadata, verify dependencies, export list, change version, homepage, lock/unlock updates) | **Content** tab: a Wraith list with enable switches, search, remove, Add file, **Browse content**, Check for updates and Open folder. **Advanced** shows the full mod manager | ✅ |
| Core Mods, Nil Mods | Content tab type chips (shown when the instance uses them) | ☑️ |
| Resource Packs, Texture Packs, Shader Packs, Data Packs (global) | Content tab type chips | ✅ |
| Notes | **Settings** tab → Advanced → Notes | ✅ |
| Worlds (add, join, rename, copy, delete, data packs, tools, reset icon, copy seed, refresh, view folder) | **Worlds** tab: world cards with Play (quick-join), Open folder and Copy seed. **Advanced tools** shows the full world page | ✅ |
| Servers (multiplayer list: add, remove, reorder, join, refresh, name/address/resources) | **Servers** tab | ✅ |
| Friend Server (Wraith) | **Friend Server** tab | ✅ |
| Screenshots (upload, copy image/files, delete, rename, view folder) | **Screenshots** tab | ✅ |
| Settings (per-instance overrides: Java install/args/memory, window size, console, game time, account override, auto-join, mod loader override, legacy tweaks, native libraries, performance, custom commands, environment variables) | **Settings** tab → Essentials (name, group, memory slider with an override switch, Java) and Advanced → Game & Java (every override) | ✅ |
| Help button | The original pages keep their help links | ☑️ |
| Launch / Restart / Kill / Close buttons | Header Play/Stop; Restart is in the ▾ menu while the game runs | ☑️ |
| Console window opened on launch (ShowConsole / ShowConsoleOnError) | The instance page opens on its Logs tab. While the launcher is hidden ("hide the launcher while playing"), the classic console window is still used, so the console stays visible | ☑️ |

## 3. Dialogs

| Prism dialog | Wraith | Status |
|---|---|---|
| New Instance → Custom (name, group, icon, version list, loader + loader version) | **New instance** sheet: name, Minecraft version (newest first, snapshots optional), loader (None, Fabric, Quilt, Forge, NeoForge, with the recommended version picked automatically) and group. **More options** opens the full dialog | ✅ |
| New Instance → Import (zip/URL), FTB App import | The sheet's **Import file** button, Library/Home **Import**, and drag and drop; FTB App import is under More options | ☑️ |
| New Instance → modpack pages (Modrinth, CurseForge, FTB, ATLauncher, Technic, legacy FTB) | **Browse**. Modrinth installs directly. The other sources open their own page in the full dialog with the search filled in. **Per-source browsers** opens them all | ✅ |
| Copy Instance | ⋯ → Duplicate (original dialog: copy saves, keep playtime, links, ...) | 🟡 |
| Export Instance / Export Pack (mrpack, CurseForge) / Export to mod list | ⋯ → Export (original dialogs) | 🟡 |
| Create Shortcut | ⋯ menu (original dialog) | 🟡 |
| Icon Picker | Instance header icon (original picker) | 🟡 |
| Resource Download dialog (mods, packs, shaders from Modrinth/CurseForge) | Content tab → Browse content (original dialog) | 🟡 |
| Install Loader, Install Java, Java test | Versions & components; Settings → Java & memory → Advanced (original dialogs) | 🟡 |
| MSA login, Offline name, Profile setup, Skins | Settings → Accounts (original dialogs) | 🟡 |
| Progress / network failed / blocked mods / untrusted mods / review messages | Unchanged: they report task state and need a decision | 🟡 |
| Delete confirmation | Kept, because deletion is destructive; success is a toast with Undo | ☑️ |
| Rename / change group prompts | Small input dialogs; the Settings tab also has inline Name and Group fields | 🟡 |
| About | Settings → About on the page, with "Credits and licenses..." opening the original dialog | ✅ |
| Setup wizard (language, Java, paste service, theme, login) | Unchanged first-run wizard, in the Wraith theme | 🟡 |
| Update available | Unchanged (the updater is disabled in this fork) | 🟡 |

## 4. Global settings → Settings page

Settings is a page with a category list. Each category starts with the common options as simple controls, and the
full original page is under **Advanced** in that category. Accounts and Network show their original pages directly.
The original pages are created fresh whenever Settings opens and saved when you leave it, like Prism's dialog does on OK.

| Prism page and options | Wraith | Status |
|---|---|---|
| Launcher → User interface: instance sorting, renaming behaviour, menubar instead of toolbar, edit on double-click | **General**: Sort instances by. The rest is under General → Advanced → Launcher | ✅ |
| Launcher → Updater (interval, auto-check) | General → Advanced → Launcher | ✅ |
| Launcher → Folders (instances, mods, icons, Java, skins, downloads) and additional instance folders | General → Advanced → Launcher | ✅ |
| Launcher → Mods and modpacks (blocked mods search/move, metadata, dependencies, incompatibilities, modpack update prompt, update channel) | General → Advanced → Launcher | ✅ |
| Launcher → Instance creation (download game files) | **General**: Download game files when creating an instance; also under Advanced | ✅ |
| Launcher → Console (history limit, stop logging on overflow) | General → Advanced → Launcher | ✅ |
| Launcher → Tasks (retries, concurrent downloads/tasks, timeout) | General → Advanced → Launcher | ✅ |
| Language | General → Advanced → Language | ✅ |
| Appearance: theme, icons, cat pack, console font, cat opacity/scaling, preview, open/reload theme folders | **Appearance**: dialog theme and background cat; everything else under Advanced | ✅ |
| Minecraft (global game settings: window, console, global data packs, game time, account override, auto-join, loader override, latest version, legacy tweaks, native libraries, performance, custom commands, environment variables) | **Game**: show the log on start/crash, start maximized, track playtime. **General** has hide the launcher while playing and quit when the game closes. Everything else is under Game → Advanced | ✅ |
| Java: installation (auto-detect, auto-download, skip checks, executable, detect/browse/test/downloader), memory (min/max/PermGen, low-memory warning), Java arguments, managed installations (download/remove/refresh) | **Java & memory**: memory slider and automatic Java downloads; everything else under Advanced | ✅ |
| Accounts (add Microsoft/offline, set default, no default, skins, refresh, remove, reorder) | **Accounts** (the original page, shown directly) | ✅ |
| APIs (paste service, metadata server + refresh on launch, assets server, legacy FML libs, user agent, MSA client ID, Modrinth/CurseForge/Technic keys, Modrinth fallback for blocked mods) | **Network** → APIs & services | ✅ |
| Proxy (system/none/SOCKS5/HTTP, address, port, auth) | **Network** → Proxy | ✅ |
| External tools (text editor, JProfiler, VisualVM, world tools) | **Advanced** → Advanced → External tools | ✅ |
| (new) Classic Prism layout toggle, folders, metadata cache, launcher logs | **Advanced** | ✅ |

## 5. Wraith additions (earlier rounds)

| Feature | Wraith | Status |
|---|---|---|
| Unified modpack browser | **Browse**, with source filters, sorting, a details panel and a verified direct Modrinth install | ✅ |
| Friend server dashboard and list | **Servers** page (cards, Start/Stop, Manage, a New server picker), the instance **Friend Server** tab, and toasts when a server comes online or stops | ✅ |
| Quilt friend servers | The same (set up in round 2) | ✅ |
