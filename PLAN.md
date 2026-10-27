# Wraith Launcher — Plan

Each step is small, builds cleanly (`wraith\build-windows.cmd build`), passes tests
(`wraith\build-windows.cmd test`) and lands as one commit on `wraith-dev`.

## Phase 0 — Setup
- [x] 0.1 Toolchain installed; unmodified Prism builds, 22/23 tests pass (FileSystem needs symlink rights), app launches.
- [x] 0.2 CLAUDE.md, PLAN.md, DECISIONS.md, PROGRESS.md, SETUP_KEYS.md, `wraith/build-windows.cmd`.

## Phase 1 — Branding
- [x] 1.1 Names: `Launcher_CommonName=WraithLauncher`, `DisplayName=Wraith Launcher`, `ENVName=WRAITHLAUNCHER`,
      AppID `io.github.wraithlauncher.WraithLauncher`, binary `wraithlauncher`; rename the matching
      `program_info/` files; fix the few hard-coded strings (Q_INIT_RESOURCE, updater messages, man page,
      installer, .desktop/metainfo). Test: exe is `wraithlauncher.exe`, title says Wraith Launcher,
      data dir is `%APPDATA%\WraithLauncher`, config `wraithlauncher.cfg`.
- [x] 1.2 Keys & URLs (done together with 1.1): blank MSA client ID / CurseForge key / Imgur ID placeholders (SETUP_KEYS.md), disable
      the self-updater feed (no Wraith releases yet), clear Prism community links; keep Prism meta server.
      Test: builds; CurseForge page hidden when key empty; MS login reports missing client ID.
- [x] 1.3 Original ghost icon: new SVG + regenerated .ico/.png (scripted with Python, no external tools);
      replace launcher.svg in the multimc icon theme. Test: window/taskbar icon is the ghost.
- [x] 1.4 `Wraith` theme (`WraithTheme.{h,cpp}`): charcoal palette, pale-cyan/violet accents, small QSS.
      Registered as built-in; becomes the default on first run. Other themes unchanged.
- [x] 1.5 README "About this fork" section (fork of Prism, not affiliated/endorsed, GPL-3.0, credits).

## Phase 2 — Goal 1: Unified modpack browser
- [x] 2.1 Pure logic `launcher/modplatform/unified/UnifiedPack.{h,cpp}`: `UnifiedPack` struct, name
      normalisation, merge of duplicates across sources, ranking (relevance → popularity). Unit tests.
- [x] 2.2 Source searchers (`UnifiedSearch.{h,cpp}`): parallel NetJobs for Modrinth `/search`, CurseForge
      `/mods/search` (only when a key is configured), FTB `/modpack/search` + details, Technic `/search`,
      ATLauncher `packsnew.json` filtered locally. Each source failure is isolated.
- [x] 2.3 `UnifiedPage` (+ list model) in New Instance dialog: search box, merged cards with source tags,
      details pane, "Install from <source>" buttons that hand off to the existing per-source page with the
      search pre-filled (reuses all version-selection / install code).
- [x] 2.4 Page ordering: "Browse Modpacks" first among modpack sources; per-source pages kept unchanged after an
      "Advanced" heading page that explains/links them. Test manually: search "skyblock" shows merged multi-source cards.

## Phase 3 — Goal 2: UI refresh (theme-first, no removed options)
- [x] 3.1 Card delegate for unified results (rounded card, icon, title, author, summary, source chips, downloads).
- [x] 3.2 Wraith stylesheet polish: spacing, rounded inputs/buttons, list hover/selection, tab bar.
- [x] 3.3 Instance grid: optional card-style painting in `ListViewDelegate` when the Wraith theme is active
      (rounded hover/selection backgrounds, more padding). Existing behaviour unchanged elsewhere.

## Phase 4 — Goal 3: Friend server dashboard
- [x] 4.1 Logic `launcher/server/ServerModFilter.{h,cpp}`: classify mods via packwiz `.index` side metadata
      (from Modrinth client_side/server_side) + a small built-in list of well-known client-only mod ids. Tests.
- [x] 4.2 Setup (in `launcher/server/FriendServer`): create `<instance>/wraith-server/`, copy server-safe mods/config,
      vanilla jar from Mojang's manifest, Fabric server launcher, Forge/NeoForge installer; `eula.txt` only after
      the user accepts. (Quilt: not yet.)
- [x] 4.3 Process control (also `FriendServer`): start/stop (graceful `stop` command, then kill), instance's Java and
      memory; stdout → console; stdin command line.
- [x] 4.4 `ServerDashboardPage` instance page: status, setup, start/stop, live console, mod exclusion list,
      port/memory, EULA consent, LAN address hint.

## Phase 5 — Wrap-up
- [ ] 5.1 SUMMARY.md (changes, manual test steps, decisions, unfinished/risky items).
