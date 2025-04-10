# Datebase Management System using C++, v1.0
g++ -std=c++17 main.cpp -o main  

# This is to build using cmake
 mkdir build && cd build                 
cmake ..
make
# This is to create the final .dmg file
create-dmg output.dmg build/minidb.app \
  --overwrite \
  --dmg-title 'MiniDB Installer' \
  --app-drop-link \
  --icon-size 100 \
  --window-size 500 300
# this is to code sign my app:
codesign --deep --sign "Your Developer ID" build/minidb.app
# notorize my app via apple:
	•	Notarize your app via Apple:
	•	You’ll need an Apple Developer ID.
	•	You can notarize via xcrun altool or Xcode.
	•	Let me know if you want a step-by-step for that.