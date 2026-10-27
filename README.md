<p align="center">
  <img alt="Wraith Launcher" src="/program_info/io.github.wraithlauncher.WraithLauncher.svg" width="128">
</p>

<h1 align="center">Wraith Launcher</h1>

<p align="center">
  A free, private Minecraft launcher with a modern interface, one search for every modpack source,
  and one-click servers for playing with friends.
</p>

<p align="center">
  <a href="https://github.com/Bacondude117/WraithLauncher/releases">Download</a> ·
  <a href="FEATURES.md">Features</a> ·
  <a href="PRIVACY.md">Privacy</a> ·
  <a href="CONTRIBUTING.md">Contributing</a>
</p>

> **Status: first preview (0.1.0).** Wraith is young. It works, but some features haven't been tested with a live game
> on every setup yet, and so far it has only been tested on Windows. Please
> [open an issue](https://github.com/Bacondude117/WraithLauncher/issues) if something breaks.

## What is Wraith Launcher?

Wraith Launcher manages your Minecraft installs. It keeps each game, modpack or server setup in its own isolated
instance so nothing interferes with anything else. It is a fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher):
everything Prism can do is still here, wrapped in a new interface with extra tools on top.

## Highlights

- **A cleaner interface.** A sidebar with Home, Library, Browse, Servers and Settings, card-based instance views and a
  dark ghost-themed look. Every Prism setting and tool is still reachable under "Advanced".
- **Browse every modpack source at once.** Search CurseForge, Modrinth, ATLauncher, FTB and Technic together. Results
  are ranked neutrally, and a pack that exists on several sources is shown once, with a tag for each source.
- **Snapshots.** Wraith saves a snapshot before it changes mods, modpacks, Minecraft or your mod loader. If an update
  breaks your game, you can see exactly what would change and roll back.
- **Friendlier crashes.** Crash logs get a plain-language explanation with a way to roll back, plus a log viewer
  with filtering and search.
- **Friend servers.** Turn an instance into a local server for your friends, with client-only mods left out, a
  start/stop dashboard and a live console. Optionally, friends can join over the internet without port forwarding
  using a free [playit.gg](https://playit.gg) account.
- **Tools for modpack makers.** A mod health check (missing dependencies, conflicts, duplicates, wrong loader), an
  export checklist for `.mrpack` and CurseForge packs, and config diffs.
- **Library tools.** Tags, bulk actions, a screenshot gallery, and import from Prism, PolyMC, the CurseForge app,
  the Modrinth App, ATLauncher, GDLauncher and the official launcher.
- **Performance help.** Memory recommendations and garbage-collector presets, with explanations.

See [FEATURES.md](FEATURES.md) for the full list and how each feature maps to Prism's.

## Our principles

- **Lightweight.** No embedded browser or web runtime.
- **Completely free.** No ads, paywalls or locked features.
- **Private.** No telemetry, no tracking, no Wraith accounts and no Wraith servers. [PRIVACY.md](PRIVACY.md) lists every
  server the launcher talks to, and every automatic request can be switched off.
- **Secure.** Downloads are checked against their published hashes, connections use HTTPS, and sign-in tokens are
  stored encrypted.
- **Fair to every mod source and loader.** No source or loader is favoured in search or features.
- **You stay in control.** Isolated instances, no forced updates, and nothing is deleted without asking.

## Getting started

1. Download the latest Windows package from the [Releases page](https://github.com/Bacondude117/WraithLauncher/releases).
   You can choose the installer, the zip, or the portable zip (which keeps all its data next to the program).
   Each release includes a `SHA256SUMS.txt` so you can verify your download.
2. Run it. Windows may show a SmartScreen warning, because the program isn't code-signed yet.
3. Create an instance, or open **Browse** to install a modpack.

Wraith stores its data in its own folder (`%APPDATA%\WraithLauncher` on Windows, `~/.local/share/WraithLauncher` on
Linux). It never touches an existing Prism Launcher install. To bring your instances over, use **Import**.

**macOS and Linux:** there are no official packages yet. The code builds on both platforms upstream, but Wraith's
new features have only been tested on Windows, and the playit.gg integration is Windows-only for now.

### Signing in with Microsoft

To play online you need a Microsoft account that owns Minecraft. Wraith can't bundle Prism's Microsoft and CurseForge
keys, because those were issued to Prism only. Until a build includes its own keys, you can supply yours in
**Settings → Network → APIs**:

- Without a Microsoft client ID, Microsoft sign-in is unavailable. Offline accounts still work.
- Without a CurseForge API key, CurseForge is switched off in Browse.

[SETUP_KEYS.md](SETUP_KEYS.md) explains how to get each key.

## Building from source

Wraith is a C++20 / Qt 6 project built with CMake. It builds like Prism Launcher does, so you can follow Prism's
[build instructions](https://prismlauncher.org/wiki/development/build-instructions). On Windows, the helper script
`wraith\build-windows.cmd` wraps the common steps:

```
wraith\build-windows.cmd configure
wraith\build-windows.cmd build
wraith\build-windows.cmd test
wraith\build-windows.cmd run
```

You'll need Visual Studio 2022 Build Tools (C++ workload), CMake 3.28 or newer, Ninja, Git and Qt 6. Development
happens on the `wraith-dev` branch. [CONTRIBUTING.md](CONTRIBUTING.md) explains how to send changes, and
[RELEASE.md](RELEASE.md) describes how releases are made.

## About this fork

Wraith Launcher is a fork of Prism Launcher. **It is not affiliated with, endorsed by, or supported by the Prism
Launcher project**, so please don't report Wraith problems to Prism's issue tracker or community channels.

Wraith exists thanks to the Prism Launcher contributors and, before them, the PolyMC and MultiMC contributors. Their
copyright notices are kept in the source files, and full credits are in the launcher's About dialog. This fork follows
Prism's forking policy: it makes clear that it is not Prism Launcher, and it doesn't reuse Prism's API keys.

Wraith still uses Prism's Minecraft metadata server (`meta.prismlauncher.org`) for game and loader versions, and links
to Prism's help wiki, which is still accurate for most features.

## Help and feedback

- Found a bug or have an idea? [Open an issue](https://github.com/Bacondude117/WraithLauncher/issues).
- Want to help? Read [CONTRIBUTING.md](CONTRIBUTING.md). Everyone taking part is expected to follow the
  [Code of Conduct](CODE_OF_CONDUCT.md).

## License

Wraith Launcher is free software, licensed under the [GNU General Public License v3.0 only](LICENSE). The Wraith ghost
icon is original artwork released under GPL-3.0-only. Prism Launcher's logo assets that remain in `program_info/`
(unused by Wraith builds) are under the CC BY-SA 4.0 license.

Minecraft is a trademark of Mojang Studios and Microsoft. Wraith Launcher is not an official Minecraft product and is
not approved by or associated with Mojang or Microsoft.
