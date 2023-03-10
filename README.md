
# Termux:X11

[![Nightly build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml/badge.svg?branch=master)](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) [![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux) [![Join the Termux discord server](https://img.shields.io/discord/641256914684084234?label=&logo=discord&logoColor=ffffff&color=5865F2)](https://discord.gg/HXpF69X)

A [Termux](https://termux.com) add-on app providing Android frontend for Xvfb.

## About
Termux:X11 uses [XCB](https://xcb.freedesktop.org/) display protocol.

## Caveat
This repo uses submodules. Use 

```
    git clone --recurse-submodules https://github.com/termux/termux-x11 
```
or
```
    git clone https://github.com/termux/termux-x11
    git submodule update --init --recursive
```

## How does it work?
The Termux:X11 app's companion package executable connects socket in `$TMPDIR/.X11-unix` in Termux directory by default.

The X11 sockets is the way for the graphical applications to communicate with. This kind of setup may not be straightforward and therefore additional packages should be installed in order for X11 applications to be run in Termux:X11

## Setup Instructions
For this one you must enable the `x11-repo` repository can be done by executing `pkg install x11-repo` command

For X applications to work, you must install Termux-x11 companion package. You can do that by downloading an artifact from [last successful build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) and installing `*.apk` and `*.deb` (if you use termux with `pkg`) or `*.tar.xz` (if you use termux with `pacman`) files.

## Running Graphical Applications
`termux-x11` does not accept arguments.

Like any other X11 program, this program requires that the `DISPLAY` environment variable be set.
`DISPLAY=:0 termux-x11`
  or
`export DISPLAY=:0`
`termux-x11`

The program does not support Xauth, so the X server must be launched with the `-ac` option or
the `xhost +` command must be executed when starting the desktop environment.
`Xvfb :0 -ac -screen 0 4096x4096`

Program is designed to work only with `Xvfb`, but work with other X servers is also
possible but not stable.
You must start `Xvfb` with parameters `-screen 0 4096x4096x32` in the case if you want changing
display resolution to be available and fully supported. In the case if you not pass it **maximal**
resolution will be set to `1280x1024`.

Proot:
If you plan to use the program with `proot`, keep in mind that you need to launch `proot`/`proot-distro`
with the `--shared-tmp` option. If passing this option is not possible, set the `TMPDIR` environment
variable to point to the directory that corresponds to /tmp in the target container.

Chroot:
If you plan to use the program with `chroot` or `unshare`, you need to run it as root and set the
`TMPDIR` environment variable to point to the directory that corresponds to /tmp in the target container.
This directory must be accessible from the shell from which you launch termux-x11,
i.e. it must be in the same SELinux context, same mount namespace, and so on.

Logs:
If you need to obtain logs from the `com.termux.x11` application,
set the `TERMUX_X11_DEBUG` environment variable to 1, like this:
`DISPLAY=:0 TERMUX_X11_DEBUG=1 termux-x11`

The log obtained in this way can be quite long.
It's better to redirect the output of the command to a file right away.

Caveat:
`termux-x11` does not launch the `Termux:X11` application and does not bring it to the foreground
as previous versions did. The program is designed to be run only once, at the device or
Termux environment startup.

To work with GUI applications, start Xvfb first.

then you can start your desired graphical application by doing:
```
~ $ Xvfb :1 -ac -screen 0 4096x4096x24 &
~ $ DISPLAY=:1 termux-x11 &
~ $ env DISPLAY=:1 dbus-launch --exit-with-session startxfce4
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

## Font or scaling is too big!
Some apps may have issues with Xvfb regarding DPI. please see https://wiki.archlinux.org/title/HiDPI on how to override application-specific DPI or scaling.

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
