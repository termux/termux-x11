package com.termux.x11;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Build;
import android.os.CancellationSignal;
import android.os.Parcel;
import android.os.ParcelFileDescriptor;
import android.os.SharedMemory;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.util.Log;
import android.webkit.MimeTypeMap;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Exposes the current X11-clipboard item(s) as real DocumentsContract documents.
 *
 * DocumentsUI-family file managers reconstruct a DocumentInfo from the pasted Uri and call
 * DocumentsContract methods on it -- a plain ContentProvider fails that silently. Not registered
 * as a browsable root; these documents only exist as paste targets.
 *
 * Three kinds of item:
 * - FILE: backed by a real file the X server has open. Its fd can't be reopened/dup()'d safely,
 *   so openDocument() asks the X server for a fresh one each time (LorieView.reopenClipboardItem).
 * - DIRECTORY: no fd; its content is whichever items in the same publish() have it as parent.
 * - RAW_BYTES: a shared-memory blob with no X-server path to reopen, so each openDocument() makes
 *   its own ashmem copy (copyToAshmem()).
 */
public class ClipboardDocumentsProvider extends DocumentsProvider {
    private static final String AUTHORITY = BuildConfig.APPLICATION_ID + ".clipboarddocuments";

    private static final String[] DEFAULT_ROOT_PROJECTION = {
            Root.COLUMN_ROOT_ID, Root.COLUMN_FLAGS, Root.COLUMN_TITLE, Root.COLUMN_DOCUMENT_ID,
    };
    private static final String[] DEFAULT_DOCUMENT_PROJECTION = {
            Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE,
            Document.COLUMN_SIZE, Document.COLUMN_FLAGS,
    };

    // Must match native LorieClipItemKind (lorie.h): LORIE_CLIP_FILE = 0, LORIE_CLIP_DIR = 1.
    static final int KIND_DIR = 1;

    private enum Kind { FILE, RAW_BYTES, DIRECTORY }

    private static final class Item {
        final String name;
        final String mime;
        final Kind kind;
        final ParcelFileDescriptor pfd; // null for DIRECTORY
        final int parentIndex; // index into the same generation's items[], or -1 if top-level
        Item(String name, String mime, Kind kind, ParcelFileDescriptor pfd, int parentIndex) {
            this.name = name;
            this.mime = mime;
            this.kind = kind;
            this.pfd = pfd;
            this.parentIndex = parentIndex;
        }
    }

    // A { generation, items } pair captured under the lock (see snapshot()), safe to read from
    // afterwards without holding it since items is only ever replaced wholesale, never mutated.
    private static final class Snapshot {
        final int generation;
        final Item[] items;
        Snapshot(int generation, Item[] items) {
            this.generation = generation;
            this.items = items;
        }
    }

    // The X server's own EVENT_CLIPBOARD_LIST_BEGIN generation, not a local counter -- meaningful
    // only when the current clip has FILE items.
    private static int wireGeneration = -1;
    // Uri-facing generation: bumped on every publish, FILE/DIRECTORY/RAW_BYTES alike.
    private static int currentGeneration = -1;
    private static long currentNativeContext;
    private static Item[] currentItems;

    // paths[i] is '/'-separated, relative to its top-level entry. Returns only the top-level
    // items' Uris -- nested ones are reachable solely via queryChildDocuments().
    static synchronized Uri[] publishFiles(String[] paths, String[] mimes, ParcelFileDescriptor[] pfds, int[] kinds, int generation, long nativeContext) {
        closeCurrentLocked();

        currentGeneration++;
        wireGeneration = generation;
        currentNativeContext = nativeContext;
        currentItems = new Item[paths.length];

        Map<String, Integer> pathToIndex = new HashMap<>();
        List<Uri> topLevelUris = new ArrayList<>();

        for (int i = 0; i < paths.length; i++) {
            String path = paths[i];
            int slash = path.lastIndexOf('/');
            String parentPath = slash >= 0 ? path.substring(0, slash) : null;
            String leaf = slash >= 0 ? path.substring(slash + 1) : path;
            Integer parentIndex = parentPath != null ? pathToIndex.get(parentPath) : null;
            Kind kind = kinds[i] == KIND_DIR ? Kind.DIRECTORY : Kind.FILE;

            currentItems[i] = new Item(leaf, mimes[i], kind, pfds[i], parentIndex != null ? parentIndex : -1);
            pathToIndex.put(path, i);

            if (parentPath == null)
                topLevelUris.add(buildUri(i));
        }

        return topLevelUris.toArray(new Uri[0]);
    }

