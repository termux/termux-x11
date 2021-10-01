package com.termux.x11.starter;

import android.os.ParcelFileDescriptor;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

public class ExecHelper {
    public static void exec(String executable, String[] args) throws IOException {
        final ByteArrayOutputStream baos = new ByteArrayOutputStream();
        final DataOutputStream ous = new DataOutputStream(baos);
        ous.writeBytes(executable);
        ous.writeByte(0);
        if (args != null)
            for (String s: args) {
                ous.write(s.getBytes());
                ous.writeByte(0); // as Null character
            }
        ous.flush();
        ous.close();

        final byte[] bArray = baos.toByteArray();
        long pipe = createPipe(bArray.length);
        int pipeFd = pipeWriteFd(pipe);
        ParcelFileDescriptor fd = ParcelFileDescriptor.adoptFd(pipeFd);
        OutputStream stream = new FileOutputStream(fd.getFileDescriptor());

        int len = Math.min(bArray.length, 1024);
        for (int i = 0; i < bArray.length; i+= len) {
            if (i + len > bArray.length)
                len = bArray.length - i;
            stream.write(bArray, i, len);
            flushPipe(pipe);
        }
        performExec(pipe);
    }

    private static native long createPipe(int capacity);
    private static native int pipeWriteFd(long p);
    private static native void flushPipe(long p);
    private static native void performExec(long p);
}
