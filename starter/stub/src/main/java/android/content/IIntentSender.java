/*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.content;

import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;

public interface IIntentSender {
    // Android 8+
    void send(int code, Intent intent, String resolvedType, IBinder whitelistToken,
              IIntentReceiver finishedReceiver, String requiredPermission, Bundle options)
            throws RemoteException;
    // Android 6+
    int send(int code, Intent intent, String resolvedType,
             IIntentReceiver finishedReceiver, String requiredPermission, Bundle options)
            throws RemoteException;
}