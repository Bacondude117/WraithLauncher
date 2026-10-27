@echo off
rem Starts the built Wraith Launcher (install\wraithlauncher.exe) for testing.
rem It uses the normal data folder (%APPDATA%\WraithLauncher), like an installed copy.
rem Build it first with: wraith\build-windows.cmd install
rem Optional: "Wraith Launcher.cmd" --dir <folder> to use another data folder.
if not exist "%~dp0install\wraithlauncher.exe" (
    echo Wraith Launcher isn't built yet. Run: wraith\build-windows.cmd install
    pause
    exit /b 1
)
start "" "%~dp0install\wraithlauncher.exe" %*
