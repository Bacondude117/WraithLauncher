# Progress

Resume from the first unchecked step in PLAN.md.

## Done
- 0.1 Environment set up; unmodified Prism builds (Debug, warnings-as-errors) and launches.
- 0.2 Project docs + `wraith/build-windows.cmd`.

## Next
- 1.1 Branding rename.

## Known issues
- `FileSystem` test needs symlink privilege (admin or Developer Mode). Excluded in the helper script.
  Running it without privilege causes looping UAC prompts — don't.
- CMake configure prints "'vswhere.exe' is not recognized" — harmless (vcvars probing).
