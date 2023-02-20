
# Termux:X11

[![Nightly build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml/badge.svg?branch=master)](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) [![Join the chat at https://gitter.im/termux/termux](https://badges.gitter.im/termux/termux.svg)](https://gitter.im/termux/termux) [![Join the Termux discord server](https://img.shields.io/discord/641256914684084234?label=&logo=discord&logoColor=ffffff&color=5865F2)](https://discord.gg/HXpF69X)

A [Termux](https://termux.com) add-on app providing Android frontend for Xwayland.

## About
Termux:X11 uses [Wayland](https://wayland.freedesktop.org/) display protocol. a modern replacement and the predecessor of the [X.org](https://www.x.org/wiki) server.
Pay attention that it is not a full-fledged Wayland server and it can not handle Wayland apps except Xwayland.

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
The Termux:X11 app's companion package executable creates socket through `$XDG_RUNTIME_DIR` in Termux directory by default.

The wayland sockets is the way for the graphical applications to communicate with. Termux X11 applications do not have wayland support yet, this kind of setup may not be straightforward and therefore additional packages should be installed in order for X11 applications to be run in Termux:X11

## Setup Instructions
For this one you must enable the `x11-repo` repository can be done by executing `pkg install x11-repo` command

For X applications to work, you must install Termux-x11 companion package. You can do that by downloading an artifact from [last successful build](https://github.com/termux/termux-x11/actions/workflows/debug_build.yml) and installing `*.apk` and `*.deb` files.

## Running Graphical Applications
to work with GUI applications, start Termux:X11 first. a toast message saying `Service was Created` indicates that it should be ready to use

then you can start your desired graphical application by doing:
```
~ $ export XDG_RUNTIME_DIR=${TMPDIR}
~ $ termux-x11 :1 &
~ $ env DISPLAY=:1 xfce4-session
```
You may replace `xfce4-session` if you use other than Xfce

If you're done using Termux:X11 just simply exit it through it's notification drawer by expanding the Termux:X11 notification then "Exit"

## Font or scaling is too big!
Some apps may have issues with wayland regarding DPI. please see https://wiki.archlinux.org/title/HiDPI on how to override application-specific DPI or scaling.

You can fix this in your window manager settings (in the case of xfce4 and lxqt via Applications Menu > Settings > Appearance). Look for the DPI value, if it is disabled enable it and adjust its value until the fonts are the appropriate size.
<details>
<summary> Screenshot </summary>

![image](./img/dpi-scale.png) 
</details>

## Using with 3rd party apps
It is possible to use Termux:X11 with 3rd party apps.
You should start Termux:X11's activity with providing some additional data.

<details>
<summary> Code fragment </summary>

```
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import com.termux.x11.common.ITermuxX11Internal;
...
private final String TermuxX11ComponentName = "com.termux.x11/.TermuxX11StarterReceiver";

private void startTermuxX11() {
    Service svc = new Service();
    Bundle bundle = new Bundle();
    bundle.putBinder("", svc);

    Intent intent = new Intent();
    intent.putExtra("com.termux.x11.starter", bundle);
    ComponentName cn = ComponentName.unflattenFromString(TermuxX11ComponentName);
    if (cn == null)
        throw new IllegalArgumentException("Bad component name: " + TermuxX11ComponentName);

    intent.setComponent(cn);

    intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|
                Intent.FLAG_ACTIVITY_SINGLE_TOP);
}

class Service extends ITermuxX11Internal.Stub {
    @Override
    public ParcelFileDescriptor getWaylandFD() throws RemoteException {
        /*
         * nativeObtainWaylandFd() should create wayland-0
         * socket in your $XDG_RUNTIME_DIR and return it's
         * fd. You should not "listen()" this socket.
         */
        int fd = nativeObtainWaylandFd();
        return ParcelFileDescriptor.adoptFd(fd);
    }

    @Override
    public ParcelFileDescriptor getLogFD() throws RemoteException {
        /*
         * nativeObtainLogFd() should create file that should
         * contain log. Pay attention that if you choose tty/pty
         * or fifo file Android will not allow writing it.
         * You can use `pipe` system call to create pipe.
         * Do not forget to change it's mode with `fchmod`.
         */
        int fd = nativeObtainLogFd();
        return ParcelFileDescriptor.adoptFd(fd);
    }

    @Override
    public void finish() throws RemoteException {
        /*
         * Termux:X11 cals this function to to notify calling
         * process that init stage was completed
         * successfully.
         */
    }
}
```
</details>

# License
Released under the [GPLv3 license](https://www.gnu.org/licenses/gpl-3.0.html).
