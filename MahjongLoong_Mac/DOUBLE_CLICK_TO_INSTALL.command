#!/bin/bash

# Mahjong Loong - Double-Click Installer for Mac
# This script can be double-clicked in Finder to install the game

echo "🐉 Welcome to Mahjong Loong!"
echo "🔧 Installing the game for you..."
echo ""

# Get the directory where this script is located
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$DIR"

echo "📁 Installation directory: $DIR"
echo "📋 Files in directory:"
ls -la
echo ""

# Check if main.cpp exists
if [ ! -f "main.cpp" ]; then
    echo "❌ main.cpp not found!"
    echo "💡 Make sure all game files are in the same folder as this installer"
    echo ""
    echo "Press any key to close this window..."
    read -n 1
    exit 1
fi

# Check if Xcode command line tools are installed
if ! command -v g++ &> /dev/null; then
    echo "🔧 Installing Xcode command line tools..."
    echo "⏳ This may take a few minutes and will open a dialog box"
    xcode-select --install
    echo "⏸️ Please complete the Xcode installation and run this script again"
    echo ""
    echo "Press any key to close this window..."
    read -n 1
    exit 1
fi

# Check if Homebrew is installed
if ! command -v brew &> /dev/null; then
    echo "📦 Installing Homebrew (this may take a few minutes)..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    # Add Homebrew to PATH for this session
    if [[ -f "/opt/homebrew/bin/brew" ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [[ -f "/usr/local/bin/brew" ]]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi

    # Verify Homebrew installation
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew installation failed!"
        echo "💡 Please install Homebrew manually from https://brew.sh"
        echo ""
        echo "Press any key to close this window..."
        read -n 1
        exit 1
    fi
fi

# Install Raylib
echo "🎮 Installing game engine (Raylib)..."
brew install raylib

if [ $? -ne 0 ]; then
    echo "❌ Failed to install Raylib!"
    echo "💡 Try running: brew install raylib"
    echo ""
    echo "Press any key to close this window..."
    read -n 1
    exit 1
fi

# Remove old executable if it exists
if [ -f "MahjongLoong" ]; then
    echo "🗑️ Removing old game executable..."
    rm MahjongLoong
fi

# Compile the game
echo "🔨 Building the game..."
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
    # Set executable permissions
    chmod +x MahjongLoong

    # Verify the executable was created and is executable
    if [ -x "MahjongLoong" ]; then
        echo ""
        echo "✅ SUCCESS! The game is ready!"
        echo "🎮 Double-click 'PLAY_GAME.command' to start playing!"
        echo "🎮 Or run: ./MahjongLoong"
        echo ""
        echo "Press any key to close this window..."
        read -n 1
    else
        echo "⚠️ Game compiled but permissions issue detected"
        echo "💡 Try running: chmod +x MahjongLoong"
        echo ""
        echo "Press any key to close this window..."
        read -n 1
    fi
else
    echo ""
    echo "❌ Compilation failed!"
    echo "💡 Common solutions:"
    echo "   • Make sure Xcode command line tools are installed: xcode-select --install"
    echo "   • Make sure Raylib is installed: brew install raylib"
    echo "   • Check that main.cpp exists in this folder"
    echo ""
    echo "📧 If problems persist, send a screenshot of this window to your partner."
    echo ""
    echo "Press any key to close this window..."
    read -n 1
    exit 1
fi
