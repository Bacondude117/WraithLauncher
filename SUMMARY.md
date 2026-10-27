# Wraith Launcher — Summary

Branch `wraith-dev`, based on Prism Launcher `develop` @ `390a240ab`. Nothing was pushed and the remotes are
unchanged (`git log 390a240ab..wraith-dev` lists every Wraith commit).

Status: **branding, all three goals and the round-3 interface redesign are implemented and build cleanly.** The Debug build passes with
warnings-as-errors, and 24/24 tests pass. The 25th test, `FileSystem`, is excluded (see "Risks"). Every feature
was checked in the running app, with the exceptions listed at the end.

## What changed

### Branding
- **Name and IDs** (`program_info/CMakeLists.txt`, `Launcher_APP_BINARY_NAME`):
  - The app is "Wraith Launcher" and the exe is `wraithlauncher.exe`.
  - AppID is `io.github.wraithlauncher.WraithLauncher` and the env var is `WRAITHLAUNCHER_DATA_DIR`.
  - The data folder is `%APPDATA%\WraithLauncher` with `wraithlauncher.cfg`, fully separate from Prism.
  - Installer, `.desktop`, metainfo, man page and macOS plist are renamed too. None of them registers
    `prismlauncher://` any more, so a real Prism install keeps that URL handler.
- **Keys and services** (see SETUP_KEYS.md):
  - The MSA client ID, CurseForge key and Imgur ID are empty placeholders.
  - The updater, Sparkle feed, news, Discord, Matrix, Reddit and bug-tracker URLs are cleared.
  - The Prism metadata server and help wiki are kept.
  - The news toolbar hides itself when no feed is configured.
- **Icon:** an original ghost: `program_info/io.github.wraithlauncher.WraithLauncher.svg`.
  `wraith/tools/gen_icons.py` regenerates the `.ico`, `.png` and `.icns` without external tools. The ghost is
  also the in-app logo.
- **Wraith theme** (`launcher/ui/themes/WraithTheme.*`):
  - Charcoal surfaces with pale-cyan highlights and violet links/accents, plus a small stylesheet for rounded
    controls.
  - It is the first-run default, paired with the `flat_white` icons.
  - All Prism themes are still listed.
- **README and About dialog:**
  - The README has an "About this fork" section: it is a fork, not affiliated with or endorsed by Prism,
    GPL-3.0, with credits.
  - The About dialog says the same, and its credits label Prism's developers by name.
  - All upstream license headers are untouched.

### Goal 1: Unified modpack browser
- **`launcher/modplatform/unified/UnifiedPack.*`:** pure logic, with 11 unit tests:
  - name normalisation
  - duplicate detection across sources: the same name with compatible or unknown authors, or the same base
    name once a subtitle is dropped, as long as the authors match
  - ranking: relevance tiers first, then downloads; exact matches always come first
- **`UnifiedSearch.*`:**
  - Searches Modrinth, CurseForge (only when a key is set), FTB, ATLauncher (index cached and filtered
    locally) and Technic in parallel.
  - Results are re-merged as each source answers, and a failing source never blocks the others.
- **"Browse Modpacks" page** (`launcher/ui/pages/modplatform/unified/`):
  - Card list via the new `ui/widgets/PackCardDelegate`: icon, title, author, summary, source chips and
    downloads.
  - Details pane with links to each source, and "Install from <source>" buttons.
  - A status line shows which sources are pending or failed, and the CurseForge-key notice.
- **Install handoff:** "Install from X" opens X's existing page with the search pre-filled. That page then
  handles version choice and the actual install.
- **Advanced:** a new "Advanced" page sits above the per-source pages, explains them and links to each. All of
  them still exist, unchanged.

### Goal 2: UI refresh
- Card-style modpack results (above).
- **Instance grid** (`InstanceDelegate`): hovered and selected instances get rounded tinted cards with more
  padding, and icons are no longer tinted when selected.
- **Welcome overlay** (`InstanceView`): a quiet bordered card instead of an inverted block.
- These two paint paths only change under the Wraith theme, so other themes render exactly as in Prism. No
  option or setting was removed.

### Goal 3: Friend server dashboard
- **`launcher/server/ServerModFilter.*`** (7 test functions): decides which mods go on the server, in this
  order of priority:
  1. the user's own override
  2. the packwiz `.index` `side` field, which holds Modrinth's `client_side`/`server_side`
  3. a short list of well-known client-only mods
  4. otherwise the mod is included
- **`launcher/server/FriendServer.*`:**
  - Builds `<instance>/wraith-server/` from copies; the instance itself is never modified.
  - Server software per loader:
    - vanilla: `server.jar` from Mojang's manifest
    - Fabric: the official server launcher
    - Forge/NeoForge: the official installer run with `--installServer`
  - Copies the included mods plus `config`, `defaultconfigs`, `kubejs`, `scripts` and `global_packs`.
  - Writes `server.properties`, and writes `eula.txt` as true only after the user agrees.
  - Runs the server with the instance's Java and memory.
  - Stop sends `stop`, then kills the process after 30 s.
  - Keeps a 5000-line console history and accepts commands.
  - The server object is owned by the instance, so it keeps running when the window closes and stops with
    the launcher.
