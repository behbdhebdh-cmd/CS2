@echo off
setlocal EnableExtensions
cd /d "%~dp0\.."

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"

if not defined VSINSTALL (
  echo Visual Studio C++ toolset not found.
  exit /b 1
)

call "%VSINSTALL%\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 exit /b 1

set "CMAKE=%VSINSTALL%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "%CMAKE%" (
  echo CMake bundled with Visual Studio was not found.
  exit /b 1
)

if not exist build mkdir build
"%CMAKE%" -S . -B build -G "Visual Studio 18 2026" -A x64
if errorlevel 1 (
  echo Generator Visual Studio 18 2026 failed, trying Ninja...
  "%CMAKE%" -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  if errorlevel 1 exit /b 1
  "%CMAKE%" --build build
  exit /b %ERRORLEVEL%
)

"%CMAKE%" --build build --config Release --parallel
exit /b %ERRORLEVEL%
