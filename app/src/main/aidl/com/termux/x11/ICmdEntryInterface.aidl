package com.termux.x11;

// This interface is used by utility on termux side.
interface ICmdEntryInterface {
    void outputResize(int width, int height);
    ParcelFileDescriptor getXConnection();
}