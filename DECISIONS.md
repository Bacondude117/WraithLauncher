# Decisions

Autonomous decisions made while building Wraith Launcher, with reasoning.

## D1 — Toolchain: Qt 6.11.2 via pinned aqtinstall, VS 2022, JDK 17
Matches Prism's CI (`.github/workflows/build.yml`, `setup-dependencies`). PyPI aqtinstall 3.3.0 can't read
Qt's new repo layout for 6.11, so the same git commit CI pins was installed. A VS "18" Build Tools install
also exists on this machine; vcpkg is pinned to VS 2022 (`VCPKG_VISUAL_STUDIO_PATH`) to avoid an STL ABI
link error.

## D2 — FileSystem test excluded locally
It needs symlink privileges and elevates `*_filelink.exe` through UAC, which looped prompts and crashed
(elevated process lacks Qt on PATH). Excluded via `ctest -E ^FileSystem$` in the helper script. Enabling
Windows Developer Mode would let it run; that's the user's call.

## D3 — Rename via CMake variables, keep upstream license headers
Branding is driven by `program_info/CMakeLists.txt` and `Launcher_APP_BINARY_NAME`. Changing these renames
the exe (`wraithlauncher.exe`), window titles, About dialog, installer, and the data dir
(`%APPDATA%\WraithLauncher`, `WRAITHLAUNCHER_DATA_DIR`), so it never shares folders with Prism. Per-file
"Prism Launcher - Minecraft Launcher" GPL headers are left untouched: they are attribution, and editing
~500 files would make every rebase conflict.

## D4 — AppID `io.github.wraithlauncher.WraithLauncher`
No Wraith domain exists; a reverse-DNS GitHub-style ID is conventional for Flatpak/desktop files and
can't collide with Prism's `org.prismlauncher.PrismLauncher`.

## D5 — Keep Prism's metadata server; disable updater and community links
`meta.prismlauncher.org` provides Minecraft/loader version metadata that the launcher can't work without;
there is no Wraith meta server. The GitHub updater would "update" Wraith into Prism, so its repo URL is
blanked. Discord/Matrix/Reddit/news/bug-tracker links point at Prism's community — cleared so we don't send
Wraith users to Prism support channels. Help wiki still points at Prism's docs (accurate, and labelled).

## D5b — Things deliberately left with Prism names
- Java launcher package `org.prismlauncher.*` (internal, never user-visible; renaming breaks rebases).
- Disabled-updater strings/filenames (`prism_launcher_update.log`, "Prism Launcher updater"): the updater is
  not built without `Launcher_UPDATER_GITHUB_REPO`, and they are translated strings.
- "Prism Launcher (zip)" export format: names Prism's file format, which Wraith still produces.
- `prismlauncher://` URLs are still *accepted* in `MainWindow` for compatibility, but the installer,
  `.desktop` and macOS plist no longer *register* that scheme, so a real Prism install keeps it.
- Unused Prism logo art in `program_info/` (`*.logo*.svg`, `bigsur`, `Social`, `Source`) stays untouched
  to keep rebases clean; nothing in the build installs it.

## D6 — API keys become empty placeholders
Prism's MSA client ID, CurseForge key and Imgur client ID were issued to Prism and CurseForge's terms
require derivatives to use their own. Defaults are now empty; the launcher already supports override via
CMake cache vars or the in-app API settings. Documented in SETUP_KEYS.md.

## D7 — Unified browser hands off installs to existing per-source pages
Every provider page implements `ModpackProviderBasePage::setSearchTerm`. The unified page searches all
sources in parallel and merges results; choosing "Install from X" switches the dialog to X's page with the
pack name searched. This reuses each source's version picker and install task (ATLauncher optional mods,
Technic Solder, etc.) instead of duplicating fragile install logic.

## D7b — "Advanced" heading page instead of renaming the per-source pages
The per-source pages are untouched upstream classes. Rather than edit five files to rename them or hide them,
an "Advanced" page is inserted before them in the sidebar; it explains them and links to each. Nothing is hidden.

