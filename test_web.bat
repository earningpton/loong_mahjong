@echo off
title Mahjong Loong - Web Test Server

echo 🐉 Mahjong Loong - Web Test Server
echo ====================================
echo.

REM Check if Python is installed
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Python not found!
    echo 💡 Please install Python from https://python.org
    echo.
    pause
    exit /b 1
)

echo ✅ Python found
echo.

REM Check if web files exist
if not exist "mahjong_loong.js" (
    echo ⚠️  Web build files not found!
    echo 💡 You need to build the web version first
    echo.
    echo 🔧 To build the web version:
    echo    1. Install Emscripten SDK
    echo    2. Run: build_web.sh
    echo.
    echo 📋 Or use the automatic GitHub Actions build
    echo.
    pause
    exit /b 1
)

echo ✅ Web build files found
echo.

REM Start the server
echo 🌐 Starting local test server...
echo 📋 The game will open in your browser automatically
echo 🎯 Press Ctrl+C to stop the server
echo.

python serve.py

echo.
echo 🛑 Server stopped
pause
