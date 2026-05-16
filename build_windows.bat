@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

set "VCPKG_ROOT=C:\vcpkg"
set "QT_ROOT="

for /d %%V in ("C:\Qt\6.*") do (
    for %%A in (msvc2022_64 msvc2019_64) do (
        if exist "%%~fV\%%A\lib\cmake\Qt6\Qt6Config.cmake" (
            set "QT_ROOT=%%~fV\%%A"
        )
    )
)

if not defined QT_ROOT (
    echo ERROR: Qt6Config.cmake was not found under C:\Qt.
    echo.
    echo Check your installed Qt path with:
    echo   dir /s /b C:\Qt\Qt6Config.cmake
    echo.
    echo Install Qt 6 MSVC 2022 64-bit from Qt Maintenance Tool if the command returns nothing.
    exit /b 1
)

if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
    echo ERROR: vcpkg toolchain was not found at:
    echo   %VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake
    exit /b 1
)

echo Using Qt: %QT_ROOT%

echo.
echo Configuring...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
  -DCMAKE_PREFIX_PATH="%QT_ROOT%" ^
  -DQt6_DIR="%QT_ROOT%\lib\cmake\Qt6"
if errorlevel 1 exit /b %errorlevel%

echo.
echo Building Release...
cmake --build build --config Release
if errorlevel 1 exit /b %errorlevel%

if exist "%QT_ROOT%\bin\windeployqt.exe" (
    echo.
    echo Deploying Qt runtime...
    "%QT_ROOT%\bin\windeployqt.exe" --release --compiler-runtime "build\Release\AccountAssistant.exe"
)

echo.
echo DONE. EXE path:
echo   %cd%\build\Release\AccountAssistant.exe