## D8 — Unified search per-source strategy
Modrinth `/search` and CurseForge `/mods/search` give relevance-ordered results with download counts.
FTB `/modpack/search/N?term=` returns IDs, followed by `/modpack/<id>` for details (installs). Technic
`/search` has no popularity data. ATLauncher has no search API; `packsnew.json` is fetched and filtered
locally (its `position` is used as a popularity proxy). Legacy FTB (dead CDN) and FTB App import (local)
are not part of the unified search.

## D8b — Ranking
Relevance = 0.7 × name match + 0.3 × best per-source rank + 0.05 per extra source, exact name matches pinned to
the top tier. Packs are sorted by relevance in tiers of 0.1, then by summed downloads/installs inside a tier
("relevance first, then popularity"). With an empty query the browser shows the most popular packs.
CurseForge has no relevance sort, so it is queried by popularity and re-ranked locally.

## D9 — Friend server lives in `<instance>/wraith-server/`, built from copies
The instance is never modified; mods and configs are copied, so the server can't corrupt the player's game files
and "Rebuild" is always safe. Settings (port, memory, EULA choice, per-mod overrides) live in
`wraith-server.json`. The server object is parented to the instance, so it outlives the instance window (console
history kept) and is stopped (`stop`, then kill after 15 s) when the instance or the launcher goes away.

## D10 — Client-only detection order
1. user override, 2. packwiz `.index` `side` (Modrinth client_side/server_side, written when mods are installed
through the launcher), 3. a short list of well-known client-only mods, 4. otherwise include. Including an
unknown mod is the safer failure: Fabric skips client-only mods itself, while wrongly excluding a gameplay mod
breaks joining.

## D11 — Server software per loader
Vanilla: Mojang version manifest. Fabric: official server launcher jar from meta.fabricmc.net (downloads vanilla
on first start). Forge/NeoForge: official installer with `--installServer`, launched via the generated
`win_args.txt`/`unix_args.txt` (modern) or the forge jar (legacy). Quilt: not yet (needs the Quilt installer).
Java and memory come from the instance's settings, like a normal launch.

## D12 — The EULA is never accepted automatically
`eula.txt` gets `eula=true` only after the user ticks the checkbox; Start stays disabled otherwise. Automated
tests therefore stop before actually running a server.

## D14 — Modrinth-style layout reuses the existing toolbars
The user asked for a full UI overhaul like Modrinth/CurseForge. Instead of a rewrite, `MainWindowWraith.cpp` moves
the original main toolbar (all its actions, accounts at the bottom) to a left navigation rail and adds Library /
Browse / Servers entries; a library header (title, search, "New instance") sits above the grid; instance tiles show
loader + Minecraft version; the Launch button is a Play button; a "Friend Servers" view lists every server.
Selecting any other theme puts Prism's classic layout back live, so no option is lost and upstream files only get
small hooks. The rail toolbar uses a different object name so the two layouts' saved positions don't fight.

## D15 — Quilt servers via the official installer; Java auto-discovery
Quilt uses `quilt-installer install server <mc> <loader> --download-server`. When an instance has never been
launched (no Java chosen), the server picks an installed or launcher-managed Java whose major version (from the
JDK `release` file) matches the version's requirement, else the newest 17+.

## D16 — Full UI redesign as a hybrid Qt Quick shell (supersedes D14)
The user asked for a UI that a Prism user would not recognise. Restyling Qt Widgets could not get there, so the
primary screens (sidebar, Home, Library, Browse, Servers, instance header, content and world lists, New instance
sheet) are QML in `QQuickWidget`s sharing one engine. MainWindow stays alive as the controller: its actions,
dialogs, URL handling and task running are what the shell calls, so behaviour and upstream fixes carry over.
The power-user pages (mod manager, components, logs, per-instance and global settings) are the original
`BasePage`s, embedded in the shell by `PageHost` (a `BasePageContainer`) and restyled. Rewriting them would risk
losing options and would diverge for no user benefit, since they are already "one click away" under Advanced.
Prism's classic layout is kept behind a setting (`WraithClassicLayout`) as an escape hatch.

