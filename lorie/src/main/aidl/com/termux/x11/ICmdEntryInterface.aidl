package com.termux.x11;

import android.os.Bundle;

// This interface is used by utility on termux side.
interface ICmdEntryInterface {
    ParcelFileDescriptor getXConnection();
    ParcelFileDescriptor getLogcatOutput();
    oneway void setPreferences(in Bundle prefs);
}
