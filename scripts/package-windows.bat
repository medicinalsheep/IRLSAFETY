@echo off
setlocal EnableExtensions

REM Creates a portable IRLSAFETY+ Windows package (zip-ready folder).
REM Version is read from buildspec.json (not hard-coded).

set "ROOT=%~dp0.."
for /f "delims=" %%V in ('powershell -NoProfile -Command "(Get-Content '%ROOT%\buildspec.json' -Raw | ConvertFrom-Json).version"') do set "VERSION=%%V"
if not defined VERSION set "VERSION=0.9.5"
set "PKG_NAME=IRLSAFETY+-v%VERSION%-win64"
set "OUT_DIR=%ROOT%\release\%PKG_NAME%"
set "SRC_DLL=%ROOT%\build_x64\rundir\RelWithDebInfo\irlsafety-plus.dll"
set "SRC_DATA=%ROOT%\build_x64\rundir\RelWithDebInfo\irlsafety-plus"

if not exist "%SRC_DLL%" (
  echo Plugin not built — running build-windows.bat first...
  call "%~dp0build-windows.bat"
  if errorlevel 1 exit /b 1
)

echo.
echo === Packaging %PKG_NAME% ===

if exist "%OUT_DIR%" rmdir /S /Q "%OUT_DIR%"
mkdir "%OUT_DIR%"
mkdir "%OUT_DIR%\obs-plugins\64bit"
mkdir "%OUT_DIR%\data\obs-plugins\irlsafety-plus"

copy /Y "%SRC_DLL%" "%OUT_DIR%\obs-plugins\64bit\"
for %%F in ("%ROOT%\build_x64\rundir\RelWithDebInfo\onnxruntime*.dll") do copy /Y "%%F" "%OUT_DIR%\obs-plugins\64bit\"
xcopy /E /I /Y "%SRC_DATA%" "%OUT_DIR%\data\obs-plugins\irlsafety-plus\"
copy /Y "%ROOT%\LICENSE" "%OUT_DIR%\"
copy /Y "%ROOT%\data\custom-pii.example.txt" "%OUT_DIR%\"
copy /Y "%~dp0install-from-package.bat" "%OUT_DIR%\"
copy /Y "%ROOT%\release\INSTALL-windows.txt" "%OUT_DIR%\INSTALL.txt"

echo.
echo Package ready: %OUT_DIR%
echo.
echo To test: open the folder and double-click install-from-package.bat
exit /b 0
