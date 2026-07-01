@echo off
setlocal EnableExtensions

REM Run from inside the IRLSAFETY+-v0.0.0-win64 package folder.

set "PKG=%~dp0"
set "OBS_ROOT=%ProgramFiles%\obs-studio"

if not exist "%OBS_ROOT%\bin\64bit\obs64.exe" (
  echo [ERROR] OBS Studio not found. Install OBS 31.x first.
  pause
  exit /b 1
)

echo Installing IRLSAFETY+ into OBS at %OBS_ROOT% ...

if not exist "%OBS_ROOT%\obs-plugins\64bit" mkdir "%OBS_ROOT%\obs-plugins\64bit"
if not exist "%OBS_ROOT%\data\obs-plugins\irlsafety-plus" mkdir "%OBS_ROOT%\data\obs-plugins\irlsafety-plus"

copy /Y "%PKG%obs-plugins\64bit\irlsafety-plus.dll" "%OBS_ROOT%\obs-plugins\64bit\"
xcopy /E /I /Y "%PKG%data\obs-plugins\irlsafety-plus" "%OBS_ROOT%\data\obs-plugins\irlsafety-plus\"

echo.
echo Done! Restart OBS and add the "IRLSAFETY+ PII Blur" filter to any source.
pause
exit /b 0