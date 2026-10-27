# Wraith Launcher: feature map

Every screen, dialog, action and option in the Prism-derived UI, and where it lives in the new Wraith interface.
Status: ✅ done and verified in the running app, 🟡 reachable but not yet redesigned (it opens the original dialog, restyled),
⬜ not done yet. Nothing is removed. Anything without a simple Wraith control is one click away under an
**Advanced** section, which hosts the original page inside the new shell.

Navigation: the **sidebar** has Home, Library, Browse, Servers and Settings, with the account chip and the
"More" menu at the bottom. The **instance page** opens from any instance card.

## 1. Main window

| Prism | Wraith | Status |
|---|---|---|
| Instance grid (groups, icons, names) | **Library**: card grid with cover art, loader and version badges, search, loader filter, group filter, sort | ⬜ |
| Double-click an instance to launch (or edit, per setting) | Card **Play** button on hover; clicking the card opens the instance page; the "Open Edit window on double-click" setting is honoured on double-click | ⬜ |
| Instance context menu (all instance actions, rename, change icon) | Right-click a card or use the card's ⋯ button: the same actions | ⬜ |
| Group context menu (create instance in group, rename group, delete group) | Library group filter ⋯ menu: Rename group, Delete group; "New instance" pre-fills the selected group | ⬜ |
| Drag and drop zips/URLs onto the window to import | Drop anywhere on the Library or Home page | ⬜ |
| Collapsible groups | Group filter dropdown + group label on each card | ⬜ |
| Cat background (and its opacity/fit settings) | Drawn behind the Library and Home pages when enabled (Settings → Appearance) | ⬜ |
| News toolbar / More News | Home "News" card when a feed is configured (no feed is configured in this fork) | ⬜ |
| Status bar: selected instance description, total playtime | Instance page header (playtime, last played); Home shows total playtime | ⬜ |

### Main toolbar and menu bar

| Prism action | Wraith | Status |
|---|---|---|
| Add Instance (Ctrl+N) | "New instance" button (Home, Library, sidebar); Ctrl+N | ⬜ |
| Folders menu: Launcher Data, Launcher Logs, Instances, Instance Icons, Mods (Quick Access), Auto Downloaded Java, Skins, Widget Themes, Icon Themes, Cat Packs | Sidebar **More → Folders** submenu (same entries) | ⬜ |
| Settings (Ctrl+,) | Sidebar **Settings**; Ctrl+, | ⬜ |
| Help menu: Clear Metadata Cache, Report a Bug, View logs, Install to PATH (macOS), Matrix/Discord/Reddit, More News, Check for updates, Wiki, About | Sidebar **More → Help** submenu (same entries) and Settings → About | ⬜ |
| Update... | Sidebar More menu (hidden when the updater is disabled, as before) | ⬜ |
| Meow (cat toggle) | Settings → Appearance → Enable cat; also More menu | ⬜ |
| Accounts menu: switch account, No Default, Manage Accounts, Manage Skins | Sidebar **account chip** opens the same menu; Settings → Accounts | ⬜ |
| Edit menu: Undo Last Instance Deletion (Ctrl+Z) | "Undo" button on the toast shown after a delete; More menu | ⬜ |
| View menu: Themes, Status Bar, Lock Toolbars, toolbar toggles | Themes: Settings → Appearance. Status bar/toolbar toggles only apply to the classic layout (Settings → Advanced → Classic layout) | ⬜ |
| Close Window (Ctrl+W), Quit (Ctrl+Q) | Same shortcuts | ⬜ |
| Replace toolbar with menubar (setting) | Applies to the classic layout; listed under Settings → Launcher (advanced) | ⬜ |
| Konami code | Still works on the classic layout | ⬜ |

### Instance toolbar (right column)

