@echo off
REM ============================================
REM  Build script for Windows (MSVC / MinGW)
REM ============================================
setlocal enabledelayedexpansion

set WIN_DIR=%~dp0
set PROJ_DIR=%WIN_DIR%..
set BUILD_DIR=%WIN_DIR%build

if "%QTDIR%"=="" (
    echo Error: QTDIR environment variable not set.
    echo Set it to your Qt installation path, e.g.:
    echo   set QTDIR=C:\Qt\5.15.2\msvc2019_64
    exit /b 1
)

echo Cleaning build directory...
if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
mkdir "%BUILD_DIR%"

echo.
echo ============================================
echo  Configuring with qmake...
echo ============================================
cd "%BUILD_DIR%"
"%QTDIR%\bin\qmake.exe" "%WIN_DIR%\Statistiques.pro" -spec win32-msvc CONFIG+=release
if errorlevel 1 (
    echo qmake failed.
    exit /b 1
)

echo.
echo ============================================
echo  Building with nmake...
echo ============================================
nmake
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo.
echo ============================================
echo  Build complete!
echo  Binary: %BUILD_DIR%\release\Statistiques.exe
echo ============================================

cd "%PROJ_DIR%"