## D17 — Settings edits: simple controls write directly, pages apply on leave
The original settings pages load their values when constructed and write them all in `apply()`. Global pages are
therefore built fresh when Settings opens and applied when it is left, as Prism's dialog does on OK. Only pages
that were opened are applied, so an untouched page never overwrites a simple control's change. When a simple
control changes a setting whose advanced page is open, that page is applied first and then rebuilt or reloaded.

## D18 — What stays a dialog
Dialogs that ask for a decision in the middle of a task (progress, failures, blocked mods, confirmations) and the
specialised editors (copy, export, icon picker, resource download, loader/Java install, account login, setup
wizard) still open as the original dialogs, in the Wraith theme. FEATURES.md marks them 🟡. Redesigning them is
follow-up work and doesn't block the navigation and layout goals.

## D19 — Modrinth installs natively in Browse; other sources hand off
Modrinth's version list is a single API call, and Prism's `InstanceImportTask` installs a `.mrpack` URL, so Browse
installs Modrinth packs directly, icon included. CurseForge (no key here), FTB, ATLauncher and Technic have
source-specific steps (optional mods, Solder, blocked files), so Browse opens their original page in the full
dialog with the search filled in, rather than duplicating those flows.

## D20 — Upstream bug fixed in place: InstanceView items per row
`InstanceView::calculateItemsPerRow()` returned 0 for a view narrower than one item, and `VisualGroup::update()`
then divided by zero. The shell keeps that view hidden at zero width, which surfaced it as a Qt debug assert. It is
clamped to 1, a one-line fix suitable for upstream.

## D21 — Core values audit (round 4)
The user set core values (CLAUDE.md) that override everything. The audit and its fixes are in VALUES_AUDIT.md. The
decisions that changed earlier work:
- **D19 is superseded:** Browse installs every source directly, using each source's own Prism install task, because
  installing only Modrinth natively treated sources unequally. "Open in the <source> page" stays for source-specific
  extras.
- **Ranking (D8b) no longer compares download counts across sources.** They aren't comparable, and ATLauncher and
  Technic publish none, so those sources sank. Each source's own order is used inside a relevance tier.
- **Fabric friend servers use the official Fabric installer** instead of meta.fabricmc.net's generated server jar,
  because that jar has no published hash and the values require verified downloads (D11 updated).
- **Tokens:** `accounts.json` is encrypted with DPAPI on Windows. A format with a text header was chosen, so Prism's
  plain files still load and an unreadable file is renamed, never deleted. macOS/Linux are left as a recorded
  limitation, because Keychain/libsecret would add dependencies; that needs its own decision.
- **HTTPS:** enforced centrally in `Net::Request` by upgrading `http://`, rather than by auditing every metadata
  source. An opt-out setting exists because user control is also a core value; it is off by default.

## D22 — Software rendering for the shell by default
Qt Quick's GPU renderer cost about 150 MB of private memory (Direct3D driver allocations) and about 90 ms of startup.
That conflicts with "lightweight", which outranks the UI goals. The shell therefore uses Qt Quick's software renderer
by default. The one GPU-only effect (blurred cover art via MultiEffect) was replaced by a cover drawn once in C++ by
the image provider, which looks the same. Smoother GPU rendering is a user setting. Numbers are in PERFORMANCE.md.

## D23 — Friend server files: manifest instead of wiping
"Never overwrite manual edits" ruled out the earlier "delete mods/ and copy again". `ServerFileSync` records the
SHA-1 of every file Wraith copies, and only replaces or removes files that still match it. This is simple, testable and
needs no prompts during a rebuild.

## D24 — Snapshot design
- **Where:** inside the instance (`.wraith/snapshots`), so snapshots move with the instance and need no global
  database. The store is plain files: contents named by SHA-1 (stored once), plus one JSON file per snapshot, so it
  can be inspected or recovered by hand (no proprietary storage). Prism's export and copy leave it out.
- **Hooks, not a watcher:** one-line `Snapshots::beforeChange()` calls in Prism code (PackProfile component edits,
  ResourceFolderModel install/remove/enable, the resource download/update dialogs, modpack update). The UI installs
  the handler, so the logic layer and the tests have no UI dependency. A burst of changes gets one snapshot (five-minute
  window per instance).
