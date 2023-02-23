// ICmdEntryPointInterface.aidl
package com.termux.x11;

// Declare any non-default types here with import statements

interface ICmdEntryPointInterface {
    ParcelFileDescriptor getConnectionFd();
}