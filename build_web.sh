#!/bin/bash

# Mahjong Loong - Web Build Script
# Compiles the game to WebAssembly for browser play

echo "🐉 Mahjong Loong - Web Build Script"
echo "======================================"
echo ""

# Check if Emscripten is installed
if ! command -v emcc &> /dev/null; then
    echo "❌ Emscripten not found!"
    echo ""
    echo "📦 Please install Emscripten first:"
    echo "   1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "   2. cd emsdk"
    echo "   3. ./emsdk install latest"
    echo "   4. ./emsdk activate latest"
    echo "   5. source ./emsdk_env.sh"
    echo ""
    echo "🌐 Official guide: https://emscripten.org/docs/getting_started/downloads.html"
    exit 1
fi

echo "✅ Emscripten found: $(emcc --version | head -n1)"
echo ""

# Check if source file exists
if [ ! -f "main.cpp" ]; then
    echo "❌ main.cpp not found!"
    echo "💡 Make sure you're in the project root directory"
    exit 1
fi

# Check if assets exist
if [ ! -d "Graphics" ] || [ ! -d "Sounds" ]; then
    echo "⚠️  Warning: Graphics or Sounds folder not found"
    echo "💡 The game may not work properly without assets"
fi

echo "🔨 Starting compilation..."
echo ""

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -f mahjong_loong.js mahjong_loong.wasm mahjong_loong.data mahjong_loong.html

# Setup Raylib for web if not already present
if [ ! -f "libraylib.a" ] || [ ! -d "raylib" ]; then
    echo "📦 Setting up Raylib for web..."

    # Clone Raylib if not present
    if [ ! -d "raylib" ]; then
        git clone https://github.com/raysan5/raylib.git
    fi

    # Build Raylib for web
    cd raylib/src
    make PLATFORM=PLATFORM_WEB -B

    # Copy the library
    cp libraylib.a ../../
    cd ../../

    echo "✅ Raylib for web ready!"
else
    echo "✅ Raylib already available"
fi

# Compile to WebAssembly
echo "📦 Compiling C++ to WebAssembly..."
emcc main.cpp \
    -std=c++17 \
    -O2 \
    -I./raylib/src \
    -L. \
    -lraylib \
    -s USE_GLFW=3 \
    -s ASYNCIFY \
    -s TOTAL_MEMORY=134217728 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s FORCE_FILESYSTEM=1 \
    -s ASSERTIONS=1 \
    -s LEGACY_GL_EMULATION=1 \
    --shell-file web_shell.html \
    --preload-file Graphics \
    --preload-file Sounds \
    -o mahjong_loong.js

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Web build successful!"
    echo ""
    echo "📁 Generated files:"
    echo "   - mahjong_loong.js (JavaScript loader)"
    echo "   - mahjong_loong.wasm (WebAssembly binary)"
    echo "   - mahjong_loong.data (Game assets)"
    echo "   - mahjong_loong.html (Game page)"
    echo ""
    echo "🌐 To test locally:"
    echo "   python -m http.server 8000"
    echo "   Then open: http://localhost:8000/mahjong_loong.html"
    echo ""
    echo "🚀 For GitHub Pages:"
    echo "   1. Commit and push all files to your repository"
    echo "   2. Enable GitHub Pages in repository settings"
    echo "   3. Your game will be available at:"
    echo "      https://yourusername.github.io/yourrepository/mahjong_loong.html"
    echo ""
    echo "📋 Files to commit:"
    echo "   - index.html (landing page)"
    echo "   - mahjong_loong.html (game page)"
    echo "   - mahjong_loong.js"
    echo "   - mahjong_loong.wasm"
    echo "   - mahjong_loong.data"
    echo "   - Graphics/ (folder)"
    echo "   - Sounds/ (folder)"
else
    echo ""
    echo "❌ Compilation failed!"
    echo ""
    echo "💡 Common solutions:"
    echo "   - Make sure Emscripten is properly installed and activated"
    echo "   - Check that main.cpp compiles on desktop first"
    echo "   - Ensure Graphics and Sounds folders exist"
    echo ""
    echo "🔧 Debug info:"
    echo "   - Emscripten version: $(emcc --version | head -n1)"
    echo "   - Current directory: $(pwd)"
    echo "   - Files present: $(ls -la main.cpp Graphics Sounds 2>/dev/null || echo 'Some files missing')"
    exit 1
fi
