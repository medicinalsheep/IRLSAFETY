@echo off
setlocal EnableExtensions

REM Installs IRLSAFETY+ into an existing OBS Studio installation.

set "ROOT=%~dp0.."
set "SRC_DLL=%ROOT%\build_x64\rundir\RelWithDebInfo\irlsafety-plus.dll"
set "SRC_DATA=%ROOT%\build_x64\rundir\RelWithDebInfo\irlsafety-plus"

if not exist "%SRC_DLL%" (
  echo [ERROR] Plugin not built. Run scripts\build-windows.bat first.
  exit /b 1
)

set "OBS_ROOT=%ProgramFiles%\obs-studio"
if not exist "%OBS_ROOT%\bin\64bit\obs64.exe" (
  echo [ERROR] OBS Studio not found at "%OBS_ROOT%"
  echo Install OBS 31.x from https://obsproject.com/ then re-run this script.
  exit /b 1
)

set "DEST_DLL=%OBS_ROOT%\obs-plugins\64bit"
set "DEST_DATA=%OBS_ROOT%\data\obs-plugins\irlsafety-plus"

echo.
echo === IRLSAFETY+ Install to OBS ===
echo OBS:  %OBS_ROOT%
echo From: %SRC_DLL%
echo.

if not exist "%DEST_DLL%" mkdir "%DEST_DLL%"
if not exist "%DEST_DATA%" mkdir "%DEST_DATA%"

copy /Y "%SRC_DLL%" "%DEST_DLL%\"
if errorlevel 1 (
  echo [ERROR] Could not copy DLL. Try running this script as Administrator.
  exit /b 1
)

xcopy /E /I /Y "%SRC_DATA%" "%DEST_DATA%\"
if errorlevel 1 (
  echo [ERROR] Could not copy plugin data.
  exit /b 1
)

echo.
echo Installed successfully.
echo  - %DEST_DLL%\irlsafety-plus.dll
echo  - %DEST_DATA%\locale\
echo.
echo Restart OBS, then add filter:  Filters ^> + ^> IRLSAFETY+ PII Blur
exit /b 0