    static synchronized Uri publishRawBytes(String mime, ParcelFileDescriptor pfd) {
        closeCurrentLocked();

        currentGeneration++;
        currentItems = new Item[]{new Item("", mime, Kind.RAW_BYTES, pfd, -1)};
        return buildUri(0);
    }

    private static Uri buildUri(int index) {
        return DocumentsContract.buildDocumentUri(AUTHORITY, currentGeneration + "_" + index);
    }

    private static void closeCurrentLocked() {
        if (currentItems == null)
            return;
        for (Item item : currentItems) {
            if (item.pfd == null)
                continue;
            try {
                item.pfd.close();
            } catch (IOException ignored) {
            }
        }
        currentItems = null;
    }

    private static synchronized Snapshot snapshot() {
        return new Snapshot(currentGeneration, currentItems);
    }

    @Nullable
    private static Item findItem(String documentId) {
        int sep = documentId.indexOf('_');
        if (sep < 0)
            return null;

        try {
            int generation = Integer.parseInt(documentId.substring(0, sep));
            int index = Integer.parseInt(documentId.substring(sep + 1));
            Snapshot snap = snapshot();
            if (generation != snap.generation || snap.items == null || index < 0 || index >= snap.items.length)
                return null;
            return snap.items[index];
        } catch (NumberFormatException e) {
            return null;
        }
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor queryRoots(String[] projection) {
        return new MatrixCursor(projection != null ? projection : DEFAULT_ROOT_PROJECTION);
    }

    @Override
    public Cursor queryDocument(String documentId, @Nullable String[] projection) throws FileNotFoundException {
        Item item = findItem(documentId);
        if (item == null)
            throw new FileNotFoundException("No clipboard item for " + documentId);

        String[] columns = projection != null ? projection : DEFAULT_DOCUMENT_PROJECTION;
        MatrixCursor cursor = new MatrixCursor(columns);
        addDocumentRow(cursor, columns, documentId, item);
        return cursor;
    }

    private static String extensionFor(String mime) {
        String ext = MimeTypeMap.getSingleton().getExtensionFromMimeType(mime);
        return ext != null ? ext : "bin";
    }

    private static void addDocumentRow(MatrixCursor cursor, String[] columns, String documentId, Item item) {
        Object[] row = new Object[columns.length];
        for (int i = 0; i < columns.length; i++) {
            switch (columns[i]) {
                case Document.COLUMN_DOCUMENT_ID: row[i] = documentId; break;
                case Document.COLUMN_DISPLAY_NAME:
                    row[i] = !item.name.isEmpty() ? item.name : ("clipboard." + extensionFor(item.mime));
                    break;
                case Document.COLUMN_MIME_TYPE: row[i] = item.mime; break;
                case Document.COLUMN_SIZE:
                    if (item.kind == Kind.DIRECTORY) {
                        row[i] = null;
                    } else {
                        try {
                            row[i] = statSize(item.pfd);
                        } catch (IOException e) {
                            row[i] = -1L;
                        }
                    }
                    break;
                case Document.COLUMN_FLAGS: row[i] = 0; break;
            }
        }
        cursor.addRow(row);
    }

    @Override
    public Cursor queryChildDocuments(String parentDocumentId, @Nullable String[] projection, @Nullable String sortOrder) {
        String[] columns = projection != null ? projection : DEFAULT_DOCUMENT_PROJECTION;
        MatrixCursor cursor = new MatrixCursor(columns);

        int sep = parentDocumentId.indexOf('_');
        if (sep < 0)
            return cursor;

        int generation, parentIndex;
        try {
            generation = Integer.parseInt(parentDocumentId.substring(0, sep));
            parentIndex = Integer.parseInt(parentDocumentId.substring(sep + 1));
        } catch (NumberFormatException e) {
            return cursor;
        }

        Snapshot snap = snapshot();
        if (generation != snap.generation || snap.items == null)
            return cursor;

        for (int i = 0; i < snap.items.length; i++)
            if (snap.items[i].parentIndex == parentIndex)
                addDocumentRow(cursor, columns, generation + "_" + i, snap.items[i]);

        return cursor;
    }

    @Override
    public ParcelFileDescriptor openDocument(String documentId, @NonNull String mode, @Nullable CancellationSignal signal) throws FileNotFoundException {
        Item item = findItem(documentId);
        if (item == null) {
            Log.e("CLIP", "openDocument(" + documentId + ") called but no matching clipboard item is published");
            throw new FileNotFoundException("No clipboard item available");
        }

        if (item.kind == Kind.DIRECTORY)
            throw new FileNotFoundException("Cannot open a directory: " + documentId);

        if (item.kind == Kind.FILE) {
            int index = Integer.parseInt(documentId.substring(documentId.indexOf('_') + 1));
            int fd = LorieView.reopenClipboardItem(currentNativeContext, wireGeneration, index);
            if (fd < 0) {
                Log.e("CLIP", "openDocument(" + documentId + ") -> X server reopen failed or timed out");
                throw new FileNotFoundException("Failed to reopen clipboard item");
            }

            Log.d("CLIP", "openDocument(" + documentId + ") -> reopened via X server, fd=" + fd);
            return ParcelFileDescriptor.adoptFd(fd);
        }

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O_MR1) {
            throw new FileNotFoundException("Clipboard blobs need Android 8.1 or newer");
        }

        try {
            ParcelFileDescriptor copy = copyToAshmem(item.pfd);
            Log.d("CLIP", "openDocument(" + documentId + ") -> ashmem copy, fd=" + copy.getFd());
            return copy;
        } catch (ErrnoException | IOException e) {
            Log.e("CLIP", "Failed to copy clipboard item into ashmem for " + documentId, e);
            throw new FileNotFoundException(e.getMessage());
        }
    }

