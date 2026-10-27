# Wraith Launcher: core values audit

The core values are in CLAUDE.md, and they override everything else. This file records the audit of everything
built before they were adopted (rounds 1–3: branding, unified browser, friend servers, the Wraith shell), plus the
Prism code those features rely on, and what was changed as a result. New features are checked against the same list
when they are built (see the per-feature notes at the end).

Legend: ✅ complies · 🔧 violation found and fixed in this audit · ⚠️ known limitation, with the reason.

## Lightweight

| Check | Result |
|---|---|
| No embedded browsers or web runtimes | ✅ No QtWebEngine/WebView/WebChannel/Electron anywhere (checked the CMake files and vcpkg.json). The new UI uses Qt Quick, which is Qt's native scene graph renderer, not a web engine. Pack descriptions are shown as plain text (Browse) or with Qt's rich-text widget (original pages). |
| Heavy features load lazily | ✅ Shell pages (Home, Library, Browse, Servers, Settings) are created the first time they are opened. Instance pages are created on first open and cached per instance. Settings pages are built when Settings opens and freed when you leave it. The friend server process and its object only exist after you set up or open a friend server. The ATLauncher pack index is downloaded only when Browse searches that source. |
| Startup time and memory tracked against Prism | ✅ PERFORMANCE.md (method, numbers and how to re-run). |

## Completely free

| Check | Result |
|---|---|
| No ads, paywalls, donation nags or locked features | ✅ None exist. Nothing is gated on an account, licence or payment. |
| GPL-3.0 kept | ✅ `COPYING.md` unchanged; every new file has an SPDX GPL-3.0-only header; upstream headers untouched (D3). |
| Prism's authors credited | ✅ The About dialog and README credit the Prism Launcher, PolyMC and MultiMC contributors. 🔧 Settings → About in the new shell now says the launcher is made by the Prism Launcher contributors and is free with no ads, paywalls or locked features. |

## Secure

| Check | Result |
|---|---|
| Hash verification: game files, libraries, assets, Java, Modrinth/FTB/ATLauncher/Technic Solder mods, resource downloads | ✅ Already verified by Prism (SHA-1/SHA-256/SHA-512/MD5 validators). |
| Hash verification: CurseForge modpack files | 🔧 Prism downloaded the files of a CurseForge pack without checking them, although CurseForge publishes SHA-1 hashes. `FlameInstanceCreationTask` now adds a validator (`net/WraithChecksum.h`). |
| Hash verification: the modpack archive itself (`.mrpack`/CurseForge zip) | 🔧 `InstanceImportTask` downloaded the archive unchecked. It now verifies `file_hash`/`file_hash_algo` when the caller supplies them, and Browse, the Modrinth and CurseForge pages, and modpack updates (ManagedPackPage) now pass the published hash. |
| Hash verification: friend server software | 🔧 `server.jar` was downloaded unchecked. It is now checked against the SHA-1 in Mojang's version manifest. Forge, NeoForge and Quilt installers are checked against their Maven `.sha1` files. Fabric used a generated jar from `meta.fabricmc.net` that has no published hash, so Wraith now uses the official Fabric installer from Fabric's Maven (verified) and lets it write `fabric-server-launch.jar`. A download without a published hash now fails instead of running unverified code. |
| Hash verification: the rest | ⚠️ No hash is published for: images (pack icons, skins, capes; decoded as images, never executed), API responses (protected by TLS), Technic single-zip packs (the Technic API gives no checksum), legacy FTB packs, and imports from a URL you paste yourself. These are TLS-only. |
| HTTPS only | 🔧 All hard-coded addresses were already HTTPS, but metadata from third parties can still contain `http://` (old Forge/Technic/Solder links). `Net::Request` now upgrades every `http://` request to `https://` and fails if the host has no HTTPS. Qt's redirect policy already refuses HTTPS→HTTP redirects. An opt-out exists for people who need it (Settings → Privacy & security), and it is off by default. |
| Tokens never stored in plain text | 🔧 Prism stores Microsoft/Minecraft tokens in `accounts.json` as plain JSON (owner-only permissions). On Windows the file is now encrypted with DPAPI, tied to your Windows login (`security/SecretStore`). Plain files from Prism or older builds still load and are encrypted on the next save. A file that can't be decrypted (copied from another PC) is renamed to `accounts-locked.json`, never deleted. ⚠️ macOS/Linux: no system secret store is wired up yet (Keychain/libsecret would add a dependency), so there the file keeps Prism's owner-only permissions. Settings → Privacy & security shows which applies. |
| Server dashboard LAN-only by default | ✅ Wraith never opens ports (no UPnP, no NAT-PMP, no relay), so a friend server can only be reached from the local network unless you forward a port yourself. 🔧 The `server.properties` Wraith creates now explicitly turns query and RCON off (online-mode stays on), and the console tells you the LAN address to join and that no router ports are opened. |

## Private

