# Termux:X11

[![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux)

A [Termux](https://termux.com) add-on app providing Android frontend for Xwayland.

When developing (or packaging), note that this app needs to be signed with the same key as the main Termux app in order to have the permission to execute scripts.

## About
Termux:X11 uses [Wayland](https://wayland.freedesktop.org/) display protocol. a modern replacement and the predecessor of the [X.org](https://www.x.org/wiki) server

## How does it work?
the Termux:X11 app writes files through `$PREFIX/tmp` in Termux directory by default and creates wayland sockets through that directory, this takes advantage of `sharedUserId` AndroidManifest attribute

the wayland sockets is the way for the graphical applications to communicate with. Termux X11 applications do not have wayland support yet, this kind of setup may not be straightforward and therefore additional packages should be installed in order for X11 applications to be run in Termux:X11

## Setup Instructions
for this one. you must enable the `x11-repo` repository can be done by executing `pkg install x11-repo` command

for X applications to work, you must install `Xwayland` packages. you can do that by doing
```
pkg install xwayland
```

## Running Graphical Applications
to work with GUI applications, start Termux:X11 first. a toast message saying `Service was Created` indicates that it should be ready to use

then you can start your desired graphical application by doing:
```
~ $ export XDG_RUNTIME_DIR=${TMPDIR}
~ $ Xwayland :1 >/dev/null &
~ $ env DISPLAY=:1 xfce4-session
```
You may replace `xfce4-session` if you use other than Xfce

If you're done using Termux:X11 just simply exit it through it's notification drawer by expanding the Termux:X11 notification then "Exit"

## Font or scaling is too big!
Some apps may have issues with wayland regarding DPI. please see https://wiki.archlinux.org/title/HiDPI on how to override application-specific DPI or scaling.

You can fix this in your window manager settings (in the case of xfce4 and lxqt via Applications Menu > Settings > Appearance). Look for the DPI value, if it is disabled enable it and adjust its value until the fonts are the appropriate size.

![image](./img/dpi-scale.png) 

# License
Released under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).
