@echo off
title Mahjong Loong - Game Launcher

REM Check if the game exists
if not exist "MahjongLoong.exe" (
    echo ❌ Game not found!
    echo 🔧 Please run 'DOUBLE_CLICK_TO_INSTALL.bat' first
    echo.
    pause
    exit /b 1
)

REM Launch the game
echo 🐉 Starting Mahjong Loong...
echo 🎮 Have fun playing!
echo.

start MahjongLoong.exe
