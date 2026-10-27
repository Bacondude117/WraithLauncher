@echo off
rem Wraith Launcher - Windows developer build helper.
rem Usage: wraith\build-windows.cmd configure|build|build-release|test|run|install|install-debug|package
rem   install: copies a self-contained, double-clickable launcher (with Qt DLLs) into .\install
rem Override any of these paths by setting the variable before calling.
if not defined WRAITH_VS set "WRAITH_VS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"
if not defined WRAITH_QT set "WRAITH_QT=C:\Qt\6.11.2\msvc2022_64"
if not defined JAVA_HOME for /d %%J in ("C:\Program Files\Microsoft\jdk-17*") do set "JAVA_HOME=%%~J"

call "%WRAITH_VS%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
rem Keep vcpkg on the same toolset as the launcher, otherwise the STL ABI can mismatch at link time
set "VCPKG_VISUAL_STUDIO_PATH=%WRAITH_VS%"
set "CMAKE_PREFIX_PATH=%WRAITH_QT%"
set "PATH=%WRAITH_QT%\bin;%JAVA_HOME%\bin;C:\Program Files\Git\cmd;C:\Program Files\CMake\bin;%LOCALAPPDATA%\Microsoft\WinGet\Links;%PATH%"
cd /d "%~dp0.." || exit /b 1
rem local API keys (git-ignored, see SETUP_KEYS.md)
if exist "%~dp0local-keys.cmd" call "%~dp0local-keys.cmd"

if "%1"=="configure" cmake --preset default
if "%1"=="build" cmake --build --preset default --config Debug
rem Release builds live only in build-release\ (with keys, Wraith_RELEASE=ON); build\ is the Debug dev build
if "%1"=="build-release" powershell -NoProfile -ExecutionPolicy Bypass -File wraith\tools\package-windows.ps1 -BuildOnly
rem FileSystem test needs symlink rights (admin or Developer Mode); see PROGRESS.md
if "%1"=="test" ctest --preset default --build-config Debug -E "^FileSystem$"
rem install: the Release build into .\install (what "Wraith Launcher.cmd" starts); "install-debug" uses the Debug build
if "%1"=="install" powershell -NoProfile -ExecutionPolicy Bypass -File wraith\tools\package-windows.ps1 -InstallOnly
rem package: release zips in dist\ (RELEASE.md); keys from WRAITH_MSA_CLIENT_ID / WRAITH_CURSEFORGE_KEY
if "%1"=="package" powershell -NoProfile -ExecutionPolicy Bypass -File wraith\tools\package-windows.ps1
if "%1"=="install-debug" cmake --install build --config Debug --prefix install
if "%1"=="run" (
    if not exist build\run-data mkdir build\run-data
    start "" build\Debug\wraithlauncher.exe --dir "%CD%\build\run-data"
)
exit /b %ERRORLEVEL%
