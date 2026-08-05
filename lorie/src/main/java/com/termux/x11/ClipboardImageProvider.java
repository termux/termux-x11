package com.termux.x11;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelFileDescriptor;
import android.os.ProxyFileDescriptorCallback;
import android.os.storage.StorageManager;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InterruptedIOException;

/**
 * Serves the current clipboard image without ever touching disk and without the fd-sharing
 * pitfalls of dup()/reopen. StorageManager.openProxyFileDescriptor() gives every caller a
 * genuinely independent, fully seekable virtual file backed by a FUSE bridge; each read the OS
 * asks for is answered with an explicit-offset pread() on the underlying shared memory region,
 * which never touches a shared file position the way plain read() on a dup()/reopened fd does, so
 * concurrent/repeated readers can't race over a cursor.
 *
 * Earlier attempts and why they didn't work: handing the raw ashmem/memfd fd straight to third-party
 * apps wasn't reliably read()-able for them; a plain pipe doesn't support the seek/rewind several
 * decoders need; re-opening via /proc/self/fd gets blocked by SELinux; writing a cache file works
 * but means a disk write on every single copy.
 */
public class ClipboardImageProvider extends ContentProvider {
    private static final Uri URI = new Uri.Builder().scheme("content")
            .authority(BuildConfig.APPLICATION_ID + ".clipboardimage").path("clipboard").build();
    private static ParcelFileDescriptor currentImage;
    private static String currentMime = "application/octet-stream";
    private static Handler handler;

    static synchronized Uri publish(String mime, ParcelFileDescriptor pfd) {
        if (currentImage != null) {
            try {
                currentImage.close();
            } catch (IOException ignored) {
            }
        }
        currentImage = pfd;
        currentMime = mime;
        return URI;
    }

    @Override
    public boolean onCreate() {
        HandlerThread thread = new HandlerThread("ClipboardImageProxy");
        thread.start();
        handler = new Handler(thread.getLooper());
        return true;
    }

    @Override
    public ParcelFileDescriptor openFile(@NonNull Uri uri, @NonNull String mode) throws FileNotFoundException {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            throw new FileNotFoundException("Clipboard images need Android 8.0 or newer");
        }

        ParcelFileDescriptor pfd;
        synchronized (ClipboardImageProvider.class) {
            pfd = currentImage;
        }
        if (pfd == null) {
            Log.e("CLIP", "openFile(" + uri + ") called but no clipboard image is published");
            throw new FileNotFoundException("No clipboard image available");
        }

        StorageManager sm = (StorageManager) getContext().getSystemService(Context.STORAGE_SERVICE);
        try {
            ParcelFileDescriptor proxy = sm.openProxyFileDescriptor(
                    ParcelFileDescriptor.MODE_READ_ONLY, new ReadCallback(pfd), handler);
            Log.d("CLIP", "openFile(" + uri + ") -> proxy fd=" + proxy.getFd());
            return proxy;
        } catch (IOException e) {
            Log.e("CLIP", "Failed to open proxy fd for clipboard image", e);
            throw new FileNotFoundException(e.getMessage());
        }
    }

    /** Reads straight from a private dup() of the published fd via pread(), never touching its position. */
    private static class ReadCallback extends ProxyFileDescriptorCallback {
        private final ParcelFileDescriptor pfd;

        ReadCallback(ParcelFileDescriptor source) throws IOException {
            pfd = source.dup();
        }

        @Override
        public long onGetSize() {
            return pfd.getStatSize();
        }

        @Override
        public int onRead(long offset, int size, byte[] data) throws ErrnoException {
            try {
                return Os.pread(pfd.getFileDescriptor(), data, 0, size, offset);
            } catch (InterruptedIOException e) {
                return 0;
            }
        }

        @Override
        public void onRelease() {
            try {
                pfd.close();
            } catch (IOException ignored) {
            }
        }
    }

    @Nullable
    @Override
    public String getType(@NonNull Uri uri) {
        synchronized (ClipboardImageProvider.class) {
            return currentMime;
        }
    }

    @Nullable
    @Override
    public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
        return null;
    }

    @Nullable
    @Override
    public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
        return null;
    }

    @Override
    public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
        return 0;
    }

    @Override
    public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
        return 0;
    }
}
