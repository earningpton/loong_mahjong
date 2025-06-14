#!/bin/bash

# Mahjong Loong - Simple Web Build Script
# Uses Raylib's web template for easier compilation

echo "🐉 Mahjong Loong - Simple Web Build"
echo "===================================="
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
    exit 1
fi

echo "✅ Emscripten found: $(emcc --version | head -n1)"
echo ""

# Check if source file exists
if [ ! -f "main.cpp" ]; then
    echo "❌ main.cpp not found!"
    exit 1
fi

# Clean previous build
echo "🧹 Cleaning previous build..."
rm -f game.* index.html

# Create a simple web build using Raylib's web template
echo "📦 Building with Raylib web template..."

# Use Raylib's web compilation approach
emcc main.cpp \
    -o game.html \
    -Wall \
    -std=c++17 \
    -D_DEFAULT_SOURCE \
    -Wno-missing-braces \
    -Wunused-result \
    -Os \
    -I. \
    -I./raylib/src \
    -I./raylib/src/external \
    -L. \
    -L./raylib/src \
    -s USE_GLFW=3 \
    -s ASYNCIFY \
    -s TOTAL_MEMORY=67108864 \
    -s FORCE_FILESYSTEM=1 \
    -DPLATFORM_WEB \
    --preload-file Graphics \
    --preload-file Sounds \
    --shell-file minshell.html

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Simple web build successful!"
    echo ""
    echo "📁 Generated files:"
    echo "   - game.html (Game page)"
    echo "   - game.js (JavaScript loader)"
    echo "   - game.wasm (WebAssembly binary)"
    echo "   - game.data (Game assets)"
    echo ""
    echo "🌐 To test locally:"
    echo "   python -m http.server 8000"
    echo "   Then open: http://localhost:8000/game.html"
    echo ""
else
    echo ""
    echo "❌ Simple build failed! Trying alternative approach..."
    echo ""
    
    # Alternative: Try without Raylib linking (if Raylib source is embedded)
    echo "🔄 Trying alternative build without external Raylib..."
    
    emcc main.cpp \
        -o game.html \
        -std=c++17 \
        -Os \
        -s USE_GLFW=3 \
        -s ASYNCIFY \
        -s TOTAL_MEMORY=67108864 \
        -s FORCE_FILESYSTEM=1 \
        -DPLATFORM_WEB \
        --preload-file Graphics \
        --preload-file Sounds
    
    if [ $? -eq 0 ]; then
        echo "✅ Alternative build successful!"
    else
        echo "❌ All build attempts failed!"
        echo ""
        echo "💡 This might be because:"
        echo "   - Raylib headers are not found"
        echo "   - The code needs modification for web"
        echo "   - Missing dependencies"
        echo ""
        echo "🔧 Try the full Raylib setup with build_web.sh instead"
        exit 1
    fi
fi
