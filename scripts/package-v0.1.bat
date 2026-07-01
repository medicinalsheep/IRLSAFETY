@echo off
setlocal EnableExtensions

REM Creates a portable IRLSAFETY+ v0.1.0 test package (zip-ready folder).

set "ROOT=%~dp0.."
set "VERSION=0.1.0"
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
xcopy /E /I /Y "%SRC_DATA%" "%OUT_DIR%\data\obs-plugins\irlsafety-plus\"
copy /Y "%ROOT%\LICENSE" "%OUT_DIR%\"
copy /Y "%ROOT%\data\custom-pii.example.txt" "%OUT_DIR%\"
copy /Y "%~dp0install-from-package.bat" "%OUT_DIR%\"
copy /Y "%ROOT%\release\INSTALL-v0.1.txt" "%OUT_DIR%\INSTALL.txt"

echo.
echo Package ready: %OUT_DIR%
echo.
echo To test: open the folder and double-click install-from-package.bat
exit /b 0