- **Blocking with progress:** a snapshot must finish before the change it protects. It runs on a worker thread while
  a modal progress dialog keeps the window alive. A hash cache (path, size, mtime) makes snapshots after the first
  one quick.
- **Restore never rolls back `instance.cfg`:** it holds playtime counters and settings that the launcher keeps in memory
  and would overwrite. Restore saves the current state as a snapshot first, so it can always be undone.
- **The ring removes old snapshots automatically.** That is the behaviour the user asked for (default 10). It can be
  set to keep everything, pinned snapshots are never removed, and deleting one by hand asks first.

## D25 — Modpack creator tools build on Prism and snapshots
- **Dependency checker** reads each jar's own metadata (Fabric, Quilt, Forge, NeoForge), including jars bundled inside
  mods (depth 2), so modules such as Fabric API's count as installed. It runs locally and doesn't ask a mod platform's
  API, because some mods aren't on any platform and the check should work offline. It reports with a suggestion and
  never changes files. An unparseable version range is never reported as a problem, to avoid false alarms.
- **Version pinning and bulk update review** already exist in Prism (update lock; the update review dialog with
  per-mod selection and changelogs). Wraith exposes the lock as a pin icon and takes a snapshot when updates are
  accepted, instead of writing a second updater.
- **Export** keeps Prism's .mrpack/CurseForge export (it resolves hashes on the platforms). Wraith adds a checklist
  before it. The checklist only reports; the user unticks files in the export dialog.
- **Test launch** = snapshot, then a normal launch. When the game exits, a toast reports how many files changed, with
  a link to Snapshots for a reviewed rollback. Nothing is rolled back automatically.

## D26 — Advanced tools and player comforts
- **Shared folders use directory junctions on Windows** (via `mklink /J`), because symlinks need admin rights or
  Developer Mode (see the FileSystem test's UAC loop). Elsewhere they are symlinks.
- **Log viewer reads files from disk** (latest.log is watched), so it covers older logs and crash reports too. Prism's
  live console stays one click away for the running game's output.
- **Crash analysis is pattern matching on known causes**, ordered most specific first, and never auto-applies a fix.
  A crash toast leads to the log viewer, where rollback means opening the Snapshots restore preview.
- **Import copies, never moves.** Prism-format instances are copied as they are. Others become a new instance with the
  same Minecraft version and loader, and their game files are copied in (account and launcher files excluded). Newer
  Modrinth App versions keep the Minecraft version in a SQLite database, which isn't read, so those profiles are listed
  but can't be imported.
- **Update notifications cover Modrinth and CurseForge modpacks only**, because Prism records managed-pack metadata only
  for those. Packs from other sources are not checked; they get the same feature as soon as their metadata exists.
- **The screenshot gallery is a page off the Library** (no new sidebar entry), to keep the requested sidebar
  (Home, Library, Browse, Servers, Settings).

## D27 — playit.gg: run the official agent, don't reimplement it
The user asked for built-in playit.gg support as an option.
- **Run the official agent:** Wraith runs playit.gg's own Windows agent (portable mode, its own console window, which
  shows the account link and tunnel status) instead of reimplementing playit's protocol or using their API with a stored
  secret. That keeps Wraith out of the user's playit account and secrets, and agent updates come from playit.gg.
- **Download and checks:** the agent is fetched from the newest stable GitHub release whose asset has a published
  SHA-256 digest; the signed build is preferred and its Authenticode signature is checked.
- **Lifecycle:** it starts with a friend server that has the option on and stops with it (or when Wraith quits).
- **Address:** read best-effort from the agent's logs, with a field to paste it.
- **Windows only for now:** the Linux agent's portable mode isn't verified.
- **Not tested here:** linking an account and creating the tunnel need the user's own playit.gg account.

## D13 — Card styling is opt-in per theme
Card painting in the instance grid and the Welcome overlay is gated on `WraithTheme::isActive()`, so every other
theme renders exactly as in Prism, and no existing option or setting was removed.
