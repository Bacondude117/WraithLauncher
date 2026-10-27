# Wraith Launcher 0.1.0 (first preview)

Wraith Launcher is a free, open-source Minecraft launcher based on Prism Launcher, with a new interface and extra tools.
It has no ads, no telemetry and no Wraith accounts. It is a fork, not affiliated with the Prism Launcher project; thanks
to everyone who made Prism Launcher, PolyMC and MultiMC.

## Highlights
- **A new interface:** Home, Library, Browse, Servers and Settings in a sidebar; instance pages with tabs; everything
  Prism can do is still there under "Advanced".
- **Browse every modpack source at once** (ATLauncher, CurseForge, FTB, Modrinth, Technic) with neutral ranking, and
  install from any of them.
- **Snapshots:** an automatic snapshot before mod, modpack, Minecraft or loader changes. Restore shows exactly what
  changes first; compare snapshots and get a changelog.
- **Creator tools:** a mod health check (missing dependencies, conflicts, duplicates, wrong loader), an export
  checklist for .mrpack and CurseForge, and config diffs.
- **Friendlier crashes:** plain-language explanations with a way to roll back; a log viewer with filters and search.
- **Performance:** a memory recommendation and garbage-collector presets, explained.
- **Library:** tags, bulk actions, a screenshot gallery, and import from other launchers (Prism, PolyMC, CurseForge,
  Modrinth App, ATLauncher, GDLauncher, the official launcher).
- **Friend servers:** turn an instance into a server for your friends. Optionally, play over the internet without port
  forwarding through playit.gg (needs your own playit.gg account).
- **Secure and private:** every download is checked against its published hash, HTTPS only, and sign-in tokens are
  encrypted. PRIVACY.md lists every server Wraith talks to, and every automatic request can be switched off.

## Good to know
- **Microsoft sign-in:** if this build has no Microsoft client ID, sign-in needs one under Settings → Network → APIs
  (see SETUP_KEYS.md).
- **Unsigned download:** the program isn't code-signed yet, so Windows may show a SmartScreen warning.
- **Data folder:** Wraith uses its own data folder and never touches Prism Launcher's. Use Import to copy your
  instances over.
- **Portable zip:** keeps everything next to the program.
- **Preview release:** some features haven't been tested with a live game yet (FEATURES.md marks them). Please report
  problems.
