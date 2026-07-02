@echo off
setlocal EnableExtensions

REM Run from inside the IRLSAFETY+-v0.1.3-win64 package folder.

set "PKG=%~dp0"
set "OBS_ROOT=%ProgramFiles%\obs-studio"

if not exist "%OBS_ROOT%\bin\64bit\obs64.exe" (
  echo [ERROR] OBS Studio not found. Install OBS 31.x or 32.x first.
  pause
  exit /b 1
)

echo Installing IRLSAFETY+ into OBS at %OBS_ROOT% ...
echo.

if not exist "%OBS_ROOT%\obs-plugins\64bit" mkdir "%OBS_ROOT%\obs-plugins\64bit"
if not exist "%OBS_ROOT%\data\obs-plugins\irlsafety-plus" mkdir "%OBS_ROOT%\data\obs-plugins\irlsafety-plus"

copy /Y "%PKG%obs-plugins\64bit\irlsafety-plus.dll" "%OBS_ROOT%\obs-plugins\64bit\"
for %%F in ("%PKG%obs-plugins\64bit\onnxruntime*.dll") do copy /Y "%%F" "%OBS_ROOT%\obs-plugins\64bit\"
if errorlevel 1 (
  echo [ERROR] Could not copy DLL. Right-click this file and choose "Run as administrator".
  pause
  exit /b 1
)

xcopy /E /I /Y "%PKG%data\obs-plugins\irlsafety-plus" "%OBS_ROOT%\data\obs-plugins\irlsafety-plus\"
if errorlevel 1 (
  echo [ERROR] Could not copy plugin data.
  pause
  exit /b 1
)

echo.
echo Done! Restart OBS and add the IRLSAFETY+ PII Blur filter.
echo Keep the filter at the TOP of your Filters list.
pause
exit /b 0