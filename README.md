# Datebase Management System using C++, v1.0
g++ -std=c++17 main.cpp -o main  

# This is to build using cmake
mkdir build && cd build                 
cmake ..
make
# This is to create the final .dmg file
create-dmg \
  --volname "qiloDB Installer" \
  --volicon "assets/icon.icns" \
  --window-size 600 400 \
  --icon-size 100 \
  --background "assets/bg.png" \
  --app-drop-link 480 150 \
  --icon "minidb.app" 100 150 \
  --icon "Enable CLI Access.command" 100 280 \
  --icon "Eject Installer.command" 250 280 \
  final.dmg \
  dist/
# this is also important:
chmod +x dist/qiloDB.app/Contents/MacOS/launcher
chmod +x dist/Enable\ CLI\ Access.command
chmod +x dist/Eject\ Installer.command
# this is to code sign my app:
codesign --deep --sign "Your Developer ID" build/minidb.app

# notorize my app via apple:
	•	Notarize your app via Apple:
	•	You’ll need an Apple Developer ID.
	•	You can notarize via xcrun altool or Xcode.
	•	Let me know if you want a step-by-step for that.
  g++ -std=c++17 main.cpp -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto -o qiloDB