    /** Fills a fresh ashmem region with a private copy of src's contents (via pread(), leaving
     * src's own position untouched) and hands it out read-only. */
    private static ParcelFileDescriptor copyToAshmem(ParcelFileDescriptor src) throws ErrnoException, IOException {
        long size = statSize(src);
        SharedMemory shm = SharedMemory.create("clipboard-item", (int) size);
        try {
            ByteBuffer buf = shm.mapReadWrite();
            try (ParcelFileDescriptor dup = src.dup()) {
                byte[] chunk = new byte[64 * 1024];
                long offset = 0;
                while (offset < size) {
                    int toRead = (int) Math.min(chunk.length, size - offset);
                    int n = Os.pread(dup.getFileDescriptor(), chunk, 0, toRead, offset);
                    if (n <= 0)
                        break;
                    buf.position((int) offset);
                    buf.put(chunk, 0, n);
                    offset += n;
                }
            } finally {
                SharedMemory.unmap(buf);
            }
            shm.setProtect(OsConstants.PROT_READ);
            return extractFd(shm);
        } finally {
            shm.close();
        }
    }

    /** SharedMemory exposes no fd getter; parceling it and reading the fd back is the only way. */
    private static ParcelFileDescriptor extractFd(SharedMemory shm) {
        Parcel parcel = Parcel.obtain();
        try {
            shm.writeToParcel(parcel, 0);
            parcel.setDataPosition(0);
            return parcel.readFileDescriptor();
        } finally {
            parcel.recycle();
        }
    }

    /** stat() doesn't report a size for ashmem-backed fds (not a regular file); fall back to the ashmem ioctl. */
    private static long statSize(ParcelFileDescriptor pfd) throws IOException {
        long size = pfd.getStatSize();
        if (size >= 0)
            return size;
        try (SharedMemory shm = SharedMemory.fromFileDescriptor(pfd.dup())) {
            return shm.getSize();
        }
    }
}
