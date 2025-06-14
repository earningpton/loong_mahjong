# 🐉 Mahjong Loong

A mystical dragon-themed Mahjong Snake game where you choose your dragon, master ancient tiles, and ascend to immortality!

## 🌐 Play in Browser

**🎮 [PLAY NOW](https://earningpton.github.io/loong_mahjong/mahjong_loong.html)**

The game runs directly in your browser using WebAssembly - no downloads required!

### Game Features
- 🐉 **9 Mythological Dragons** - Each with unique powers and abilities
- 🎯 **5 Difficulty Levels** - From Foundation Building to Immortal Sage
- 🀄 **Advanced Mahjong System** - Complex tile matching and strategy
- 🎵 **Beautiful Audio** - Immersive music and sound effects
- 💾 **Progress Saving** - Your achievements are remembered
- 🖱️ **Mouse Controls** - Intuitive gameplay

## 🚀 Quick Start

### Browser Version (Recommended)
1. Visit the GitHub Pages link above
2. Wait for the game to load
3. Choose your dragon and start playing!

### Desktop Versions

#### 🪟 Windows
```bash
g++ -std=c++17 -IC:/raylib/raylib/src main.cpp -LC:/raylib/raylib/src -lraylib -lgdi32 -lwinmm -O2 -s -o MahjongLoong_Windows/MahjongLoong.exe
```

#### 🍎 Mac
1. Copy `MahjongLoong_Mac` folder to Desktop
2. Open Terminal: `cd ` then drag folder into Terminal
3. Run: `chmod +x *.command build_mac.sh`
4. Run: `./build_mac.sh`

## 🔧 Building the Web Version

### Automatic Build (GitHub Actions)
The web version is automatically built and deployed when you push to GitHub:
1. Push your code to GitHub
2. Enable GitHub Pages in repository settings
3. The game will be available at: `https://yourusername.github.io/yourrepository`

### Manual Build
If you want to build locally:

#### Prerequisites
- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html)

#### Build Steps
```bash
# Install Emscripten (one-time setup)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# Build the game
./build_web.sh

# Test locally
python -m http.server 8000
# Open: http://localhost:8000/mahjong_loong.html
```

## 💾 Save System

### Desktop Versions
- **Windows/Mac**: Saves to local files (`highscore.txt`, `progress.txt`)
- **Persistent**: Saves remain between game sessions
- **Fresh Start**: Each distribution starts clean for new players

### Web Version
- **Browser Storage**: Uses localStorage for persistent saves
- **Cross-Session**: Progress saved between browser sessions
- **Per-Browser**: Each browser maintains separate progress
- **Privacy-Friendly**: All data stays on user's device

**What Gets Saved:**
- 🏆 High scores for each dragon + difficulty combination
- 🔓 Unlocked dragons and difficulty levels
- 🎯 Overall best score

**Note**: Web saves persist until the user clears browser data or uses incognito mode.

## 📦 Distribution

**For Windows:**
1. Zip the `MahjongLoong_Windows` folder
2. Users double-click `DOUBLE_CLICK_TO_INSTALL.bat`

**For Mac:**
1. Zip the `MahjongLoong_Mac` folder
2. Users follow instructions in `INSTALL_ME.txt`
3. If issues, they use `FIX_PERMISSIONS.command`

**For Web:**
- Automatically deployed via GitHub Actions
- No installation required - just visit the URL!