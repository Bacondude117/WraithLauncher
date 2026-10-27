# Wraith Launcher: feature map and parity audit

This file lists every screen, dialog, action, setting, shortcut and command-line option in Prism Launcher, and where
it lives in the Wraith interface. Nothing is removed. Anything without a simple Wraith control is one click away under
an **Advanced** section, which hosts the original page inside the new shell.

Status (round 4 parity audit, 2026-10-27):
- **Verified:** exercised in the running app (scripted UI run with screenshots, or a real install/action).
- **Moved:** reachable in the new UI and wired to Prism's original code, but not exercised here, usually because it
  needs a game launch or a Microsoft account (there is no account on the test machine, and signing in is the user's
  to do).
- **Missing / Broken:** found during the audit. Every item found was fixed; the "Fix" column says what changed.
  Nothing is Missing or Broken at the end of the audit.

How it was checked: the `WRAITH_UI_SCRIPT` runner drives the real UI. It can open dialogs through the same calls as
the buttons, act on modal dialogs (`modal-dump`, `modal-type`, `modal-accept`, `modal-reject`, `modal-shot`), press the
newest toast's button, and run whatever a key sequence is bound to (`key Ctrl+G`). Logs and screenshots are in
`/screenshots` (22–27) and the session scratchpad.

## 1. Main window

