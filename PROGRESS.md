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

## Next
- All planned steps are done. Follow-ups are in SUMMARY.md under "Unfinished and risky": a real server start
  test (needs EULA consent), Fabric/Forge/NeoForge server setup tests, Quilt support, and a CurseForge test
  with a key.

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
  which is the user's decision. Fabric/Forge/NeoForge setup paths are untested (no such instance locally).
  Quilt is reported as unsupported.
- GUI testing note: computer-use can't target the dev exe; UIA `Invoke` on "Add Instance" hangs (modal exec),
  so tests post WM_LBUTTONDOWN/WM_CHAR to the window instead.
