# Progress

Resume from the first unchecked step in PLAN.md.

## Done
- 0.1 Environment set up; unmodified Prism builds (Debug, warnings-as-errors) and launches.
- 0.2 Project docs + `wraith/build-windows.cmd`.
- 1.1/1.2 Renamed to Wraith Launcher (exe `wraithlauncher.exe`, data dir `%APPDATA%\WraithLauncher`,
  env `WRAITHLAUNCHER_DATA_DIR`, config `wraithlauncher.cfg`); keys/updater/community URLs blanked.
  Verified: 22/22 tests, window title "Wraith Launcher 12.0.0-wraith-dev".
- 1.3 Original ghost icon (SVG + `wraith/tools/gen_icons.py` for ico/png/icns). Verified exe icon.
- 1.4 Wraith theme, default on first run (with flat_white icons). Verified by screenshot. News toolbar hidden
  when no feed URL is configured.
- 1.5 README "About this fork"; About dialog states the fork/non-affiliation and credits Prism developers by name.

- 2.1 `Unified::` merge/rank logic + 10 unit tests (`tests/UnifiedPack_test.cpp`).
- 2.2 `UnifiedSearch`: parallel per-source NetJobs, partial results, isolated failures.
- 2.3/2.4/3.1 "Browse Modpacks" page with `PackCardDelegate` cards, details pane, "Install from X" handoff;
  "Advanced" heading page before the unchanged per-source pages. Verified live via UI automation
  (posted mouse messages; see Known issues): browse, search, merge (Sky Factory 4 = ATLauncher + Technic), handoff.

- 3.2/3.3 Instance grid: rounded hover/selection cards and a quiet Welcome card when the Wraith theme is active;
  other themes paint exactly as before. Verified by screenshot with dummy instances.

- 4.1 `WraithServer::classifyModsFolder` (packwiz side metadata, then known client-only list, else include) + tests.
- 4.2-4.4 `FriendServer` (setup: vanilla jar from Mojang manifest, Fabric server launcher, Forge/NeoForge installer
  `--installServer`; copies included mods + config folders; eula/server.properties; start/stop/console/commands)
  and the "Friend Server" instance page. Verified live: vanilla 1.20.1 setup downloaded server.jar, copied lithium,
  skipped sodium + modmenu, Start stays disabled until the EULA box is ticked.

- 5.1 SUMMARY.md written.

- Round 2 (user request): Quilt server support + Java auto-discovery (Quilt 1.20.1 setup verified live);
  Modrinth-style layout (nav rail, library header with search, loader/version tiles, Play button, Friend Servers
  view), verified by screenshots, including that the Dark theme restores Prism's classic layout;
  `wraith\build-windows.cmd install` produces a double-clickable `install\wraithlauncher.exe`.

