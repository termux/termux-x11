
# Termux:X11

[![Nightly build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml/badge.svg?branch=master)](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) [![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux) [![Join the Termux discord server](https://img.shields.io/discord/641256914684084234?label=&logo=discord&logoColor=ffffff&color=5865F2)](https://discord.gg/HXpF69X)

A [Termux](https://termux.com) add-on app providing Android frontend for Xwayland.

## About
Termux:X11 uses [Wayland](https://wayland.freedesktop.org/) display protocol. a modern replacement and the predecessor of the [X.org](https://www.x.org/wiki) server.
Pay attention that it is not a full-fledged Wayland server and it can not handle Wayland apps except Xwayland.

## Caveat
This repo uses submodules. Use 

```
~ $ git clone --recurse-submodules https://github.com/termux/termux-x11 
```
or
```
~ $ git clone https://github.com/termux/termux-x11
~ $ cd termux-x11
~ $ git submodule update --init --recursive
```

## How does it work?
The Termux:X11 app's companion package executable creates socket through `$XDG_RUNTIME_DIR` in Termux directory by default.

The wayland sockets is the way for the graphical applications to communicate with. Termux X11 applications do not have wayland support yet, this kind of setup may not be straightforward and therefore additional packages should be installed in order for X11 applications to be run in Termux:X11

## Setup Instructions
For this one you must enable the `x11-repo` repository can be done by executing `pkg install x11-repo` command

For X applications to work, you must install Termux-x11 companion package. You can do that by downloading an artifact from [last successful build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) and installing `*.apk` and `*.deb` (if you use termux with `pkg`) or `*.tar.xz` (if you use termux with `pacman`) files.

## Running Graphical Applications
to work with GUI applications, start Termux:X11 first. a toast message saying `Service was Created` indicates that it should be ready to use

then you can start your desired graphical application by doing:
```
~ $ XDG_RUNTIME_DIR=${TMPDIR} termux-x11 :1 &
~ $ env DISPLAY=:1 dbus-launch --exit-with-session xfce4-session
```
You may replace `xfce4-session` if you use other than Xfce

If you're done using Termux:X11 just simply exit it through it's notification drawer by expanding the Termux:X11 notification then "Exit"

In Android 13 post notifications was restricted so you should explicitly let Termux:X11 show you notifications.
<details>
<summary>Video</summary>

![video](./img/enable-notifications.webm)
</details>

Preferences:
You can access preferences menu three ways:
<details>
<summary>By clicking "PREFERENCES" button on main screen when no client connected.</summary>

![image](./img/1.jpg)
</details>
<details>
<summary>By clicking "Preferences" button in notification, if available.</summary>

![image](./img/2.jpg)
</details>
<details>
<summary>By clicking "Preferences" application shortcut (long tap `Termux:X11` icon in launcher). </summary>

![image](./img/3.jpg)
</details>

## Touch gestures
### Touchpad emulation mode.
In touchpad emulation mode you can use the following gestures:
* Tap for click
* Double tap for double click
* Two-finger tap for right click
* Three-finger tap for middle click
* Two-finger vertical swipe for vertical scroll
* Two-finger horizontal swipe for horizontal scroll
* Three-finger swipe down to show-hide additional keys bar.
### Mouse emulation mode.
In touchpad emulation mode you can use the following gestures:
* Mouse is in click mode as long as you hold finger on a screen.
* Double tap for double click
* Two-finger tap for right click
* Three-finger tap for middle click
* Two-finger vertical swipe for vertical scroll
* Two-finger horizontal swipe for horizontal scroll
* Three-finger swipe down to show-hide additional keys bar.

## Font or scaling is too big!
Some apps may have issues with wayland regarding DPI. please see https://wiki.archlinux.org/title/HiDPI on how to override application-specific DPI or scaling.

You can fix this in your window manager settings (in the case of xfce4 and lxqt via Applications Menu > Settings > Appearance). Look for the DPI value, if it is disabled enable it and adjust its value until the fonts are the appropriate size.
<details>
<summary> Screenshot </summary>

![image](./img/dpi-scale.png) 
</details>

## Using with 3rd party apps
It is possible to use Termux:X11 with 3rd party apps.
Check how `shell-loader/src/main/java/com/termux/x11/Loader.java` works.

# License
Released under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).