- **"Friend Server" instance page** (`ui/pages/instance/ServerDashboardPage.*`):
  - Status line plus Set Up/Rebuild, Start, Stop and Open Folder buttons.
  - Port and memory fields, and the EULA checkbox with a link to the EULA.
  - The LAN addresses friends can join at.
  - A mod list with checkboxes and the reason for each decision.
  - A live console with a command line.

### Tooling and docs
- `wraith/build-windows.cmd configure|build|test|run`.
- CLAUDE.md, PLAN.md, PROGRESS.md, DECISIONS.md, SETUP_KEYS.md and this file.

### Round 2: Quilt, Modrinth-style layout, runnable build
- **Quilt friend servers:** set up with the official Quilt installer. When an instance has no Java chosen, a
  matching installed Java is found automatically. Verified: a Quilt 1.20.1 setup produced
  `quilt-server-launch.jar`.
- **Modrinth-style layout** with the Wraith theme (`ui/MainWindowWraith.cpp`, `ui/instanceview/WraithTiles.*`,
  `ui/dialogs/FriendServersDialog.*`):
  - A left navigation rail: Library, Browse, Servers, then every original toolbar action, with Accounts at the
    bottom.
  - A "Library" header with an instance search and a "New instance" button.
  - Tile cards showing the loader and Minecraft version.
  - The Launch button is a big Play button.
  - A "Friend Servers" list with status, Start/Stop and Manage buttons.
  - Choosing any other theme restores Prism's classic layout immediately.
- **Runnable build:** `wraith\build-windows.cmd install` writes `install\wraithlauncher.exe` with the Qt DLLs
  next to it.

### Round 3: a new interface (Wraith shell)
Wraith no longer uses Prism's toolbar, grid and dialogs. FEATURES.md maps every old screen, action and option to
its new place, with a status for each. DESIGN.md describes the layout, tokens and technical approach. Screenshots of
every screen are in `/screenshots`.
- **Technology:**
  - The primary screens are Qt Quick (QML), hosted in `QQuickWidget`s inside the existing main window
    (`launcher/ui/wraith/`).
  - MainWindow keeps running as the controller, so launching, deleting with trash and undo, exporting, accounts
    and imports are Prism's own code.
  - Power-user pages are the original Prism pages, embedded inside the shell under "Advanced" and restyled.
- **Sidebar:** Home, Library, Browse and Servers; Settings and More (folders, help, about, quit); an account chip.
- **Home:** a "Continue playing" hero with Play, recent instance cards and quick actions.
- **Library:**
  - Cards with blurred cover art, loader and version badges, last played, and a hover Play/Stop button and ⋯ menu.
  - Search, loader chips, a group filter (with a group menu) and sorting.
  - Empty states and drag-and-drop import.
- **Instance page:**
  - A banner with icon, name, badges and playtime, Play with a launch menu, the folder and an ⋯ menu.
  - Tabs: Content, Worlds, Screenshots, Servers, Logs, Friend Server and Settings.
  - Content (enable switches, search, Browse content, update check) and Worlds (cards with quick-join Play) have
    simple lists; Advanced opens the full original page in place.
  - Settings has Essentials (name, group, memory slider, Java) and every original page under Advanced.
  - Esc goes back.
- **Browse:**
  - The unified search, with source filters (counts, progress, errors), relevance/downloads sorting, result cards and
    a details panel.
  - Modrinth packs install directly (version picker; verified end to end). Other sources open their own page with
    the search filled in.
- **Settings:** General, Appearance, Game, Java & memory, Accounts, Network, Advanced (classic layout switch, folders,
  cache, logs) and About, with simple controls first and the original pages under Advanced.
- **Servers:** friend-server cards with live status, Start/Stop and Manage, a New server picker, and toasts when a
  server goes online or stops.
- **New instance sheet:** name, version, loader with automatic recommended version, and group (verified by creating
  a Fabric 1.21.1 instance). Ctrl+N opens it, and "More options" opens Prism's full dialog.
- **Toasts** replace several popups: instance ready (Open), moved to trash (Undo), install started, server state,
  seed copied.
- **Classic layout:** Settings → Advanced → Classic layout brings Prism's layout back after a restart.
- **Upstream fix:** `InstanceView::calculateItemsPerRow()` could return 0 and divide by zero; it is now clamped to 1.
- **UI test script:** `WRAITH_UI_SCRIPT` drives the app for screenshots (navigate, select, trigger actions, create
  instances, capture). It does nothing unless that variable is set.

## How to test manually

First build and run:

```
wraith\build-windows.cmd configure
wraith\build-windows.cmd build
wraith\build-windows.cmd test
wraith\build-windows.cmd run
```

`run` uses the throwaway data dir `build\run-data`. Toolchain notes are in CLAUDE.md.

1. **Branding**
   - The window title is "Wraith Launcher …" and the title bar, taskbar and exe icon are the ghost.
   - Help → About shows the fork notice, and the Credits tab lists "Prism Launcher Developers".
   - On a fresh data dir, the setup wizard and main window use the Wraith theme.
   - Settings → Appearance still lists Dark, Bright, System and the rest, and switching works.
