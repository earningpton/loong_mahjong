#!/bin/bash

# Mahjong Loong - Game Launcher
# Double-click this file to play the game!

# Get the directory where this script is located
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$DIR"

echo "🐉 Mahjong Loong Game Launcher"
echo "📁 Game directory: $DIR"
echo ""

# Check if the game exists
if [ ! -f "MahjongLoong" ]; then
    echo "❌ Game executable not found!"
    echo "🔧 Please run 'DOUBLE_CLICK_TO_INSTALL.command' first to compile the game"
    echo ""
    echo "Press any key to close this window..."
    read -n 1
    exit 1
fi

# Check if the game is executable
if [ ! -x "MahjongLoong" ]; then
    echo "⚠️ Game found but not executable. Fixing permissions..."
    chmod +x MahjongLoong
    if [ ! -x "MahjongLoong" ]; then
        echo "❌ Failed to set executable permissions!"
        echo "💡 Try running: chmod +x MahjongLoong"
        echo ""
        echo "Press any key to close this window..."
        read -n 1
        exit 1
    fi
    echo "✅ Permissions fixed!"
fi

# Launch the game
echo "🐉 Starting Mahjong Loong..."
echo "🎮 Have fun playing!"
echo ""

./MahjongLoong

# Keep terminal open if game exits with error
if [ $? -ne 0 ]; then
    echo ""
    echo "❌ Game exited with an error"
    echo "Press any key to close this window..."
    read -n 1
fi
