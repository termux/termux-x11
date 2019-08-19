// ITermuxService.aidl
package com.termux.x11;

import android.view.Surface;

interface ITermuxService {
    oneway void windowChanged(in Surface surface, int width, int height);
    oneway void pointerMotion(int x, int y);
    oneway void pointerScroll(int axis, float value);
    oneway void pointerButton(int button, int type);
    oneway void keyboardKey(int key, int type, int shift, String characters);
}
