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

## Next
- 2.1 Unified search core logic + tests.

## Known issues
- `FileSystem` test needs symlink privilege (admin or Developer Mode). Excluded in the helper script.
  Running it without privilege causes looping UAC prompts — don't.
- CMake configure prints "'vswhere.exe' is not recognized" — harmless (vcvars probing).
- After changing CMake CACHE defaults, delete `build\CMakeCache.txt` before configuring.
- After changing the `.ico`, delete `build\launcher\CMakeFiles\WraithLauncher.dir\Debug\__\program_info\*.rc.res`
  (Ninja doesn't track the icon as a dependency of the resource script).
- macOS `.icns`/Icon Composer asset not verified (no Mac available).
- The main window "Welcome!" overlay still uses Prism's light styling (addressed in Goal 2).
