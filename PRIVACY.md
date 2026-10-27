# Wraith Launcher: privacy

Wraith Launcher has **no telemetry, no analytics, no crash reporting to anyone, no ads, no Wraith accounts and no
Wraith servers**. Everything it analyses (crash logs, mod lists, snapshots, changelogs, dependency checks) is
analysed on your computer. It only talks to the services listed below, and only to do something you use.

Every request goes over HTTPS. A plain `http://` address found in old metadata is upgraded to `https://`; a host that
doesn't support HTTPS then fails rather than downloading in the clear (Settings → Privacy & security can allow
http for people who need it). Login tokens are encrypted on disk (see VALUES_AUDIT.md).

What every request sends: the address itself, your IP address (as with any connection) and a user agent of the form
`WraithLauncher/<version>`. API keys you configure (CurseForge, Modrinth, Technic) are sent only to their own
service. Nothing identifies you to Wraith, because there is no Wraith service to identify you to.

**Keep this file current:** any change that adds a network endpoint must add it here (see CLAUDE.md, Core values).

## Automatic requests (and how to turn them off)

| When | Endpoint | Why | Off switch |
|---|---|---|---|
| Launcher start | `i18n.prismlauncher.org` (translation index) | List of interface languages | Settings → Privacy & security → Refresh the translation list on start |
| Launcher start, then every few hours while signed in | Microsoft/Xbox/Minecraft services (below) | Keeps your Microsoft account signed in (token refresh, profile and skin) | Remove the account, or use an offline account |
| Before launching a game | `meta.prismlauncher.org` | New Minecraft/loader versions | Settings → Privacy & security → Refresh version metadata when launching |
| Opening Browse | Every switched-on modpack source (below) | The "popular modpacks" list | Settings → Privacy & security → List popular modpacks when Browse opens; and the Sources checkboxes in Browse (a source that is switched off is never contacted) |
| Creating or launching an instance whose files are missing | Mojang, `meta.prismlauncher.org`, loader Mavens (below) | Downloading the game, libraries and assets | Inherent to playing; "Download game files when creating an instance" (Settings → General) only moves it to the first launch |
| Start and every 6 hours, **only if you turn it on** | `api.modrinth.com` (`/project/<id>/version`), `api.curseforge.com` (`/mods/<id>/files`) for your modpack instances | Modpack update notifications | Off by default: Settings → Privacy & security → Tell me about modpack updates |
| Launching with automatic Java on | Java vendors via `meta.prismlauncher.org` | Installing a matching Java | Settings → Java & memory → Download Java automatically |

There is no update check: the updater is disabled in this fork, and there is no news feed.

## Requests made only when you act

### Minecraft and Microsoft (playing, signing in)
- `login.live.com`, `login.microsoftonline.com`: Microsoft sign-in (device code or browser flow).
- `user.auth.xboxlive.com`, `xsts.auth.xboxlive.com`: Xbox Live tokens (required by Minecraft).
- `api.minecraftservices.com`: Minecraft profile, entitlements, skins and capes.
- `sessionserver.mojang.com`, `textures.minecraft.net`: profile textures (skins, capes).
- `piston-meta.mojang.com`, `libraries.minecraft.net`, `resources.download.minecraft.net`: game versions, libraries and
  assets (also used by the friend server for `server.jar`).
- `launcher.mojang.com` / `piston-data.mojang.com`: game and server jars (addresses come from Mojang's manifests).

### Version and Java metadata
- `meta.prismlauncher.org`: the version index for Minecraft, Fabric, Quilt, Forge, NeoForge, LiteLoader and Java (the
  Prism Launcher project's metadata server; Wraith has none of its own, see DECISIONS D5).
- Java downloads, by URL from that index: `api.adoptium.net` and `github.com` (Eclipse Temurin), `cdn.azul.com`
  (Azul Zulu), `piston-meta.mojang.com` / `piston-data.mojang.com` (Mojang's Java runtimes).
- `files.prismlauncher.org/fmllibs/`: old Forge (≤ 1.5.2) libraries.

### Mod loaders
- `maven.fabricmc.net`, `meta.fabricmc.net`: Fabric libraries, and the Fabric installer for friend servers.
- `maven.quiltmc.org`: Quilt libraries and the Quilt installer.
- `maven.minecraftforge.net`, `files.minecraftforge.net`: Forge libraries and installers.
- `maven.neoforged.net`: NeoForge libraries and installers.
- Libraries that a loader's metadata lists from other Mavens are fetched from those addresses.

### Modpacks and mods (Browse, Content tab, modpack updates)
- `api.modrinth.com`, `cdn.modrinth.com`: Modrinth search, versions and downloads.
- `api.curseforge.com`, `edge.forgecdn.net` / `mediafilez.forgecdn.net`: CurseForge (needs your own API key).
- `api.feed-the-beast.com`: FTB modpacks; files from the CDNs its manifests list.
- `download.nodecdn.net/containers/atl/`, `api.atlauncher.com`: ATLauncher pack index and files.
- `api.technicpack.net`, `solder.technicpack.net`, `mirror-mods.technicpack.net` and any Solder server a Technic pack
  names: Technic.
- `dist.creeper.host`: legacy FTB packs (the "Legacy FTB" page only).
- Icons and screenshots of packs and mods: from the image addresses those services return.

### Sharing (only when you press Upload/Share)
- Log upload: `0x0.st`, `hst.sh`, `paste.gg`/`api.paste.gg` or `api.mclo.gs`, whichever you choose in Settings → Network
  → APIs. Logs are anonymised first (paths and tokens removed) by Prism's `AnonymizeLog`.
- Screenshot upload: `api.imgur.com` (only if an Imgur client ID is configured; empty in this fork).

### Links that open in your browser (not requests made by Wraith)
Help and wiki pages (`prismlauncher.org/wiki`), mod and pack websites, `aka.ms/MinecraftEULA`, the translation site.

## Friend servers
A friend server runs on your computer. Wraith never opens ports on your router (no UPnP or NAT-PMP) and uses no relay,
so only people on your local network can join unless you forward the port yourself. `online-mode` stays on, and query
and RCON are off in the `server.properties` Wraith creates.

## Local data
Everything Wraith stores is in its data folder (`%APPDATA%\WraithLauncher` on Windows), in open formats: Prism's
instance format (`instance.cfg`, `mmc-pack.json`), JSON, INI and plain folders. Snapshots are plain files plus a JSON
index. The only encrypted file is `accounts.json`, because it contains login tokens.