| Check | Result |
|---|---|
| No telemetry or tracking | ✅ None in Prism or in Wraith's code. The user agent only names the launcher and its version. |
| No Wraith accounts or servers | ✅ None. The updater and news feed are disabled (D5), and community links are cleared. |
| Analysis runs locally | ✅ Unified search ranking and merging, client-only mod detection, and (new) crash analysis, changelogs, dependency checks and snapshots all run on your computer. Log upload happens only when you press Upload, and logs are anonymised first. |
| Every endpoint listed | ✅ PRIVACY.md lists every endpoint, split into automatic and on-demand requests, each automatic one with its off switch. |
| Disabled sources not contacted | 🔧 Unticking a source in Browse used to only hide its results; it was still queried. Unticked sources are now never contacted, and the choice is remembered. |

## Equal treatment

| Check | Result |
|---|---|
| Source-neutral ranking | 🔧 Within a relevance tier, packs were sorted by summed download counts. That favoured sources that publish large counts (CurseForge, Modrinth) over ones that publish none (ATLauncher, Technic). Ranking now uses each source's own order, which interleaves sources, and never compares raw downloads across sources (new tests: `rankWithinTierIgnoresRawDownloads`, `rankBrowsingInterleavesSources`). Sorting by downloads is still offered as an explicit choice. |
| No preferred source | 🔧 The source order was Modrinth first, and a merged pack took its name, summary and icon from Modrinth when available. Sources are now alphabetical everywhere, and merged data comes from the hit its own source ranked best (test `mergeTakesDataFromBestRankedHit`). |
| Duplicate packs show all sources | ✅ Merged cards show every source tag. 🔧 The details panel now lets you choose which source to install from (defaulting to the source that ranked the pack best for your search, not a fixed favourite). |
| Same install experience for every source | 🔧 Only Modrinth could be installed from Browse; the others opened their old dialog. Every source now has a version list and a direct Install in Browse, using that source's own install task: CurseForge (verified hash, respects authors who block third-party downloads), FTB, ATLauncher (with its optional-mods dialog) and Technic (Solder builds or single zip). Verified live for Technic (full install), FTB and ATLauncher (version lists). CurseForge needs an API key, which isn't on this machine. |
| Every loader equally supported | ✅ New instance: None, Fabric, Quilt, Forge, NeoForge, each with automatic version choice. Library filters and badges cover all of them. Friend servers support all five, and Fabric now uses its official installer like the others. ⚠️ LiteLoader (legacy, for Minecraft ≤ 1.12) is available from Versions & components, as in Prism, but not in the simple sheet. |

## Complete user control

| Check | Result |
|---|---|
| Instance folders isolated and Prism-compatible | ✅ Instances keep Prism's format; Wraith data lives next to it in plain files (`wraith-server/`, and new: `.wraith/` for snapshots). An instance copied to Prism works there. |
| No forced updates | ✅ No updater; modpack and mod updates only happen when you press Update (with Prism's confirmation). |
| Every automatic behaviour can be turned off | 🔧 Added switches (Settings → Privacy & security) for the automatic network requests Prism and Wraith make: the translation refresh on start (new setting; it was always on), metadata refresh before launch (existing setting, now in the simple view), and popular packs on opening Browse (new). Automatic Java download is in Settings → Java & memory. Snapshots (new) have their own switch. |
| No locked or proprietary storage | ✅ JSON, INI, and plain folders and files. The only encrypted file is `accounts.json` (tokens), encrypted with the operating system's own per-user protection. |
| Never overwrite manual edits | 🔧 Rebuilding a friend server deleted and re-copied `mods/` and overwrote `config/` and the other copied folders, which would erase changes made in the server folder. It now keeps a manifest of what it copied (`wraith-server-files.json`). It only replaces or removes a file that is still exactly what it wrote, and keeps (and reports) files you edited or added. Tested in `WraithValues_test`. 🔧 Starting a server rewrote `server-port` in `server.properties` from the dashboard every time; a port typed into the file by hand now wins. |
| Analyzers suggest, never auto-apply | ✅ The client-only mod filter only decides what is copied to the friend server, and every decision can be overridden per mod. New analyzers (crash analyzer, dependency checker) follow the same rule: they suggest and link, and you apply. |
| Nothing deleted without confirmation | ✅ Deleting an instance asks first (Prism's dialog) and goes to the trash with Undo. Removing a mod asks first. 🔧 The friend server mods-folder wipe above was a deletion without confirmation; it now only removes its own untouched copies, as part of the rebuild you asked for. |

## Feature notes (round 4)

Each new feature was designed against the values above:
- **Snapshots:** plain files and a JSON index inside the instance (`.wraith/snapshots`); automatic snapshots can be
  turned off; restore always shows a preview and asks; pruning the ring never removes pinned snapshots; deleting a
  snapshot asks first.
- **Modpack creator tools:** everything runs locally; exports go to a location you pick; the pre-export checklist only
  reports.
- **Crash analyzer, dependency checker, memory and GC advice:** suggestions with explanations, never applied
  automatically.
- **Update notifications:** off by default, and they only report.
