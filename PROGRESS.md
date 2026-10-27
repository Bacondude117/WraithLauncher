# Progress

Resume from the first unchecked step in PLAN.md.

## Done
- 0.1 Environment set up; unmodified Prism builds (Debug, warnings-as-errors) and launches.
- 0.2 Project docs + `wraith/build-windows.cmd`.

- 1.1/1.2 Renamed to Wraith Launcher (exe `wraithlauncher.exe`, data dir `%APPDATA%\WraithLauncher`,
  env `WRAITHLAUNCHER_DATA_DIR`, config `wraithlauncher.cfg`); keys/updater/community URLs blanked.
  Verified: build OK, 22/22 tests, window title "Wraith Launcher 12.0.0-wraith-dev".
  Note: after changing CACHE defaults, delete `build\CMakeCache.txt` before configuring.

## Next
- 1.3 Ghost icon.

## Known issues
- `FileSystem` test needs symlink privilege (admin or Developer Mode). Excluded in the helper script.
  Running it without privilege causes looping UAC prompts — don't.
- CMake configure prints "'vswhere.exe' is not recognized" — harmless (vcvars probing).