| Prism | Wraith | Status |
|---|---|---|
| Instance grid (groups, icons, names) | **Library**: card grid with cover art, loader and version badges, search, loader chips, group filter, sort | Verified |
| Keyboard use of the grid (arrows, Enter, Delete, F2) | Library grid: arrows move a focus ring, Enter opens, Ctrl+Enter plays, Delete/Backspace deletes (with confirmation), F2 renames, Ctrl+K/G/D/I, Menu key | Verified (real key presses: Right then Enter opened the second instance). *Fix: was Missing (the cards had no keyboard support).* |
| Double-click to launch (or edit, per setting) | Card Play button (hover); clicking a card opens the instance page; the setting applies to the classic layout | Verified |
| Instance context menu | Right-click a card or its ⋯ button: Play/Stop, Open, Open folder, Rename, Change icon, Change group, Duplicate, Export, Create shortcut, Delete | Verified (each dialog opened from the shell, see §3) |
| Group context menu (new instance in group, rename group, delete group) | Library group filter → ⋯ | Moved |
| Collapsible groups | Group filter dropdown; group shown on the instance page | Verified |
| Drag and drop zips/URLs to import | Drop onto Home or Library (Prism's import flow) | Moved |
| Cat background (opacity, fit) | Behind Home and Library when enabled (Settings → Appearance) | Moved |
| News toolbar | No feed is configured in this fork; the classic layout keeps the bar for builds that set one | Verified |
| Status bar (instance description, total playtime) | Instance header (playtime, last played); Home (total playtime) | Verified |

### Menu bar and main toolbar

| Prism action | Wraith | Status |
|---|---|---|
| Add Instance | **New instance** sheet (Home, Library, Ctrl+N) | Verified |
| Folders menu (data, logs, instances, icons, mods, Java, skins, themes, icon themes, cat packs) | Sidebar More → Folders; Settings → Advanced → Folders | Moved |
| Settings | Sidebar **Settings**, Ctrl+, | Verified. *Fix: Ctrl+, was Broken on Windows/Linux (Qt's Preferences key is empty there, as in Prism); it is now bound explicitly.* |
| Help menu (clear metadata cache, report a bug, view logs, wiki, about, check for updates) | Sidebar More → Help; Settings → Advanced (cache, logs); Settings → About | Moved |
| Update | Hidden while the updater is disabled (as in Prism) | Moved |
| Meow (cat) | Settings → Appearance; More menu | Moved |
| Accounts menu (switch, no default, manage accounts, skins) | Sidebar account chip (same menu); Settings → Accounts | Moved |
| Undo last instance deletion | **Undo** on the deletion toast; More → Undo; Ctrl+Z | Verified (delete → toast → Undo restored the instance). *Fix: was Broken: the toast never appeared because the list still held the trashed instance for a moment; Ctrl+Z was Missing.* |
| View menu (themes, status bar, lock toolbars, toolbar toggles) | Themes: Settings → Appearance. Toolbars and status bar only exist in the classic layout | Verified |
| Close window, Quit | Ctrl+W, Ctrl+Q; More → Quit | Moved |
| Menu bar instead of toolbar (setting) | Classic layout only; Settings → General → Advanced → Launcher | Verified |
| Konami code | Classic layout | Moved |

### Instance toolbar

| Prism action | Wraith | Status |
|---|---|---|
| Change icon | Instance header icon; ⋯ menu | Verified (icon picker opens from the shell) |
| Rename | Header name; ⋯ menu; Settings tab → Name; F2 | Verified (renamed a copy) |
| Launch (+ offline, demo, profilers) | **Play**; ▾ launch menu; Ctrl+Enter | Moved (needs an account; without one, Prism's "No Accounts" prompt appears, verified) |
| Kill | Play becomes **Stop**; Ctrl+K | Moved |
| Edit | Opens the instance page; Ctrl+I opens its Settings tab | Verified |
| Change group | ⋯ menu; Settings tab; Ctrl+G | Verified (group list saved) |
| Folder | Header folder button; Settings tab | Moved |
| Export (Prism zip, Modrinth mrpack, CurseForge zip) | ⋯ → Export; Settings tab | Verified (both export dialogs open) |
| Copy | ⋯ → Duplicate; Ctrl+D | Verified (copy created) |
| Delete | ⋯ → Delete; Settings tab; Delete key in the library | Verified (confirmation, trash, Undo) |
| Create shortcut | ⋯ menu; Settings tab | Verified (dialog opens) |

## 2. Instance window → instance page

| Prism page | Wraith | Status |
|---|---|---|
| Minecraft Log (console with its tools) | **Logs** tab → Game log | Verified |
| Other Logs | **Logs** tab → Older logs | Verified |
| Version (components, loaders, jar mods, agents, libraries) | **Settings** tab → Advanced → Versions & components | Verified |
| Modpack (managed pack update) | **Settings** tab → Advanced → Modpack | Moved |
| Mods (all actions) | **Content** tab (switches, search, remove, add, Browse content, updates, folder); **Advanced** = full mod manager | Verified |
| Core Mods, Nil Mods | Content type chips (when used) | Moved |
| Resource/Texture/Shader/Data packs | Content type chips | Verified |
| Notes | Settings tab → Advanced → Notes | Verified |
| Worlds (all actions) | **Worlds** tab (Play, folder, copy seed); Advanced tools = full page | Verified |
| Servers (multiplayer list) | **Servers** tab | Verified |
| Screenshots | **Screenshots** tab | Verified |
| Instance settings (Java, memory, window, console, playtime, account, auto-join, loader overrides, legacy, native libraries, performance, commands, environment) | Settings tab → Essentials, and Advanced → Game & Java (every override) | Verified |
| Help button | Original pages keep their help links | Moved |
| Launch / Restart / Kill / Close | Header Play/Stop; Restart in ▾ while running | Moved |
| Console on launch/crash | Instance page opens on Logs; classic console window while the launcher is hidden | Moved |

## 3. Dialogs

| Prism dialog | Wraith | Status |
|---|---|---|
| New Instance → Custom | **New instance** sheet; More options = full dialog | Verified |
| New Instance → Import, FTB App import | Sheet → Import file; Library/Home Import; drag and drop; `--import` | Verified (`--import` opens the import page) |
| New Instance → modpack pages | **Browse**: every source has a version list and a direct Install; "Open in the <source> page" for extras | Verified (Technic install end to end; FTB and ATLauncher version lists; Modrinth install in round 3) |
| Copy Instance | Duplicate | Verified |
| Export Instance / Export Pack / mod list | Export | Verified (dialogs open) |
| Create Shortcut | ⋯ menu | Verified (dialog opens) |
| Icon Picker | Header icon | Verified (dialog opens) |
| Resource Download (mods, packs, shaders) | Content → Browse content | Moved |
| Install Loader, Install Java, Java test | Versions & components; Settings → Java & memory → Advanced | Moved |
| MSA login, offline name, profile setup, skins | Settings → Accounts | Moved |
| Progress / network failed / blocked mods / untrusted mods | Unchanged task dialogs | Moved |
| Delete confirmation | Kept; then a toast with Undo | Verified |
| Rename / change group prompts | Small input dialogs; inline fields on the Settings tab | Verified |
| No accounts prompt (on launch) | Unchanged | Verified |
| About | Settings → About; Credits opens the original dialog | Verified |
| Setup wizard | Unchanged first-run wizard | Moved |
| Update available | Updater disabled in this fork | Moved |

## 4. Global settings → Settings page

| Prism page | Wraith | Status |
|---|---|---|
| Launcher (UI, updater, folders, mods/modpacks, instance creation, console, tasks) | **General** (common options) + General → Advanced → Launcher | Verified |
| Language | General → Advanced → Language | Verified |
| Appearance (themes, icons, cat packs, console font, preview) | **Appearance** + Advanced | Verified |
| Minecraft (all global game options) | **Game** + Advanced | Verified |
| Java (installations, memory, arguments, managed Java) | **Java & memory** + Advanced | Verified |
| Accounts | **Accounts** (original page) | Verified |
| APIs, Proxy | **Network** (original pages) | Verified |
| External tools | **Advanced** → Advanced | Verified |
| (Wraith) privacy and automatic behaviour switches | **Privacy & security** | Verified |
| (Wraith) classic layout, folders, cache, logs | **Advanced** | Verified |

## 5. Keyboard shortcuts

| Shortcut (Prism) | Wraith | Status |
|---|---|---|
| Ctrl+N new instance | Window-wide | Verified |
| Ctrl+, settings | Window-wide | Verified (fixed, see §1) |
| Ctrl+Z undo deletion | Window-wide (enabled after a deletion) | Verified. *Fix: was Missing (lived in the hidden menu bar).* |
| Delete / Backspace delete instance | Library grid | Moved. *Fix: was Missing.* |
| Ctrl+K kill, Ctrl+G group, Ctrl+I edit, Ctrl+D copy | Instance page and library grid | Verified on the instance page. *Fix: were Missing (lived in the removed instance toolbar).* |
| F2 rename | Instance page and library grid | Verified |
| Ctrl+1..9 switch account, Ctrl+0 no default account | Window-wide | Verified (Ctrl+0). *Fix: were Missing (lived in the hidden accounts menu).* |
| Ctrl+W close window, Ctrl+Q quit | Window-wide | Moved |
| (Wraith) Ctrl+F library search, Esc back, Ctrl+Enter play | Library / instance page | Verified |

## 6. Command-line options

| Option | Wraith | Status |
|---|---|---|
| `-d, --dir` | Unchanged | Verified (every test run uses it) |
| `-l, --launch`, `-s, --server`, `-w, --world`, `-a, --profile`, `-o, --offline` | Unchanged (launch flow) | Moved (needs an account) |
| `--alive` | Unchanged | Verified (`live.check` written) |
| `-I, --import` | Opens the import page | Verified |
| `--show <id>` | Opens the main window on the instance's page | Verified. *Fix: was Broken: it opened Prism's old instance window without the main window, because the shell only took over when the main window was already visible.* |

## 7. Wraith additions

| Feature | Wraith | Status |
|---|---|---|
| Unified modpack browser | **Browse** (source filters, neutral ranking, per-source install) | Verified |
| Friend server dashboard and list | **Servers** page, Friend Server tab, toasts | Verified (setup; running a server needs the user's EULA consent) |
| Quilt and Fabric friend servers (official installers, verified downloads) | Same | Moved (Quilt verified in round 2; the Fabric installer path is new) |
| Privacy & security settings | Settings → Privacy & security | Verified |

## 8. Round 4 features

| Feature | Where | Status |
|---|---|---|
| **A. Snapshots**: automatic snapshot before mod, modpack, Minecraft or loader changes (switch, at most one per 5 minutes) | Instance → **Snapshots**; Settings → General | Verified (toggling a mod took one, with a "View" toast) |
| Manual snapshots with a name, optionally with worlds | Snapshots → Take snapshot | Verified |
| Ring of N snapshots (default 10, or keep all); pin to keep forever; rename | Snapshots tab and Settings → General | Verified (ring, pinning: unit tests; rename/pin buttons: Moved) |
| Deduplicated storage (each file stored once, SHA-1) | `<instance>/.wraith/snapshots` (plain files + JSON) | Verified (unit tests) |
| Worlds left out unless asked; logs, caches, screenshots never | Same | Verified (unit tests) |
| Restore with a preview of what changes; current state saved first; playtime/settings never rolled back | Snapshot → Restore... | Verified (restore preview and restore in the app; unit tests) |
| Compare two snapshots | Snapshot → Compare with... | Verified |
| Export a snapshot as a folder or zip | Snapshot → Export | Moved (folder export unit-tested; the file dialogs weren't driven) |
| Instance export and copy leave the snapshot store out | Prism's export/copy dialogs | Moved |
| **B. Changelog generator** between snapshots (or a snapshot and now), Markdown | Instance → **Creator** → Changelog; Snapshots panel | Verified |
| Dependency/conflict checker (missing/disabled dependencies, declared incompatibilities, duplicates, wrong loader, Minecraft range; reads bundled mods) | Creator → Health check | Verified (real 16-mod NeoForge pack: no false alarms; unit tests for every rule) |
| Version pinning (Prism's update lock) | Content tab pin icon on mods with download metadata; Creator → Version pins | Moved |
| Bulk update review | Content → Check for updates opens Prism's review dialog (select which updates, changelogs); an automatic snapshot is taken when it is accepted | Moved |
| Export to .mrpack and CurseForge with a pre-export checklist (mod problems, mods without download info and licences, disabled mods, personal files, large files, release snapshot) | Creator → Export | Verified (checklist); the export dialog itself is Prism's (Verified in §3) |
| Config diff between snapshots, line by line | Creator → Config diff | Verified |
| Test launch (snapshot, play, then a summary of what changed with a way to roll back) | Creator → Test launch | Moved (needs an account to launch) |
