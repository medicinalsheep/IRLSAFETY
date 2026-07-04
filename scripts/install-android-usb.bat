@echo off
setlocal EnableExtensions

REM Install IRLSAFETY+ APK to a USB-connected phone via adb.

set "APK="
set "RELEASE_DIR=%~dp0..\release"

for %%F in ("%RELEASE_DIR%\IRLSAFETY+-*-android.apk") do set "APK=%%~fF"

if not defined APK (
  echo [ERROR] No APK found in %RELEASE_DIR%
  echo Build first: scripts\package-android-apk.bat
  echo Or download from GitHub Releases into release\
  exit /b 1
)

where adb >nul 2>&1
if errorlevel 1 (
  echo [ERROR] adb not found. Install Android Platform Tools:
  echo https://developer.android.com/tools/releases/platform-tools
  exit /b 1
)

echo.
echo === IRLSAFETY+ USB Install ===
echo APK: %APK%
echo.

adb devices
echo.

adb install -r "%APK%"
set "RC=%ERRORLEVEL%"

if %RC% neq 0 (
  echo.
  echo [ERROR] adb install failed. On the phone:
  echo   - Accept the USB debugging prompt
  echo   - Try a data-capable USB cable / port
  exit /b %RC%
)

echo.
echo Installed. Open IRLSAFETY+ on the phone and grant Camera.
echo See android\TESTER.md for Samsung battery settings.
exit /b 0