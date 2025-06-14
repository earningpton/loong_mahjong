# 🐉 Mahjong Loong - Mac Troubleshooting Guide

## 🚨 COMMON ERROR: "permission denied"

**If you see this error:**
```
zsh: permission denied: /Users/chenchen/Library/Containers/com.tencent.xinWeChat/...
```

**This means:**
- You downloaded the game from WeChat/QQ/Messenger
- The files are in a restricted location
- The files don't have execute permissions

## ✅ SOLUTION (Step by Step):

### Step 1: Move the files to a better location
1. **Find the MahjongLoong_Mac folder** in Finder
2. **Copy it** to your Desktop or Documents folder
3. **Don't try to run it from the WeChat download location**

### Step 2: Open Terminal
1. Press `Cmd + Space`
2. Type "Terminal"
3. Press Enter

### Step 3: Navigate to the game folder
1. In Terminal, type: `cd ` (with a space after cd)
2. **Drag the MahjongLoong_Mac folder** from Finder into Terminal
3. Press Enter
4. You should see something like: `cd /Users/yourname/Desktop/MahjongLoong_Mac`

### Step 4: Fix permissions
```bash
chmod +x *.command
chmod +x build_mac.sh
```

### Step 5: Install and run
```bash
./build_mac.sh
```

## 🔧 Alternative: Use the Permission Fix Tool

1. **Double-click** `FIX_PERMISSIONS.command`
2. If that doesn't work, run it in Terminal:
   ```bash
   ./FIX_PERMISSIONS.command
   ```

## 🎯 Quick Copy-Paste Commands

If you're comfortable with Terminal, just copy and paste these commands:

```bash
# Navigate to Desktop (adjust path if needed)
cd ~/Desktop/MahjongLoong_Mac

# Fix permissions
chmod +x *.command build_mac.sh

# Install the game
./build_mac.sh

# Play the game
./MahjongLoong
```

## ❓ Still Having Issues?

1. **Make sure you have internet connection** (needed for installation)
2. **Make sure you have macOS 10.12 or later**
3. **If you get "g++ command not found":**
   ```bash
   xcode-select --install
   ```
4. **If you get "brew command not found":**
   - Install Homebrew from https://brew.sh
   - Or let the installer do it automatically

## 📧 Need Help?

Send a screenshot of your Terminal window showing the error to your partner.
Include the full error message and the commands you tried.
