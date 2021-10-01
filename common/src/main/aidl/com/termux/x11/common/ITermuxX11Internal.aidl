package com.termux.x11.common;

// This interface is used by utility on termux side.
interface ITermuxX11Internal {
    ParcelFileDescriptor getWaylandFD();
    ParcelFileDescriptor getLogFD();
    void finish();
}