# Wraith Launcher: making a release

## Build the packages (Windows)

```
set WRAITH_MSA_CLIENT_ID=<your Microsoft client ID>        (optional but strongly recommended, see below)
set WRAITH_CURSEFORGE_KEY=<your CurseForge API key>        (optional)
wraith\build-windows.cmd package
```

This configures a separate `build-release\` folder with `Wraith_RELEASE=ON`, so the version reads "0.1.0" without a
branch suffix. It builds the Release configuration, installs it into `dist\WraithLauncher-<version>-Windows-x64\`,
starts the packaged launcher once (smoke test: it must write `live.check`), and writes:

- `dist\WraithLauncher-<version>-Windows-x64.zip`: normal install; data in `%APPDATA%\WraithLauncher`.
- `dist\WraithLauncher-<version>-Windows-x64-Portable.zip`: the same plus `portable.txt`, so data stays next to the program.
- `dist\SHA256SUMS.txt`: checksums to publish with the files.

The version is set in the top-level `CMakeLists.txt` (`Launcher_VERSION_MAJOR/MINOR/PATCH`).

## Before publishing: checklist

1. **Keys** (never committed; see SETUP_KEYS.md):
   - Without a Microsoft client ID, nobody can sign in with a Microsoft account, so online play and the Play button
     need each user to enter their own ID (Settings → Network → APIs).
   - Register one in the Microsoft Entra admin center and request Minecraft API access from Mojang
     (https://aka.ms/mce-reviewappid), which takes a few days.
   - Without a CurseForge key, CurseForge is switched off in Browse and in content downloads.
2. **Test on a clean PC or user account** with the portable zip:
   - first-run wizard, sign in, create a Fabric instance, play;
   - install a modpack from Browse;
   - crash a test instance on purpose and check the crash card;
   - take a snapshot and restore it;
   - start a friend server, with and without playit.gg.
   These steps need an account or a game launch and weren't possible on the development machine (FEATURES.md lists
   them as "Moved").
3. **Tests:** `wraith\build-windows.cmd test` (never run the `FileSystem` test without Developer Mode).
4. **Docs:** update `PROGRESS.md` and the release notes (`RELEASE_NOTES.md`), and check that PRIVACY.md lists every
   endpoint.
5. **Tag** the release commit (`git tag 0.1.0`) and publish the zips plus `SHA256SUMS.txt`.

## Not included yet

- **Windows installer (.exe):** `program_info/win_install.nsi.in` is ready but needs NSIS (`makensis`), which isn't
  installed on the development machine. With NSIS installed, build it as upstream Prism does.
- **Code signing:** the packages are unsigned, so Windows SmartScreen will warn the first time. Signing needs a
  certificate.
- **macOS and Linux packages:** the code builds there upstream, but Wraith's new features were only tested on Windows,
  and the playit.gg integration is Windows-only.
- **Automatic updates:** there are none (the updater is disabled); users download new versions themselves.
