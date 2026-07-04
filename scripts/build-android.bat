@echo off
setlocal EnableExtensions

REM Build IRLSAFETY+ Android debug APK (requires Android SDK + NDK).

set "ANDROID_DIR=%~dp0..\android"
set "GRADLEW=%ANDROID_DIR%\gradlew.bat"

if not exist "%GRADLEW%" (
  echo [ERROR] %GRADLEW% not found.
  echo Open android\ in Android Studio first to generate the Gradle wrapper, or install Gradle 8.10+.
  exit /b 1
)

echo.
echo === IRLSAFETY+ Android Build ===
echo.

pushd "%ANDROID_DIR%"
call gradlew.bat assembleDebug
set "RC=%ERRORLEVEL%"
popd

if %RC% neq 0 (
  echo [ERROR] Android build failed.
  exit /b %RC%
)

echo.
echo APK: %ANDROID_DIR%\app\build\outputs\apk\debug\app-debug.apk
exit /b 0