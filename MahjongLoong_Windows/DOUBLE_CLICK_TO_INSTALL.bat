@echo off
title Mahjong Loong - Windows Installer

echo.
echo 🐉 Welcome to Mahjong Loong!
echo 🔧 Installing the game for you...
echo.

REM Check if we have a C++ compiler
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo ❌ C++ compiler not found!
    echo.
    echo 📦 Please install one of these first:
    echo    • MinGW-w64: https://www.mingw-w64.org/downloads/
    echo    • MSYS2: https://www.msys2.org/
    echo    • Visual Studio: https://visualstudio.microsoft.com/
    echo.
    echo 💡 Or download the pre-compiled version from your friend!
    echo.
    pause
    exit /b 1
)

REM Check if Raylib is available
echo 🎮 Checking for game engine...
if not exist "C:\raylib" (
    echo ❌ Raylib not found at C:\raylib
    echo.
    echo 📦 Please install Raylib first:
    echo    • Download from: https://www.raylib.com/
    echo    • Extract to C:\raylib\
    echo.
    echo 💡 Or ask your friend for a pre-compiled version!
    echo.
    pause
    exit /b 1
)

REM Compile the game
echo 🔨 Building the game...
g++ -std=c++17 -IC:/raylib/raylib/src main.cpp -LC:/raylib/raylib/src -lraylib -lgdi32 -lwinmm -O2 -s -o MahjongLoong.exe

if %errorlevel% equ 0 (
    echo.
    echo ✅ SUCCESS! The game is ready!
    echo 🎮 Double-click 'PLAY_GAME.bat' to start playing!
    echo.
) else (
    echo.
    echo ❌ Something went wrong during compilation.
    echo 📧 Please send a screenshot of this window to your friend.
    echo.
)

echo Press any key to close this window...
pause >nul
