# Wraith Launcher: startup time and memory, compared with Prism Launcher (see PERFORMANCE.md).
# Usage: powershell -File wraith\tools\measure-startup.ps1 -Exe <launcher.exe> -Template <data dir> [-Runs 5] [-ConfigName wraithlauncher.cfg]
# Each run copies the template data dir to a fresh folder under build\perf, starts the launcher with --dir, waits for
# its main window, then samples memory after the window has settled. Nothing outside build\perf is written.
param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Template,
    [int]$Runs = 5,
    [string]$ConfigName = "",
    [int]$SettleSeconds = 8,
    [string]$Label = ""
)
$ErrorActionPreference = "Stop"
$root = Join-Path (Split-Path $PSScriptRoot -Parent | Split-Path -Parent) "build\perf"
New-Item -ItemType Directory -Force $root | Out-Null
$results = @()
for ($i = 1; $i -le $Runs; $i++) {
    $dir = Join-Path $root ("{0}-{1}" -f ($Label -replace '\W', ''), $i)
    if (Test-Path $dir) { Remove-Item -Recurse -Force $dir }
    Copy-Item -Recurse $Template $dir
    if ($ConfigName -ne "") {
        $cfg = Get-ChildItem $dir -Filter "*.cfg" | Where-Object { $_.Name -like "*launcher.cfg" } | Select-Object -First 1
        if ($cfg -and $cfg.Name -ne $ConfigName) { Copy-Item $cfg.FullName (Join-Path $dir $ConfigName) }
    }
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $p = Start-Process $Exe -ArgumentList "--dir", "`"$dir`"" -PassThru
    while ($sw.Elapsed.TotalSeconds -lt 60) {
        $p.Refresh()
        if ($p.MainWindowHandle -ne 0 -and $p.MainWindowTitle -ne "") { break }
        Start-Sleep -Milliseconds 20
    }
    $window = $sw.Elapsed.TotalMilliseconds
    Start-Sleep -Seconds $SettleSeconds
    $p.Refresh()
    $results += [pscustomobject]@{
        Run          = $i
        WindowMs     = [int]$window
        WorkingSetMB = [math]::Round($p.WorkingSet64 / 1MB, 1)
        PrivateMB    = [math]::Round($p.PrivateMemorySize64 / 1MB, 1)
    }
    Stop-Process -Id $p.Id -Force
    Start-Sleep -Milliseconds 800
}
$results | Format-Table -AutoSize | Out-String | Write-Output
function Median($values) { $s = $values | Sort-Object; $s[[int][math]::Floor($s.Count / 2)] }
"{0}: median window {1} ms, working set {2} MB, private {3} MB" -f $Label, (Median $results.WindowMs), (Median $results.WorkingSetMB), (Median $results.PrivateMB)
