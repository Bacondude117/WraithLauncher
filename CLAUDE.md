# CLAUDE.md — Wraith Launcher

Wraith Launcher is a ghost-themed fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher)
(C++20 / Qt 6, CMake). Work happens on the `wraith-dev` branch. Keep changes small and upstream-friendly
so the fork can keep rebasing on Prism's `develop`.

## Build (Windows)

Prerequisites (installed on the dev machine): VS 2022 Build Tools (C++ workload), CMake ≥ 3.28, Ninja,
Git, Qt 6.11.2 `win64_msvc2022_64` with `qtimageformats` + `qtnetworkauth` (via aqtinstall into `C:\Qt`),
Microsoft OpenJDK 17. Third-party C/C++ deps come from vcpkg (`cmake/vcpkg` submodule, `vcpkg.json`).

```
wraith\build-windows.cmd configure   # cmake --preset default (Ninja Multi-Config + vcpkg toolchain)
wraith\build-windows.cmd build       # Debug build -> build\Debug\wraithlauncher.exe
wraith\build-windows.cmd test        # ctest, excluding FileSystem (needs symlink privilege)
wraith\build-windows.cmd run         # launches with a throwaway data dir: build\run-data
```

Gotchas:
- `Launcher_WARNING_AS_ERROR` is ON in the default preset — new code must be warning-free under MSVC /W4.
- If a newer VS is also installed, vcpkg may build deps with it and linking fails
  (`__std_find_first_not_of_trivial_pos_1`). The script pins `VCPKG_VISUAL_STUDIO_PATH`.
- Qt's `bin` must be on PATH to run tests or the exe from `build\Debug`.
- Windows PowerShell 5.1 `Get-Content`/`Set-Content` default to ANSI and corrupt UTF-8 docs (— becomes â€”).
  Edit docs with an editor or `python -X utf8`, or pass `-Encoding utf8` explicitly.
- Never run the `FileSystem` test: it elevates `*_filelink.exe` to create symlinks, which triggers UAC
  prompts in a loop on a machine without Developer Mode.
- UI screenshots: set `WRAITH_UI_SCRIPT=<file>` and run with `--dir build\run-theme` (seeded test instances).
  Commands are documented above `Shell::runUiScript`. Screenshots go to `/screenshots`. This machine has a GPU
  overlay tool that draws into Direct3D windows; in-app capture (`shot`) avoids it, external screen grabs don't.
- QML: a local component may not share a name with a Qt Quick Controls type (Qt 6.10+ has `SearchField`), and
  a model role must not be called `enabled`. `font.pixelSize` takes integers.
- `git commit -m` with double quotes in the message is mangled by Windows PowerShell 5.1; use `-F <file>`.
- `wraith\build-windows.cmd install` deploys QML imports via windeployqt `--qmldir`; it once exited 255 mid-way
  and succeeded when re-run.

## Architecture map

