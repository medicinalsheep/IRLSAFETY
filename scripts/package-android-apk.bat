@echo off
setlocal EnableExtensions

REM Package IRLSAFETY+ Android debug APK (0.9.x-dev). Requires Android SDK + NDK.

set "ANDROID_DIR=%~dp0..\android"
set "GRADLEW=%ANDROID_DIR%\gradlew.bat"
set "OUT_DIR=%ANDROID_DIR%\app\build\outputs\apk\debug"

if not exist "%GRADLEW%" (
  echo [ERROR] Gradle wrapper not found. Open android\ in Android Studio first.
  exit /b 1
)

echo.
echo === IRLSAFETY+ Android APK ===
echo.

pushd "%ANDROID_DIR%"
call gradlew.bat assembleDebug
set "RC=%ERRORLEVEL%"
popd

if %RC% neq 0 (
  echo [ERROR] Build failed.
  exit /b %RC%
)

if not exist "%OUT_DIR%\app-debug.apk" (
  echo [ERROR] APK not found at %OUT_DIR%\app-debug.apk
  exit /b 1
)

set "RELEASE_DIR=%~dp0..\release"
if not exist "%RELEASE_DIR%" mkdir "%RELEASE_DIR%"

for /f "usebackq tokens=*" %%V in (`powershell -NoProfile -Command "(Get-Content '%ANDROID_DIR%\app\build.gradle.kts' | Select-String 'versionName = \"([^\"]+)\"').Matches.Groups[1].Value"`) do set "VER=%%V"

set "DEST=%RELEASE_DIR%\IRLSAFETY+-%%VER%%-android.apk"
copy /Y "%OUT_DIR%\app-debug.apk" "%DEST%" >nul

echo.
echo APK: %DEST%
echo Sideload to your phone and grant Camera permission.
echo See android\TESTER.md for Samsung A53 setup.
exit /b 0