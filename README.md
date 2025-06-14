# 🐉 Mahjong Loong 

## 🪟 Windows Distribution

**Compilation Command:**
```bash
g++ -std=c++17 -IC:/raylib/raylib/src main.cpp -LC:/raylib/raylib/src -lraylib -lgdi32 -lwinmm -O2 -s -o MahjongLoong_Windows/MahjongLoong.exe
```
**What's included:**
- `MahjongLoong_Windows/` folder with complete game
- Pre-compiled `MahjongLoong.exe` 
- All game assets (Graphics, Sounds)
- Installation scripts for easy setup
- Source code (`main.cpp`)

## Mac Distribution
1. Copy `MahjongLoong_Mac` folder to Desktop
2. Open Terminal
3. Type `cd ` then drag folder into Terminal
4. Run: `chmod +x *.command build_mac.sh`
5. Run: `./build_mac.sh`

**For Windows:**
1. Zip the `MahjongLoong_Windows` folder
2. Send to Windows users
3. They double-click `DOUBLE_CLICK_TO_INSTALL.bat`

**For Mac:**
1. Zip the `MahjongLoong_Mac` folder  
2. Send to Mac users
3. They follow instructions in `INSTALL_ME.txt`
4. If issues, they use `FIX_PERMISSIONS.command`