| Prism action | Wraith | Status |
|---|---|---|
| Change icon (click the big icon) | Instance page header: click the icon; card ⋯ menu | ⬜ |
| Rename (editable label) | Instance page header: click the name; card ⋯ menu | ⬜ |
| Launch (+ launch menu: offline, demo, profilers) | Big **Play** button (card, Home, instance header); the ▾ next to Play has the launch menu | ⬜ |
| Kill (Ctrl+K) | Play turns into **Stop** while running | ⬜ |
| Edit... (Ctrl+I) | Opens the instance page | ⬜ |
| Change Group... (Ctrl+G) | Instance ⋯ menu; Settings tab → Essentials → Group | ⬜ |
| Folder | Instance header "Open folder" button | ⬜ |
| Export (Prism zip / Modrinth mrpack / CurseForge zip) | Instance ⋯ menu → Export submenu | ⬜ |
| Copy... (Ctrl+D) | Instance ⋯ menu → Duplicate | ⬜ |
| Delete | Instance ⋯ menu → Delete (toast with Undo afterwards) | ⬜ |
| Create Shortcut | Instance ⋯ menu | ⬜ |

## 2. Instance window (Edit Instance) → instance page

The instance page is a full page in the shell: header (cover art, icon, name, loader, version, playtime, Play,
Open folder, ⋯) and tabs.

| Prism page | Wraith | Status |
|---|---|---|
| Minecraft Log (console: keep updating, wrap, color, copy, upload, clear, find, bottom) | **Logs** tab → Game log | ⬜ |
| Other Logs | **Logs** tab → Older logs | ⬜ |
| Version (components: change version, move up/down, remove, customize, edit, revert, install loader, add to/replace Minecraft.jar, add agents, add empty, import components, open .minecraft/libraries, reload, download all) | **Settings** tab → Advanced → Versions & components | ⬜ |
| Modpack (managed pack: update/change version) | **Settings** tab → Advanced → Modpack (only for modpack instances) | ⬜ |
| Mods (add file, remove, enable/disable, view configs, view folder, download, check for updates, reset update metadata, verify dependencies, export list, change version, homepage, lock/unlock updates) | **Content** tab → Mods: Wraith list with enable toggle, search, Remove, Open folder, **Browse content**, **Check for updates**; everything else under the tab's **Advanced** view (the full mod manager) | ⬜ |
| Core Mods, Nil Mods | Content tab → type selector (shown only when the instance uses them) | ⬜ |
| Resource Packs, Texture Packs, Shader Packs, Data Packs (global) | Content tab → type selector | ⬜ |
| Notes | **Settings** tab → Advanced → Notes | ⬜ |
| Worlds (add, join, rename, copy, delete, data packs, tools, reset icon, copy seed, refresh, view folder) | **Worlds** tab: Wraith world list with Play, Open folder; full world tools under Advanced | ⬜ |
| Servers (multiplayer list: add, remove, reorder, join, refresh, name/address/resources) | **Servers** tab | ⬜ |
| Friend Server (Wraith) | **Friend Server** tab | ⬜ |
| Screenshots (upload, copy image/files, delete, rename, view folder) | **Screenshots** tab | ⬜ |
| Settings (per-instance overrides: Java install/args/memory, window size, console, game time, account override, auto-join, mod loader override, legacy tweaks, native libraries, performance, custom commands, environment variables) | **Settings** tab → Essentials (memory, Java, window) + Advanced → Game & Java (every override) | ⬜ |
| Help button | Settings tab → Advanced pages keep their own Help links | ⬜ |
| Launch / Restart / Kill / Close buttons | Instance header Play/Stop; Restart in the Play ▾ menu while running | ⬜ |
| Console window opened on launch (ShowConsole / ShowConsoleOnError) | Opens the instance page's Logs tab; if the launcher is hidden ("hide launcher when game opens"), the classic console window is used so the console still appears | ⬜ |

## 3. Dialogs

