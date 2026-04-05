@echo off
setlocal
cd /d "%~dp0"

where bazelisk >nul 2>&1
if %errorlevel% equ 0 (
  bazelisk run //:OminiDrive-UI
) else (
  bazel run //:OminiDrive-UI
)

if errorlevel 1 (
  echo.
  echo Build or run failed.
  pause
)

endlocal