| Area | Location |
|---|---|
| App bootstrap, data dir, settings registration, theme defaults | `launcher/Application.cpp` |
| Branding variables (name, AppID, copyright) | `program_info/CMakeLists.txt` |
| URLs, API keys, binary name | top-level `CMakeLists.txt` (≈L178–281, L386) |
| Compile-time constants | `buildconfig/BuildConfig.{h,cpp.in}` |
| Installer / platform metadata | `program_info/` (`win_install.nsi.in`, `*.rc.in`, `*.manifest.in`, `.desktop.in`, `.metainfo.xml.in`) |
| Themes | `launcher/ui/themes/` (`ITheme`, `FusionTheme`, `DarkTheme`, `ThemeManager::initializeWidgets`) |
| Icon themes | `launcher/resources/<theme>/` + `ThemeManager::s_builtinIcons` |
| New-instance dialog & page list | `launcher/ui/dialogs/NewInstanceDialog.cpp` (`getPages()`) |
| Per-source modpack pages | `launcher/ui/pages/modplatform/{flame,modrinth,atlauncher,ftb,legacy_ftb,import_ftb,technic}` |
| Platform APIs | `launcher/modplatform/` (`ResourceAPI`, `flame/FlameAPI`, `modrinth/ModrinthAPI`, …) |
| Main instance grid | `launcher/ui/instanceview/` (`InstanceView`, `InstanceDelegate`) |
| Modpack list delegate | `launcher/ui/widgets/ProjectItem.{h,cpp}` |
| Instance launch | `launcher/LaunchController`, `MinecraftInstance::createLaunchTask`, `launcher/launch/` |
| Console / logs | `launcher/launch/LogModel`, `launcher/ui/pages/instance/LogPage`, `ui/widgets/LogView` |
| Instance pages registry | `launcher/InstancePageProvider.h` |
| Mod metadata (side info) | `modplatform/packwiz/Packwiz` (`side`), `ModPlatform::SideType` in `modplatform/ModIndex.h` |
| Java discovery | `launcher/java/JavaUtils`, `JavaInstallList` |
| Tests | `tests/*_test.cpp`, listed in `tests/CMakeLists.txt` `TEST_SOURCES` (single `LauncherTests` exe) |

## Where the Wraith goals live

All new Wraith-specific code goes in new files under clearly named folders so upstream files only get
small hook edits:

- **Branding**: CMake variables + renamed `program_info/` assets; `Wraith` theme in
  `launcher/ui/themes/WraithTheme.{h,cpp}` registered in `ThemeManager::initializeWidgets`.
- **Goal 1 – unified modpack browser**: `launcher/modplatform/unified/` (search fan-out, merge, ranking —
  pure logic, unit-tested) and `launcher/ui/pages/modplatform/unified/` (page). Registered first in
  `NewInstanceDialog::getPages()`; existing per-source pages stay, grouped as "Advanced".
- **Goal 2 – UI (round 3: full redesign)**: the Wraith shell in `launcher/ui/wraith/` replaces Prism's main
  window chrome, instance window and settings dialog (see DESIGN.md, FEATURES.md):
  - `WraithShell` (sidebar + page stack + toasts + UI test script), mounted by `MainWindowWraith.cpp::setupWraithShell`
    unless the `WraithClassicLayout` setting is on. `MainWindow` stays the controller; the shell drives its QActions.
  - QML in `ui/wraith/qml/` (tokens in `style/Theme.qml`, mirrored in `WraithStyle.h`), hosted in `QQuickWidget`s that
    share one engine; collected into a generated `wraith.qrc` (prefix `/wraith`) by `launcher/CMakeLists.txt`.
    Line icons in `ui/wraith/icons/` (`currentColor`, tinted by the `image://wraith/glyph/...` provider).
  - C++ bridges: `WraithBackend` (context property `backend`), `WraithInstanceModel`, `WraithListModels`,
    `WraithBrowse` (`browseModel`), `WraithNewInstance` (`newInstance`).
  - Original Prism pages are embedded by `WraithPageHost` (a `BasePageContainer`) and restyled by
    `shellStyleSheet()`/`modernizePage()`. `Application::showInstanceWindow`/`ShowGlobalSettings` route into the shell.
  - Classic layout + Wraith theme still uses the round-1/2 card delegates in `launcher/ui/instanceview/WraithTiles`.
- **Goal 3 – friend server dashboard**: `launcher/server/` (server setup/launch logic, client-only mod
  filtering) and `launcher/ui/pages/instance/ServerDashboardPage.{h,cpp}` registered in `InstancePageProvider.h`.

## Conventions

- `.clang-format`: Chromium base, 4-space indent, 140 cols, braces on new line for functions, `m_` members.
- New files: `// SPDX-License-Identifier: GPL-3.0-only` + short GPL header naming Wraith Launcher contributors.
- Keep upstream license headers untouched. Don't commit API keys (see `SETUP_KEYS.md`).
- Track work in `PLAN.md`, `PROGRESS.md`, `DECISIONS.md`.