2. **Unified browser**
   - Open Add Instance → "Browse Modpacks". Popular packs from all sources load, and the status line says how
     many were found.
   - Search "skyfactory": "Sky Factory 4" appears once, with ATLauncher and Technic chips.
   - Search "tekkit": the exact "Tekkit" ranks first.
   - Select a card: the details pane shows source links and "Install from …" buttons. Click one: the source's
     page opens with the search filled in. Pick the pack and a version, then OK, and it installs as usual.
   - "Advanced" lists and links the original pages.
   - Add a CurseForge key (SETUP_KEYS.md), restart, search again: CurseForge results and chips appear.
3. **UI refresh**
   - With some instances, hovering or selecting one shows a rounded card.
   - With no instances, the Welcome card is dark with a cyan border.
   - Switch to Dark or Bright: Prism's original look comes back.
4. **Friend server**
   - Edit an instance → "Friend Server".
   - The mod list shows each mod with the reason. Client-only mods such as Sodium or Mod Menu are unticked.
   - Press **Set Up** and watch the console download and copy. `wraith-server/` appears under the instance.
   - Tick **I accept the Minecraft EULA**, then **Start**. The status goes Starting → Running and the console
     streams the server log.
   - Type `list` and press Enter to see the reply.
   - Join from the game at `localhost:<port>`, or from another PC on the LAN using the address shown.
   - **Stop** saves the world and the status returns to Stopped.
   - Untick a mod and press **Rebuild**: the console logs it as skipped.
   - Test with a Fabric instance and a Forge or NeoForge instance as well.

## Decisions

DECISIONS.md has all of them with reasoning (D1–D13). The main ones:
- **Separate data folder and no Prism keys:** these are a hard requirement for a fork.
- **Prism's meta server is kept:** the launcher needs that metadata to work.
- **The unified browser hands installs to the existing source pages:** each pack's version picker and install
  task is reused rather than duplicated.
- **"Advanced" heading page:** used instead of editing or hiding the upstream pages.
- **Friend servers are built from copies** in `<instance>/wraith-server/`, and the EULA is never auto-accepted.
- **New code lives in new files** (`modplatform/unified`, `server/`, `ui/pages/.../unified`, `WraithTheme`).
  Upstream files only get small, commented hook edits, which keeps rebases manageable.

## Unfinished and risky

- **Round 3 UI:**
  - Launching a game was not tested: there is no account here, and Microsoft login needs a client ID. This
    includes the Play/Stop state, the "N playing" badge and the console switching to the Logs tab.
  - Delete with Undo, export, copy, shortcut, rename, change icon and the folder/help menus call Prism's actions
    unchanged, but weren't clicked through in this session (see the ☑️ rows in FEATURES.md).
  - The dialogs marked 🟡 in FEATURES.md (copy, export, icon picker, resource download, loader/Java install,
    login, progress) are the original dialogs in the Wraith theme, not redesigned.
  - The shell is always dark. Choosing another theme in Settings → Appearance only changes dialogs and the
    classic layout.
  - Home has no "Popular modpacks" strip yet (DESIGN.md mentions it as not built).
  - The test data folder `build\run-theme` holds the seeded test instances used for the screenshots, plus two
    created during testing ("Sheet Test", "Simply Optimized Reloaded").

- **The friend server was never actually started in testing.** Starting it requires accepting the Minecraft
  EULA, which is your decision to make, so automated testing stopped after Set Up. Vanilla setup was verified
  end to end: `server.jar` downloaded, mods filtered, Start disabled without the EULA box. The process, console
  and stop code is untested against a real server.
- **Fabric, Forge and NeoForge server setup are untested.** There were no such instances locally. Old Forge
  versions (before 1.17) and NeoForge for 1.20.1 (`net.neoforged:forge`) may need a different installer URL.
  Vanilla and Quilt setup are verified.
- **The friend server firewall prompt:** the first real start will probably trigger a Windows Defender Firewall
  prompt for `java.exe`. Approve it on private networks for LAN play.
- **CurseForge is untested live,** because no API key is configured. The code follows Prism's request format and
  the documented response fields.
- **The handoff isn't fully automatic.** It pre-fills the source page's search, but you still click the pack
  there. Technic jumps straight to the exact pack.
- **Merge heuristics can be wrong:** generic names from different authors are kept apart, but the same pack
  under different titles won't merge. FTB detail requests are capped at 12 per search.
- **Microsoft login is disabled until you add a client ID.** Offline or demo accounts still work as in Prism.
  Without login, you can't really play online on the friend server.
- **The `FileSystem` test is excluded** because it needs symlink rights. Running it without Developer Mode loops
  UAC prompts. All other tests pass.
- **Only Windows was built and tested.** Linux and macOS packaging metadata were renamed but never built, and
  the `.icns` and Icon Composer asset are unverified.
- **Prism-named items deliberately kept** (D5b): the Java launcher package, strings for the disabled updater,
  and unused Prism logo files in `program_info/`.
- **Translations:** the new strings use `tr()` but have no translations yet.
