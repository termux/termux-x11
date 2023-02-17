package android.os;

public class ServiceManager {
    /**
     * Returns a reference to a service with the given name.
     *
     * @param name the name of the service to get
     * @return a reference to the service, or <code>null</code> if the service doesn't exist
     */
    public static IBinder getService(String name) {
        throw new RuntimeException("STUB");
    }
}
