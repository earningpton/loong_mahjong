#!/bin/bash

# Mahjong Loong - Permission Fix Script
# This script fixes permission issues for files downloaded from WeChat/Messenger

echo "🐉 Mahjong Loong - Permission Fix Tool"
echo "🔧 This will fix permission issues from WeChat/Messenger downloads"
echo ""

# Get the directory where this script is located
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$DIR"

echo "📁 Working in directory: $DIR"
echo ""

# Fix permissions for all command files
echo "🔐 Setting executable permissions..."
chmod +x *.command
chmod +x build_mac.sh

# Verify permissions were set
echo "✅ Checking permissions..."
ls -la *.command build_mac.sh

echo ""
echo "🎉 Permissions fixed!"
echo "📋 Next steps:"
echo "   1. Close this window"
echo "   2. Double-click 'DOUBLE_CLICK_TO_INSTALL.command'"
echo "   3. Or run './build_mac.sh' in Terminal"
echo ""
echo "Press any key to close this window..."
read -n 1
