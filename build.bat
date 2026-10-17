@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   Mirror's Edge Catalyst Network Shim Build Script
echo ========================================================

set "SCRIPT_DIR=%~dp0"
set "GCC_BIN="

:: 1. Check if gcc is on system PATH
where gcc.exe >nul 2>&1
if !ERRORLEVEL! equ 0 (
    set "GCC_BIN=gcc.exe"
) else (
    :: 2. Check standard MinGW / MSYS2 installation paths
    for %%P in (
        "C:\Program Files\mingw64\bin\gcc.exe"
        "C:\msys64\mingw64\bin\gcc.exe"
        "C:\msys64\ucrt64\bin\gcc.exe"
        "C:\mingw64\bin\gcc.exe"
    ) do (
        if exist %%P (
            set "GCC_BIN=%%~P"
            goto :found_gcc
        )
    )
)

:found_gcc
if "!GCC_BIN!"=="" (
    :: Fallback to CMake if available
    where cmake.exe >nul 2>&1
    if !ERRORLEVEL! equ 0 (
        echo [INFO] GCC not found. Building via CMake...
        cmake -B "!SCRIPT_DIR!build" -S "!SCRIPT_DIR!"
        cmake --build "!SCRIPT_DIR!build" --config Release
        if !ERRORLEVEL! equ 0 (
            if exist "!SCRIPT_DIR!build\Release\Engine.BuildInfo_Win64_retail.dll" (
                copy /y "!SCRIPT_DIR!build\Release\Engine.BuildInfo_Win64_retail.dll" "!SCRIPT_DIR!Engine.BuildInfo_Win64_retail.dll" >nul
            ) else if exist "!SCRIPT_DIR!build\Engine.BuildInfo_Win64_retail.dll" (
                copy /y "!SCRIPT_DIR!build\Engine.BuildInfo_Win64_retail.dll" "!SCRIPT_DIR!Engine.BuildInfo_Win64_retail.dll" >nul
            )
            echo [SUCCESS] Built via CMake: !SCRIPT_DIR!Engine.BuildInfo_Win64_retail.dll
            exit /b 0
        )
    )
    echo [ERROR] No suitable C compiler (MinGW-w64 GCC or CMake/Visual Studio) found.
    echo Please install MinGW-w64 GCC or CMake and ensure it is on your PATH.
    exit /b 1
)

echo [INFO] Compiling with "!GCC_BIN!"...
"!GCC_BIN!" -shared -static -static-libgcc -O2 -s ^
    -o "!SCRIPT_DIR!Engine.BuildInfo_Win64_retail.dll" ^
    "!SCRIPT_DIR!src\shim.c" ^
    "!SCRIPT_DIR!src\hook.c" ^
    "!SCRIPT_DIR!src\minhook\src\buffer.c" ^
    "!SCRIPT_DIR!src\minhook\src\hook.c" ^
    "!SCRIPT_DIR!src\minhook\src\trampoline.c" ^
    "!SCRIPT_DIR!src\minhook\src\hde\hde64.c" ^
    -I"!SCRIPT_DIR!src" ^
    -I"!SCRIPT_DIR!src\minhook\include" ^
    -I"!SCRIPT_DIR!src\minhook\src\hde" ^
    -lws2_32

if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] Built: !SCRIPT_DIR!Engine.BuildInfo_Win64_retail.dll
    exit /b 0
) else (
    echo [ERROR] Compilation failed.
    exit /b %ERRORLEVEL%
)
