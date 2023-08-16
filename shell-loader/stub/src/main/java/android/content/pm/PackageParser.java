package android.content.pm;

import android.content.pm.pkg.FrameworkPackageUserState;
import android.os.Parcel;
import android.os.Parcelable;

import java.io.File;
import java.util.Set;

public class PackageParser {
    public interface Callback {
        boolean hasFeature(String feature);
        String[] getOverlayPaths(String targetPackageName, String targetPath);
        String[] getOverlayApks(String targetPackageName);
    }

    public final static class Package implements Parcelable {
        private Package(Parcel in) {
            throw new RuntimeException("STUB");
        }

        public static final Creator<Package> CREATOR = new Creator<Package>() {
            @Override
            public Package createFromParcel(Parcel in) {
                throw new RuntimeException("STUB");
            }

            @Override
            public Package[] newArray(int size) {
                throw new RuntimeException("STUB");
            }
        };

        @Override
        public int describeContents() {
            throw new RuntimeException("STUB");
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            throw new RuntimeException("STUB");
        }
    }

    public PackageParser() {
        throw new RuntimeException("STUB");
    }

    public void setCallback(Callback cb) {
        throw new RuntimeException("STUB");
    }

    public Package parsePackage(File packageFile, int flags, boolean useCaches) {
        throw new RuntimeException("STUB");
    }

    public static void collectCertificates(Package pkg, int parseFlags) {
        throw new RuntimeException("STUB");
    }

    public static void collectCertificates(Package pkg, boolean skipVerify) {
        throw new RuntimeException("STUB");
    }

    public static PackageInfo generatePackageInfo(PackageParser.Package p,
                                                  int[] gids, int flags, long firstInstallTime, long lastUpdateTime,
                                                  Set<String> grantedPermissions, FrameworkPackageUserState state) {
        throw new RuntimeException("STUB");
    }

    public static PackageInfo generatePackageInfo(PackageParser.Package p,
                                                  int gids[], int flags, long firstInstallTime, long lastUpdateTime,
                                                  Set<String> grantedPermissions, PackageUserState state) {
        throw new RuntimeException("STUB");
    }
}
