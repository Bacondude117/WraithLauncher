<p align="center">
  <img alt="Wraith Launcher" src="/program_info/io.github.wraithlauncher.WraithLauncher.svg" width="128">
</p>

<h1 align="center">Wraith Launcher</h1>

<p align="center">
  A ghost-themed custom launcher for Minecraft: one search for every modpack source, a cleaner card-based UI,
  and one-click local servers for playing with friends.
</p>

## About this fork

**Wraith Launcher is a fork of [Prism Launcher](https://github.com/PrismLauncher/PrismLauncher).
It is not affiliated with, endorsed by, or supported by the Prism Launcher project.** Please don't report
Wraith bugs to Prism's issue tracker or community channels.

Wraith exists thanks to the work of the Prism Launcher contributors and, before them, the PolyMC and MultiMC
contributors. All of their copyright notices are kept in the source files, and the full credits are in the
launcher's About dialog. Like its upstream, Wraith Launcher is licensed under the
[GNU General Public License v3.0 only](LICENSE).

What Wraith changes on top of Prism:

- **Branding**: its own name, ghost icon and data folder (`%APPDATA%\WraithLauncher`, `~/.local/share/WraithLauncher`),
  so it never shares instances, settings or URL handlers with a real Prism install.
- **Wraith theme**: a default dark theme (deep charcoal, pale-cyan and violet accents). All of Prism's themes
  are still available under Settings → Appearance.
- **Unified modpack browser**: search CurseForge, Modrinth, ATLauncher, FTB and Technic at once, with duplicates
  merged into one card. The per-source pages are still there as "Advanced" options.
- **Friend server dashboard**: turn an instance into a local server with client-only mods excluded,
  start/stop controls and a live console.

Wraith still uses Prism's Minecraft metadata server (`meta.prismlauncher.org`) and links to Prism's help wiki,
because the launcher needs that metadata and the documentation is still accurate.

## API keys

Wraith ships **without** Microsoft or CurseForge API keys: Prism's keys were issued to Prism. Until you add
your own, Microsoft account login and CurseForge browsing are disabled. See [SETUP_KEYS.md](SETUP_KEYS.md).

## Building

Wraith builds exactly like Prism Launcher; see Prism's
[build instructions](https://prismlauncher.org/wiki/development/build-instructions). On Windows,
`wraith\build-windows.cmd configure|build|test|run` wraps the steps. [CLAUDE.md](CLAUDE.md) documents the
toolchain and the code layout.

## Upstream

- Prism Launcher: <https://github.com/PrismLauncher/PrismLauncher> ([original README](https://github.com/PrismLauncher/PrismLauncher/blob/develop/README.md))
- Prism Launcher's forking policy, which this fork follows: it makes clear that it is not Prism Launcher, and
  it removes Prism's API keys.

## License [![GPL-3.0-only](https://img.shields.io/badge/License-GPL--3.0--only-C4282D?logo=gnu)](LICENSE)

All launcher code is available under the GPL-3.0-only license.

The Wraith ghost icon is original artwork released under GPL-3.0-only. Prism Launcher's logo assets that remain
in `program_info/` (unused by Wraith builds) are under the CC BY-SA 4.0 license.
