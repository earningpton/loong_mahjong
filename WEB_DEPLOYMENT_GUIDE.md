# 🌐 Web Deployment Guide - Mahjong Loong

## 🚀 Quick Setup (Automatic)

### Step 1: Push to GitHub
```bash
git add .
git commit -m "Add web build files"
git push origin main
```

### Step 2: Enable GitHub Pages
1. Go to your repository on GitHub
2. Click **Settings** tab
3. Scroll down to **Pages** section
4. Under **Source**, select **GitHub Actions**
5. The build will start automatically

### Step 3: Access Your Game
Your game will be available at:
- **Landing page**: `https://yourusername.github.io/yourrepository`
- **Direct game**: `https://yourusername.github.io/yourrepository/mahjong_loong.html`

## 🔧 Manual Setup (Local Build)

### Prerequisites
1. **Emscripten SDK** - Install from [emscripten.org](https://emscripten.org/docs/getting_started/downloads.html)

### Installation Steps
```bash
# 1. Install Emscripten (one-time setup)
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 2. Navigate to your game directory
cd /path/to/your/mahjong-loong

# 3. Build the web version
chmod +x build_web.sh
./build_web.sh

# 4. Test locally
python -m http.server 8000
# Open: http://localhost:8000/mahjong_loong.html
```

### Build Output
The build process creates these files:
- `mahjong_loong.html` - Main game page
- `mahjong_loong.js` - JavaScript loader
- `mahjong_loong.wasm` - WebAssembly binary
- `mahjong_loong.data` - Packed game assets

## 📁 File Structure for Web

```
your-repository/
├── index.html              # Landing page
├── mahjong_loong.html      # Game page (generated)
├── mahjong_loong.js        # JS loader (generated)
├── mahjong_loong.wasm      # WebAssembly (generated)
├── mahjong_loong.data      # Assets (generated)
├── main.cpp                # Source code
├── Graphics/               # Game graphics
├── Sounds/                 # Game audio
├── build_web.sh           # Build script
├── web_shell.html         # HTML template
├── Makefile.web           # Alternative build
└── .github/workflows/     # Auto-build setup
    └── build-web.yml
```

## 🎯 Customization

### Update Landing Page
Edit `index.html` to customize:
- Game title and description
- Styling and colors
- Download links
- Instructions

### Modify Game Page
Edit `web_shell.html` to change:
- Loading messages
- Progress indicators
- Game container styling
- Control instructions

### Build Settings
Edit `build_web.sh` to adjust:
- Memory allocation
- Optimization level
- Asset preloading
- Emscripten flags

## 🐛 Troubleshooting

### Build Fails
```bash
# Check Emscripten installation
emcc --version

# Verify source files
ls -la main.cpp Graphics/ Sounds/

# Clean and rebuild
rm -f mahjong_loong.*
./build_web.sh
```

### Game Won't Load
1. **Check browser console** for error messages
2. **Verify file sizes** - WASM file should be several MB
3. **Test locally** before deploying
4. **Check HTTPS** - Some features require secure context

### GitHub Pages Issues
1. **Check Actions tab** for build status
2. **Verify Pages settings** in repository
3. **Wait 5-10 minutes** for deployment
4. **Clear browser cache** if changes don't appear

## 🎮 Performance Tips

### Optimize Build
```bash
# Smaller file size (slower loading)
emcc main.cpp -O3 -s TOTAL_MEMORY=67108864

# Faster loading (larger files)
emcc main.cpp -O2 -s ALLOW_MEMORY_GROWTH=1
```

### Asset Optimization
- **Compress audio files** to reduce download size
- **Optimize images** for web (PNG/WebP)
- **Remove unused assets** from Graphics/Sounds folders

## 🌟 Going Live

### Final Checklist
- ✅ Game builds without errors
- ✅ All assets load correctly
- ✅ Controls work in browser
- ✅ Audio plays properly
- ✅ Game saves progress
- ✅ Mobile-friendly (responsive)

### Share Your Game
Once deployed, share these links:
- **Main page**: `https://yourusername.github.io/yourrepository`
- **Direct play**: `https://yourusername.github.io/yourrepository/mahjong_loong.html`

## 🎉 Success!

Your Mahjong Loong game is now playable in any modern web browser! Players can enjoy the full dragon-themed Mahjong experience without downloading anything.
