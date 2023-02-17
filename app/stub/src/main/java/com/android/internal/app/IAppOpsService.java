package com.android.internal.app;

import android.os.IBinder;

public interface IAppOpsService extends android.os.IInterface {
    public static abstract class Stub extends android.os.Binder implements IAppOpsService {
        public static IAppOpsService asInterface(IBinder obj) {
            throw new RuntimeException("STUB");
        }
    }
}
