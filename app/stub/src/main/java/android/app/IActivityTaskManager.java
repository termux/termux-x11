package android.app;

import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.IInterface;
import android.os.RemoteException;
import android.content.IIntentSender;

public interface IActivityTaskManager extends IInterface {
    int startActivity(IApplicationThread caller, String callingPackage, String callingFeatureId,
							  Intent intent, String resolvedType, IBinder resultTo, String resultWho,
					          int requestCode, int startFlags, ProfilerInfo profilerInfo, Bundle bOptions)
			throws RemoteException;
	int startActivity(IApplicationThread caller, String callingPackage, Intent intent,
					  String resolvedType, IBinder resultTo, String resultWho, int requestCode,
					  int flags, ProfilerInfo profilerInfo, Bundle options)
			throws RemoteException;
	IIntentSender getIntentSender(int type, String packageName, IBinder token,
								  String resultWho, int requestCode, Intent[] intents, String[] resolvedTypes,
								  int flags, Bundle options, int userId)
			throws RemoteException;
}
