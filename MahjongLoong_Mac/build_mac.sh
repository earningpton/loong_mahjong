#!/bin/bash

# Mahjong Loong - Mac Build Script
# This script compiles the game for macOS

echo "🐉 Building Mahjong Loong for macOS..."
echo "📁 Current directory: $(pwd)"

# Check if we're in a WeChat/Messenger download path
if [[ "$PWD" == *"WeChat"* ]] || [[ "$PWD" == *"QQ"* ]] || [[ "$PWD" == *"MessageTemp"* ]]; then
    echo "⚠️  WARNING: You're running from a messenger download location!"
    echo "💡 For best results, copy this folder to Desktop or Documents first"
    echo "📋 Current path: $PWD"
    echo ""
fi

echo "📋 Files in directory:"
ls -la

# Check if main.cpp exists
if [ ! -f "main.cpp" ]; then
    echo "❌ main.cpp not found in current directory!"
    echo "💡 Make sure you're in the MahjongLoong_Mac folder"
    echo "💡 Try: cd MahjongLoong_Mac"
    exit 1
fi

# Check if Raylib is installed
if ! brew list raylib &>/dev/null; then
    echo "❌ Raylib not found. Installing via Homebrew..."
    echo "⏳ This may take a few minutes..."
    brew install raylib
    if [ $? -ne 0 ]; then
        echo "❌ Failed to install Raylib via Homebrew"
        echo "💡 Try installing manually: brew install raylib"
        exit 1
    fi
fi

# Remove old executable if it exists
if [ -f "MahjongLoong" ]; then
    echo "🗑️ Removing old executable..."
    rm MahjongLoong
fi

# Compile the game
echo "🔨 Compiling..."
g++ -std=c++17 main.cpp \
    -lraylib \
    -framework OpenGL \
    -framework Cocoa \
    -framework IOKit \
    -framework CoreVideo \
    -framework CoreAudio \
    -O2 \
    -o MahjongLoong

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "✅ Compilation successful!"
    echo "🎮 Executable created: MahjongLoong"

    # Make executable with proper permissions
    chmod +x MahjongLoong
    echo "🔐 Set executable permissions"

    # Verify the executable exists and has correct permissions
    if [ -x "MahjongLoong" ]; then
        echo "✅ Executable verified and ready!"
        echo "🚀 To run the game: ./MahjongLoong"
        echo "📁 Or double-click PLAY_GAME.command"
    else
        echo "⚠️ Executable created but permissions may be incorrect"
        echo "💡 Try: chmod +x MahjongLoong"
    fi
else
    echo "❌ Compilation failed!"
    echo "💡 Make sure you have Xcode command line tools installed:"
    echo "💡 xcode-select --install"
    exit 1
fi