| Prism dialog | Wraith | Status |
|---|---|---|
| New Instance → Custom (name, group, icon, version list, loader + loader version) | **New instance** sheet: name, Minecraft version, loader (Vanilla, Fabric, Quilt, Forge, NeoForge), group; "Advanced options" opens the full dialog | ⬜ |
| New Instance → Import (zip/URL), FTB App import | New instance sheet → "Import from file"; Advanced options | ⬜ |
| New Instance → modpack pages (Modrinth, CurseForge, FTB, ATLauncher, Technic, legacy FTB) | **Browse** (unified search). Modrinth installs natively; other sources open their own page in the advanced dialog, pre-filled | ⬜ |
| Copy Instance | Instance ⋯ → Duplicate (original dialog: copy saves, keep playtime, links, etc.) | 🟡 |
| Export Instance / Export Pack (mrpack, CurseForge) / Export to mod list | Instance ⋯ → Export (original dialogs) | 🟡 |
| Create Shortcut | Instance ⋯ (original dialog) | 🟡 |
| Icon Picker | Instance header icon (original picker) | 🟡 |
| Resource Download dialog (mods, packs, shaders from Modrinth/CurseForge) | Content tab → Browse content (original dialog) | 🟡 |
| Install Loader, Install Java, Java test | Versions & components; Settings → Java (original dialogs) | 🟡 |
| MSA login, Offline name, Profile setup, Skins | Settings → Accounts (original dialogs) | 🟡 |
| Progress / network failed / blocked mods / untrusted mods / review messages | Unchanged (they report task state and need a decision) | 🟡 |
| Delete confirmation | Kept (destructive); success is a toast with Undo | ⬜ |
| About | Settings → About (in-page), plus the original dialog for licenses/credits | ⬜ |
| Setup wizard (language, Java, paste service, theme, login) | Unchanged first-run wizard, restyled | 🟡 |
| Update available | Unchanged (updater disabled in this fork) | 🟡 |

## 4. Global settings → Settings page

Settings is a page with a category list. Each category starts with the common options as simple controls; the
full original page sits under **Advanced** in the same category.

| Prism page and options | Wraith category | Status |
|---|---|---|
| Launcher → User interface: instance sorting, renaming behaviour, menubar instead of toolbar, edit on double-click | **General** (sorting, double-click) + Advanced | ⬜ |
| Launcher → Updater (interval, auto-check) | General → Advanced | ⬜ |
| Launcher → Folders (instances, mods, icons, Java, skins, downloads) and additional instance folders | **Storage** → Advanced | ⬜ |
| Launcher → Mods and modpacks (blocked mods search/move, metadata, dependencies, incompatibilities, modpack update prompt, update channel) | **Content** → Advanced | ⬜ |
| Launcher → Instance creation (download game files) | General → Advanced | ⬜ |
| Launcher → Console (history limit, stop logging on overflow) | General → Advanced | ⬜ |
| Launcher → Tasks (retries, concurrent downloads/tasks, timeout) | **Network** → Advanced | ⬜ |
| Language | **General** → Language | ⬜ |
| Appearance: theme, icons, cat pack, console font, cat opacity/scaling, preview, open/reload theme folders | **Appearance** (theme + cat) + Advanced | ⬜ |
| Minecraft (global game settings: window, console, global data packs, game time, account override, auto-join, loader override, latest version, legacy tweaks, native libraries, performance, custom commands, environment variables) | **Game** (window size, maximized, hide launcher, show console) + Advanced | ⬜ |
| Java: installation (auto-detect, auto-download, skip checks, executable, detect/browse/test/downloader), memory (min/max/PermGen, low-memory warning), Java arguments, managed installations (download/remove/refresh) | **Java & memory** (memory slider, auto-download) + Advanced | ⬜ |
| Accounts (add Microsoft/offline, set default, no default, skins, refresh, remove, reorder) | **Accounts** (account list) + Advanced | ⬜ |
| APIs (paste service, metadata server + refresh on launch, assets server, legacy FML libs, user agent, MSA client ID, Modrinth/CurseForge/Technic keys, Modrinth fallback for blocked mods) | **Network** → Advanced: APIs & services | ⬜ |
| Proxy (system/none/SOCKS5/HTTP, address, port, auth) | **Network** → Advanced: Proxy | ⬜ |
| External tools (text editor, JProfiler, VisualVM, world tools) | **Advanced** category | ⬜ |
| (new) Classic Prism layout toggle | **Advanced** category | ⬜ |

## 5. Wraith additions (earlier rounds)

| Feature | Wraith | Status |
|---|---|---|
| Unified modpack browser | **Browse** | ⬜ |
| Friend server dashboard and list | **Servers** page + instance **Friend Server** tab | ⬜ |
| Quilt friend servers | Same | ⬜ |
