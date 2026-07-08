@echo off
REM Compatibility wrapper — use package-windows.bat going forward.
call "%~dp0package-windows.bat"
exit /b %ERRORLEVEL%
