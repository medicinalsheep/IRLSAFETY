@echo off
setlocal EnableExtensions

REM IRLSAFETY+ one-click Windows build script (OBS plugin template).
REM Requires: Visual Studio 2022 Build Tools, CMake 3.28+

set "ROOT=%~dp0.."
set "BUILD_DIR=%ROOT%\build_x64"
set "CMAKE="

for %%P in (
  "%ProgramFiles%\CMake\bin\cmake.exe"
  "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
  "%ProgramFiles%\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
) do (
  if exist %%~P (
    set "CMAKE=%%~P"
    goto :found_cmake
  )
)

:found_cmake
if "%CMAKE%"=="" (
  echo [ERROR] CMake not found. Install CMake 3.28+ or Visual Studio 2022 with C++ workload.
  exit /b 1
)

echo.
echo === IRLSAFETY+ Build (Windows x64) ===
echo Project: %ROOT%
echo CMake:   %CMAKE%
echo.

pushd "%ROOT%"

echo [1/3] Configuring...
"%CMAKE%" --preset windows-x64
if errorlevel 1 (
  echo [ERROR] CMake configure failed.
  popd
  exit /b 1
)

echo [2/3] Building RelWithDebInfo...
"%CMAKE%" --build "%BUILD_DIR%" --config RelWithDebInfo
if errorlevel 1 (
  echo [ERROR] Build failed.
  popd
  exit /b 1
)

echo [3/3] Running tests...
"%CMAKE%" --build "%BUILD_DIR%" --target RUN_TESTS --config RelWithDebInfo
if errorlevel 1 (
  echo [WARN] Some tests failed — check output above.
)

echo.
echo Build complete.
echo Plugin DLL: %BUILD_DIR%\rundir\RelWithDebInfo\irlsafety-plus.dll
echo Locale:     %BUILD_DIR%\rundir\RelWithDebInfo\irlsafety-plus\
echo.
echo Next: run scripts\install-to-obs.bat  (or package-v0.0.bat)
popd
exit /b 0