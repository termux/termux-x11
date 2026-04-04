# Termux:X11 Project Overview

## Project Summary

**Termux:X11** is a fully-featured X11 server application built with the Android NDK and optimized for use with [Termux](https://termux.com). It allows running graphical Linux applications on Android devices.

### Architecture
- **Android App** (`app/`): Main X11 server application with UI, input handling, and preferences
- **Shell Loader** (`shell-loader/`): Standalone loader for launching the X server from Termux shell
- **Native Libraries** (`app/src/main/cpp/`): X11 server components built from X.Org source using CMake
- **Companion Package**: Termux package (`termux-x11-nightly`) providing shell scripts and loader

### Key Technologies
- **Build System**: Gradle (Android), CMake (native)
- **Languages**: Java, Kotlin, C/C++
- **Target**: Android API 26+, Compile SDK 34
- **License**: GPLv3

## Repository Structure

```
termux-x11/
├── app/                          # Main Android application
│   ├── src/main/
│   │   ├── java/com/termux/x11/ # Java source code
│   │   ├── cpp/                  # Native C/C++ X11 server code
│   │   ├── res/                  # Android resources
│   │   └── AndroidManifest.xml
│   └── build.gradle
├── shell-loader/                 # Standalone shell loader
│   ├── src/main/java/
│   └── build.gradle
├── termux-x11-preference         # Preference management script
├── build_termux_package          # Script to build .deb and pacman packages
├── build.gradle                  # Root build configuration
└── settings.gradle               # Gradle settings
```

## Building and Running

### Prerequisites
- Android Studio / Gradle
- JDK 17
- Android NDK
- Git submodules initialized

### Setup
```bash
# Clone with submodules
git clone --recurse-submodules https://github.com/termux/termux-x11

# Or initialize submodules after cloning
git submodule update --init --recursive
```

### Build Commands

**Build Android APK:**
```bash
./gradlew assembleDebug
```
Output: `app/build/outputs/apk/debug/`

**Build Companion Package:**
```bash
./build_termux_package
```
Output: `.deb` and `.pkg.tar.xz` in `app/build/outputs/apk/debug/`

**Clean Build:**
```bash
./gradlew clean
```

### Native Library Build
The native X11 server is built automatically via CMake during Gradle build:
- CMake config: `app/src/main/cpp/CMakeLists.txt`
- Recipes: `app/src/main/cpp/recipes/*.cmake`
- Submodules: X.Org libraries (libx11, pixman, xserver, etc.)

## Source Code Organization

### Java Packages

**`com.termux.x11`** - Main application:
- `CmdEntryPoint.java` - Shell entry point, native library loader
- `MainActivity.java` - Main Android activity
- `LorieView.java` - X11 display view
- `LoriePreferences.java` - Preferences UI and broadcast receiver
- `input/` - Touch/mouse input handling
- `utils/` - Utilities (keyboard, fullscreen, etc.)

**`com.termux.shared.termux.extrakeys`** - Extra keys bar implementation

**`com.termux.x11.shell_loader`**:
- `Loader.java` - APK loader with signature verification

### Native Components (`app/src/main/cpp/`)
- `lorie/` - Custom X11 driver
- `xserver/` - X.Org server (submodule)
- `libx11/`, `libxfont/`, etc. - X11 libraries (submodules)
- `pixman/` - Pixel manipulation library
- `recipes/` - CMake build recipes for each component

## Configuration and Preferences

Preferences are defined in `app/src/main/res/xml/preferences.xml` and auto-generated into `Prefs.java` during build.

**Key preference categories:**
- **Output**: Display resolution, scaling, fullscreen, orientation
- **Pointer**: Touch mode, tap-to-move, pointer capture
- **Keyboard**: Extra keys, scancode handling, accessibility
- **Other**: Clipboard, notifications, secondary display settings

### Modifying Preferences Programmatically
Use `termux-x11-preference` script:
```bash
termux-x11-preference list                    # Dump current prefs
termux-x11-preference "fullscreen"="false"   # Set preference
```

## Usage

### Starting X11 Session
```bash
# With xstartup
termux-x11 :1 -xstartup "dbus-launch --exit-with-session xfce4-session"

# Or separate commands
termux-x11 :1 &
env DISPLAY=:1 dbus-launch --exit-with-session xfce4-session
```

### Common Options
- `-legacy-drawing` - Fix black screen issues on some devices
- `-force-bgra` - Fix color swapping issues
- `-dpi <value>` - Set DPI (e.g., `-dpi 120`)

### Stopping
```bash
# Kill X server process
pkill termux-x11

# Close Android activity
am broadcast -a com.termux.x11.ACTION_STOP -p com.termux.x11
```

## Development Notes

### Code Generation
The `generatePrefs` Gradle task parses `preferences.xml` and generates `Prefs.java` in `app/build/generated/java/`.

### Signing
Debug builds use the included keystore `app/testkey_untrusted.jks`.

### Submodules
The project uses 16+ git submodules for X.Org components. All are marked with `ignore = dirty` to allow local modifications.

### Debug Logging
Set `TERMUX_X11_DEBUG=1` environment variable for verbose logging.

## CI/CD

GitHub Actions workflow (`.github/workflows/debug_build.yml`):
- Builds APKs for all ABIs (arm64-v8a, armeabi-v7a, x86_64, x86, universal)
- Builds companion packages (.deb, .pkg.tar.xz)
- Uploads artifacts for nightly releases

## Key Files Reference

| File | Purpose |
|------|---------|
| `app/build.gradle` | Android app build config, version, signing |
| `shell-loader/build.gradle` | Loader build config with signature embedding |
| `app/src/main/cpp/CMakeLists.txt` | Native build configuration |
| `app/src/main/AndroidManifest.xml` | App permissions, activities, services |
| `build_termux_package` | Package builder script |
| `termux-x11` | Main shell command script |
| `termux-x11-preference` | Preference management script |
