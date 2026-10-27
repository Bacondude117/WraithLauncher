# API keys and client IDs

Wraith Launcher ships **no** API keys. Prism Launcher's keys were issued to Prism and must not be reused by
a derivative (CurseForge's terms say so explicitly). Until you add your own, Microsoft login and CurseForge
are unavailable.

| Key | CMake cache variable | In-app override | Where to get it |
|---|---|---|---|
| Microsoft (Xbox/Minecraft) auth client ID | `Launcher_MSA_CLIENT_ID` | Settings → APIs → "Microsoft Authentication" (`MSAClientIDOverride`) | Register an app in the Microsoft Entra admin center (public client, no secret), then request Minecraft API access from Mojang: https://aka.ms/mce-reviewappid |
| CurseForge API key | `Launcher_CURSEFORGE_API_KEY` | Settings → APIs → "CurseForge Core API" (`FlameKeyOverride`) | https://console.curseforge.com/ |
| Imgur client ID (screenshot upload, optional) | `Launcher_IMGUR_CLIENT_ID` | — | https://api.imgur.com/oauth2/addclient |

The defaults live in the top-level `CMakeLists.txt` ("API Keys" section) and are empty placeholders.

## Supplying keys at build time (recommended, keeps them out of git)

```
cmake --preset default -D Launcher_MSA_CLIENT_ID=<guid> -D Launcher_CURSEFORGE_API_KEY=<key>
```

Or put them in a git-ignored `CMakeUserPresets.json`:

```json
{
  "version": 8,
  "configurePresets": [{
    "name": "wraith-local", "inherits": "default",
    "cacheVariables": { "Launcher_MSA_CLIENT_ID": "<guid>", "Launcher_CURSEFORGE_API_KEY": "<key>" }
  }],
  "buildPresets": [{ "name": "wraith-local", "configurePreset": "wraith-local" }]
}
```

Never commit real values to `CMakeLists.txt`.
