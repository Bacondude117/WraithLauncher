# Wraith Launcher: performance

"Lightweight" is a core value (CLAUDE.md), so startup time and memory are tracked against upstream Prism Launcher.
Re-measure after any change that adds something at startup, and add a row to the history.

## How to measure

```
wraith\build-windows.cmd build-release
powershell -File wraith\tools\measure-startup.ps1 -Exe "%LOCALAPPDATA%\Programs\PrismLauncher\prismlauncher.exe" -Template build\run-theme -ConfigName prismlauncher.cfg -Label Prism
powershell -File wraith\tools\measure-startup.ps1 -Exe build\Release\wraithlauncher.exe -Template build\run-theme -Label Wraith
```

The script copies the same data folder (11–12 test instances, no accounts) to a fresh folder under `build\perf` for every
run, starts the launcher with `--dir`, records the time until the main window has a title, waits 8 s, then reads the
working set and private bytes. It does 5 runs and reports medians. Qt's `bin` must be on PATH for the Wraith build.

- **Window:** time from process start to the main window existing (the launcher's own start-up work, as the user sees it).
- **Working set:** physical memory in use. **Private:** memory that belongs only to this process (includes graphics driver allocations).
- Machine: Windows 11, 125 % display scaling. Prism is the official 11.1.1 release build; Wraith is a local Release
  build of `wraith-dev` (based on Prism `develop`, 12.0.0-dev). Both use the same Qt major version.

## Current results (2026-10-27)

| Build | Window (median) | Working set | Private |
|---|---|---|---|
| Prism Launcher 11.1.1 | 263 ms | 60.8 MB | 76.9 MB |
| Wraith, classic layout | 314 ms | 70.7 MB | 82.3 MB |
| **Wraith shell (default)** | **382 ms** | **100.0 MB** | **119.3 MB** |
| Wraith shell, GPU interface on | 472 ms | 146.1 MB | 276.5 MB |

What the numbers mean:
- The shell adds about 120 ms to startup and about 40 MB of memory compared with Prism. Most of it comes from the
  QML engine, the Qt Quick libraries and the two views that exist at startup (sidebar and Home). Every other page is
  created only when it is first opened.
- The classic layout costs about 10 MB over Prism (the Qt Quick libraries are loaded even if unused, plus Wraith's
  own code).
- Qt Quick's GPU renderer added another ~150 MB of private memory (Direct3D driver allocations) and ~90 ms. Because of
  that, Wraith now uses Qt Quick's **software renderer by default**. To make that possible, cover art is drawn once in
  C++ by the image provider instead of with shader effects. The GPU renderer is still available (Settings →
  Appearance → Use the graphics card for the interface) for people who prefer smoother animations.

## History

| Date | Change | Wraith shell window / working set / private |
|---|---|---|
| 2026-10-27 | First measurement (round 3 shell, GPU renderer, shader blur) | 472 ms / 146.1 MB / 276.5 MB |
| 2026-10-27 | Software renderer by default; cover art blurred on the CPU | 382 ms / 100.0 MB / 119.3 MB |

## Ideas not yet done
- Create the Home page lazily after the window is shown (startup shows the sidebar first).
- Load the Qt Quick libraries only when the shell is used (the classic layout would then match Prism).