- Round 3 (user request, "must no longer look like Prism"): FEATURES.md (full feature map) and DESIGN.md first,
  then the Wraith shell, built screen by screen with a screenshot review and a commit per screen
  (screenshots in `/screenshots`):
  - Shell + sidebar + Home + Library (Qt Quick in QQuickWidgets over MainWindow's actions).
  - Instance page (header, tabs; simple Content/Worlds lists; original pages under Advanced via PageHost).
  - Browse (filters panel, cards, details; Modrinth installs natively, verified end to end).
  - Settings (categories with simple controls; original pages under Advanced, applied on leave).
  - Servers (friend server cards, New server picker, state toasts).
  - New instance sheet (verified: Fabric 1.21.1 instance created), creation toasts, Esc back, Ctrl+N.
  - Upstream bug fix: InstanceView divide-by-zero when narrower than one item.
  - Verified: Edit Instance / Settings / Manage Accounts requests from Prism code land in the shell; 24/24 tests;
    `install\wraithlauncher.exe` runs the new UI with Qt off PATH.

- Round 4 (user request: core values, parity, new features):
  - Core values added to CLAUDE.md.
  - Step 1, values audit (VALUES_AUDIT.md, PRIVACY.md, PERFORMANCE.md):
    - Download hashes verified for CurseForge pack files, modpack archives and friend-server software.
    - HTTPS enforced, and accounts.json encrypted with DPAPI.
    - Source-neutral ranking, every Browse source installs directly, and switched-off sources are never contacted.
    - Friend server never overwrites manual edits, and has no open ports.
    - Off switches for automatic requests, and the software renderer by default (-46 MB working set, -157 MB private).
    - `Wraith Launcher.cmd` (and a local `.lnk`) at the repo root starts `install\wraithlauncher.exe`, now a
      Release build.
    - Verified: 25/25 tests; a Technic Solder install from Browse; FTB and ATLauncher version lists; screenshots
      22–24.
  - Step 2, parity audit (FEATURES.md rewritten with Verified/Moved statuses; nothing left Missing or Broken):
    - Fixed: the delete toast with Undo never appeared, and `--show` opened Prism's old window.
    - Fixed: missing shortcuts (Ctrl+Z, Ctrl+K/G/I/D, F2, Ctrl+0..9, Ctrl+, on Windows), and library keyboard
      navigation.
    - Verified by script: copy, rename, change group, icon picker, export (zip and mrpack), create shortcut, delete
      and Undo, `--show`, `--alive`, `--import`, and the shortcuts.
  - Step 3A, snapshots:
    - Built: `launcher/snapshots/` (store, diff and changelog logic, hooks; 7 unit tests), the instance Snapshots tab,
      and the automatic snapshot handler.
    - Verified in the app: an automatic snapshot on a mod toggle, manual snapshots, the restore preview and restore,
      compare, and the changelog.
  - Step 3B, modpack creator tools: `launcher/creator/` (ModCheck, TextDiff; 6 unit tests) and the instance Creator
    tab (health check, export checklist, changelog, config diff, test launch, pins), plus pin icons in Content.
    Verified: the health check on a real 16-mod pack, the export checklist, the changelog and the config diff.
  - Step 3C, advanced user tools:
    - Built: `launcher/advanced/` (JVM presets and memory advice, crash analyzer, shared folders; tests in
      WraithAdvanced_test), the instance Settings Performance and Shared cards, the Logs tab log viewer with crash
      cards, crash toasts, and library tags and bulk actions.
  - Step 3D, player comforts: import from other launchers (scanner, ForeignImportTask, Import page), opt-in modpack
    update notifications, and the screenshot gallery.
  - Verified: 28/28 tests; screenshots 33–38 (settings cards, crash explanation, log viewer, tags and bulk bar,
    gallery, an import scan of this PC's launchers).
  - Not verified (needs a game launch, account or real crash): test launch, the crash toast, update notifications
    with a real newer version, and copying an imported instance.
- Round 5 (user request: playit.gg + release):
  - **playit.gg:** an optional tunnel for friend servers. The official signed agent is checked by SHA-256 and code
    signature, and starts and stops with the server. Install verified; linking an account and the live tunnel need
    the user\'s playit.gg account.
  - **Release:** version 0.1.0, Wraith_RELEASE, and wraith\\build-windows.cmd package, which produces dist\\ zips
    (normal and portable) plus SHA256SUMS after a smoke test. RELEASE.md (checklist) and RELEASE_NOTES.md. Fixed the
    upstream qtlogging.ini install path.

## Next
- Follow-ups are in SUMMARY.md under "Unfinished and risky": a real game launch and server start (both need an
  account or the user's EULA consent), Fabric/Forge/NeoForge server setup tests, a CurseForge test with a key,
  and redesigning the dialogs marked 🟡 in FEATURES.md.

## Known issues
- `FileSystem` test needs symlink privilege (admin or Developer Mode). Excluded in the helper script.
  Running it without privilege causes looping UAC prompts — don't.
- CMake configure prints "'vswhere.exe' is not recognized" — harmless (vcvars probing).
- After changing CMake CACHE defaults, delete `build\CMakeCache.txt` before configuring.
- After changing the `.ico`, delete `build\launcher\CMakeFiles\WraithLauncher.dir\Debug\__\program_info\*.rc.res`
  (Ninja doesn't track the icon as a dependency of the resource script).
- macOS `.icns`/Icon Composer asset not verified (no Mac available).
- Unified search: CurseForge untested live (no API key on this machine). Handoff pre-fills the source page's
  search but the user still clicks the pack there (Technic resolves to the exact pack via `#slug`).
- Friend server: actually *running* a server was not tested, because that requires accepting the Minecraft EULA,
  which is the user's decision. Fabric/Forge/NeoForge setup paths are untested (no such instance locally);
  vanilla and Quilt setup are verified.
- GUI testing note: computer-use can't target the dev exe; UIA `Invoke` on "Add Instance" hangs (modal exec),
  so tests post WM_LBUTTONDOWN/WM_CHAR to the window instead.
