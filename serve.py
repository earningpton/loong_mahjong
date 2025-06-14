#!/usr/bin/env python3
"""
Simple HTTP server for testing Mahjong Loong web version locally
"""

import http.server
import socketserver
import webbrowser
import os
import sys
from pathlib import Path

PORT = 8000

class CustomHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Add CORS headers for local testing
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        super().end_headers()

def main():
    print("🐉 Mahjong Loong - Local Test Server")
    print("=" * 40)
    
    # Check if web files exist
    web_files = ['mahjong_loong.js', 'mahjong_loong.wasm', 'mahjong_loong.data']
    missing_files = [f for f in web_files if not Path(f).exists()]
    
    if missing_files:
        print("⚠️  Warning: Some web build files are missing:")
        for file in missing_files:
            print(f"   - {file}")
        print("")
        print("💡 Run './build_web.sh' first to build the web version")
        print("")
    
    # Check if index.html exists
    if Path('index.html').exists():
        print("✅ Landing page found: index.html")
    else:
        print("⚠️  Landing page not found: index.html")
    
    # Check if game page exists
    if Path('mahjong_loong.html').exists():
        print("✅ Game page found: mahjong_loong.html")
    else:
        print("⚠️  Game page not found: mahjong_loong.html")
    
    print("")
    print(f"🌐 Starting server on port {PORT}...")
    
    try:
        with socketserver.TCPServer(("", PORT), CustomHTTPRequestHandler) as httpd:
            print(f"✅ Server running at: http://localhost:{PORT}")
            print("")
            print("📋 Available pages:")
            print(f"   🏠 Landing page: http://localhost:{PORT}")
            print(f"   🎮 Game page:    http://localhost:{PORT}/mahjong_loong.html")
            print("")
            print("🎯 Press Ctrl+C to stop the server")
            print("")
            
            # Try to open browser automatically
            try:
                webbrowser.open(f'http://localhost:{PORT}')
                print("🌐 Opening browser automatically...")
            except:
                print("💡 Manually open your browser to the URL above")
            
            print("")
            httpd.serve_forever()
            
    except KeyboardInterrupt:
        print("\n🛑 Server stopped by user")
    except OSError as e:
        if e.errno == 48:  # Address already in use
            print(f"❌ Port {PORT} is already in use!")
            print("💡 Try a different port or stop the other server")
        else:
            print(f"❌ Error starting server: {e}")
    except Exception as e:
        print(f"❌ Unexpected error: {e}")

if __name__ == "__main__":
    main()
