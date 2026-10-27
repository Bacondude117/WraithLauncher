# Wraith Launcher: build release packages for Windows (see RELEASE.md).
# Run through: wraith\build-windows.cmd package   (sets up the compiler environment first)
# Produces dist\WraithLauncher-<version>-Windows-x64.zip, ...-Portable.zip and SHA256SUMS.txt.
# Builds in build-release\ so the everyday Debug build in build\ is untouched.
param([string]$MsaClientId = $env:WRAITH_MSA_CLIENT_ID, [string]$CurseForgeKey = $env:WRAITH_CURSEFORGE_KEY)
$ErrorActionPreference = "Stop"
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
Set-Location $root

$version = (Select-String CMakeLists.txt -Pattern 'set\(Launcher_VERSION_(MAJOR|MINOR|PATCH) (\d+)\)' | ForEach-Object { $_.Matches[0].Groups[2].Value }) -join '.'
Write-Output "Packaging Wraith Launcher $version"

# keys are never committed; they come from the environment of the person building the release (SETUP_KEYS.md)
$configure = @("--preset", "default", "-B", "build-release", "-DWraith_RELEASE=ON", "-DLauncher_BUILD_PLATFORM=windows-x64")
if ($MsaClientId) { $configure += "-DLauncher_MSA_CLIENT_ID=$MsaClientId" } else { Write-Warning "No Microsoft client ID: users must enter their own in Settings > Network > APIs to sign in (SETUP_KEYS.md)." }
if ($CurseForgeKey) { $configure += "-DLauncher_CURSEFORGE_API_KEY=$CurseForgeKey" } else { Write-Warning "No CurseForge API key: CurseForge search is off unless users add a key." }
& cmake @configure
if ($LASTEXITCODE -ne 0) { throw "configure failed" }
& cmake --build build-release --config Release
if ($LASTEXITCODE -ne 0) { throw "build failed" }

$name = "WraithLauncher-$version-Windows-x64"
$stage = Join-Path $root "dist\$name"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
& cmake --install build-release --config Release --prefix $stage
if ($LASTEXITCODE -ne 0) { throw "install failed" }
foreach ($doc in "COPYING.md", "README.md", "PRIVACY.md", "SETUP_KEYS.md") { Copy-Item $doc $stage }

# smoke test: the packaged launcher starts and reports its version
$probe = Join-Path $root "dist\probe"
if (Test-Path $probe) { Remove-Item -Recurse -Force $probe }
$p = Start-Process (Join-Path $stage "wraithlauncher.exe") -ArgumentList "--dir", "`"$probe`"", "--alive" -PassThru
Start-Sleep -Seconds 8
$alive = Test-Path (Join-Path $probe "live.check")
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
Remove-Item -Recurse -Force $probe -ErrorAction SilentlyContinue
if (-not $alive) { throw "the packaged launcher didn't start (no live.check)" }
Write-Output "Smoke test passed"

$dist = Join-Path $root "dist"
$zip = Join-Path $dist "$name.zip"
$portableZip = Join-Path $dist "$name-Portable.zip"
Remove-Item $zip, $portableZip -ErrorAction SilentlyContinue
Compress-Archive -Path $stage -DestinationPath $zip
# portable: data next to the program (Prism's portable.txt convention)
Set-Content (Join-Path $stage "portable.txt") "Wraith Launcher keeps its data in this folder." -Encoding ascii
Compress-Archive -Path $stage -DestinationPath $portableZip
Remove-Item (Join-Path $stage "portable.txt")

$sums = foreach ($f in $zip, $portableZip) { "{0}  {1}" -f (Get-FileHash $f -Algorithm SHA256).Hash.ToLower(), (Split-Path $f -Leaf) }
$sums | Set-Content (Join-Path $dist "SHA256SUMS.txt") -Encoding ascii
$sums
