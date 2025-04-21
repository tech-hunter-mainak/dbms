**Database Management System using C++, v1.0**

**QiloDB** is a lightweight, terminal-based database management system developed in C++. It provides an intuitive command-line interface for managing databases, tables, records, and transactions in a modular and extensible codebase.

---

## Overview

QiloDB offers a robust set of features for working with bin-backed databases directly from your terminal. It is ideal for learning fundamental database concepts or embedding a lightweight storage engine within custom C++ applications.

---

## Features

- **Modular C++17 Codebase**: Easily extend and customize core components.
- **Database & Table Management**: Create, enter, erase databases; create, choose, and delete tables.
- **Data Manipulation**: Insert, update, delete, and filter records with expressive commands.
- **Query & Display**: Flexible `show` variations for head, tail, column selection, and conditional filters.
- **Transaction Control**: Support for `commit` and `rollback` to manage changes safely.
- **Formatted Output**: Clean, tabular display of schema and query results.

---

## Requirements

- **C++17** or later
- **CMake** (for production build)
- **OpenSSL 3** (optional, for SSL-enabled builds)

---

## Installation

### Production Build (CMake)

Build QiloDB using CMake for a production-ready executable and installer:

```bash
# Create and enter build directory
mkdir build && cd build

# Generate build files and compile
cmake ..
make
```

To create a macOS installer (.dmg):
```bash
create-dmg \
  --volname "qiloDB Installer" \
  --volicon "assets/icon.icns" \
  --window-size 600 400 \
  --icon-size 100 \
  --background "assets/bg.png" \
  --app-drop-link 480 150 \
  --icon "qilodb.app" 100 150 \
  --icon "Enable CLI Access.command" 100 280 \
  --icon "Eject Installer.command" 250 280 \
  final.dmg \
  dist/
```

### Development Build

Compile directly with g++ for quick iteration and testing:

```bash
# Standard build (no SSL)
g++ -std=c++17 main.cpp table.cpp database.cpp file_utils.cpp -o qilodb

# Build with SSL (windows only)
g++ -std=c++17 -static main.cpp -IC:\OpenSSL-Win64\include -LC:\OpenSSL-Win64\lib -lssl -lcrypto -lws2_32 -lgdi32 -lcrypt32 -luser32 -o qilodb

# SSL-enabled build (macOS Homebrew OpenSSL)
g++ -std=c++17 main.cpp -I/opt/homebrew/opt/openssl@3/include -L/opt/homebrew/opt/openssl@3/lib -lssl -lcrypto -o qilo
```  

---

## Platform-specific Packaging

### macOS Package Installer (Packages.app)

For a native macOS installer bundle, use [Packages.app](http://s.sudre.free.fr/Software/Packages/about.html):

1. **Download & Launch**: Install and open Packages.app.
2. **Create a New Project**: Select “Raw Package” and set the package identifier (e.g., `com.qilodb.installer`).
3. **Add Files**: Under the “Payload” section, drag your `qilodb.app` (built with CMake) into the target volume root.
4. **Scripts & Resources**: In “Resources,” include `assets/icon.icns` and `background.png` if desired. Optionally, add post-install scripts to create CLI symlinks:
   ```bash
   ln -s "/Applications/qilodb.app/Contents/MacOS/qilodb" /usr/local/bin/qilodb
   ```
5. **Build**: Choose the output directory and click “Build.” The result is a `.pkg` installer that users can double-click to install QiloDB with macOS’s native installer UI.

### Windows Installer (Inno Setup)

To package QiloDB on Windows, compile with Developer Command Prompt and bundle using Inno Setup:

1. **Open Developer Command Prompt**: Launch the Visual Studio Developer Command Prompt.
2. **Compile**:
   ```batch
   cl /EHsc main.cpp table.cpp database.cpp file_utils.cpp /link /OUT:qilodb.exe
   ```
3. **Install Inno Setup**: Download and install from [jrsoftware.org](https://jrsoftware.org).
4. **Create Script**: Save the following as `qilodb.iss`:
   ```ini
   [Setup]
   AppName=QiloDB
   AppVersion=1.0
   DefaultDirName={pf}\QiloDB
   DefaultGroupName=QiloDB
   OutputBaseFilename=qilodb_installer

   [Files]
   Source: "{#SourceExe}"; DestDir: "{app}"; Flags: ignoreversion

   [Icons]
   Name: "{group}\QiloDB"; Filename: "{app}\qilodb.exe"
   ```
5. **Compile Script**: In Inno Setup Compiler, open `qilodb.iss`, set `SourceExe=..\qilodb.exe`, and click “Compile.”
6. **Result**: An `qilodb_installer.exe` ready to distribute, which installs QiloDB under Program Files and creates Start Menu shortcuts.

---

## Getting Started & Documentation

For detailed command usage, examples, and troubleshooting, please visit our online guide:

**https://qiloDB.vercel.app**

This site provides up-to-date instructions, use-case walkthroughs, and an interactive command reference.

---

## Support & Contributions

We welcome feedback, bug reports, and pull requests:

- **Issues**: Submit feature requests or bug reports on the GitHub repository.
- **Pull Requests**: Fork the project, commit your changes, and open a PR—please include clear documentation for any additions.
- **Community**: Join the discussions for design proposals or enhancements.

---

## License

Not licensed yet.