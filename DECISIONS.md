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

## D6 — API keys become empty placeholders
Prism's MSA client ID, CurseForge key and Imgur client ID were issued to Prism and CurseForge's terms
require derivatives to use their own. Defaults are now empty; the launcher already supports override via
CMake cache vars or the in-app API settings. Documented in SETUP_KEYS.md.

## D7 — Unified browser hands off installs to existing per-source pages
Every provider page implements `ModpackProviderBasePage::setSearchTerm`. The unified page searches all
sources in parallel and merges results; choosing "Install from X" switches the dialog to X's page with the
pack name searched. This reuses each source's version picker and install task (ATLauncher optional mods,
Technic Solder, etc.) instead of duplicating fragile install logic.

## D8 — Unified search per-source strategy
Modrinth `/search` and CurseForge `/mods/search` give relevance-ordered results with download counts.
FTB `/modpack/search/N?term=` returns IDs, followed by `/modpack/<id>` for details (installs). Technic
`/search` has no popularity data. ATLauncher has no search API; `packsnew.json` is fetched and filtered
locally (its `position` is used as a popularity proxy). Legacy FTB (dead CDN) and FTB App import (local)
are not part of the unified search.
