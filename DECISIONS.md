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

## D13 — Card styling is opt-in per theme
Card painting in the instance grid and the Welcome overlay is gated on `WraithTheme::isActive()`, so every other
theme renders exactly as in Prism, and no existing option or setting was removed.
