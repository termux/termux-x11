package com.termux.x11.inputcontrols;

import android.content.Context;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedWriter;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;
import java.nio.charset.StandardCharsets;

/** Trimmed subset of Winlator's core.FileUtils/StreamUtils needed by profile JSON storage. */
abstract class FileUtils {
    private static final int BUFFER_SIZE = 64 * 1024;

    static byte[] read(File file) {
        try (InputStream inStream = new BufferedInputStream(new FileInputStream(file))) {
            return copyToByteArray(inStream);
        }
        catch (IOException e) {
            return null;
        }
    }

    static String readString(File file) {
        return new String(read(file), StandardCharsets.UTF_8);
    }

    static boolean writeString(File file, String data) {
        try (BufferedWriter bw = new BufferedWriter(new FileWriter(file))) {
            bw.write(data);
            bw.flush();
            return true;
        }
        catch (IOException e) {
            return false;
        }
    }

    static boolean copy(File srcFile, File dstFile) {
        File parent = dstFile.getParentFile();
        if (!srcFile.exists() || (parent != null && !parent.exists() && !parent.mkdirs())) return false;

        try (FileInputStream fis = new FileInputStream(srcFile); FileOutputStream fos = new FileOutputStream(dstFile)) {
            FileChannel inChannel = fis.getChannel();
            FileChannel outChannel = fos.getChannel();
            inChannel.transferTo(0, inChannel.size(), outChannel);
            return dstFile.exists();
        }
        catch (IOException e) {
            return false;
        }
    }

    static boolean isEmpty(File targetFile) {
        if (targetFile == null) return true;
        if (targetFile.isDirectory()) {
            String[] files = targetFile.list();
            return files == null || files.length == 0;
        }
        else return targetFile.length() == 0;
    }

    static boolean isDirectory(Context context, String assetFile) {
        try {
            String[] files = context.getAssets().list(assetFile);
            return files != null && files.length > 0;
        }
        catch (IOException e) {
            return false;
        }
    }

    static String getName(String path) {
        if (path == null) return "";
        while (path.endsWith("/")) path = path.substring(0, path.length() - 1);
        int index = Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
        return path.substring(index + 1);
    }

    static void copy(Context context, String assetFile, File dstFile) {
        if (isDirectory(context, assetFile)) {
            if (!dstFile.isDirectory()) dstFile.mkdirs();
            try {
                String[] filenames = context.getAssets().list(assetFile);
                for (String filename : filenames) {
                    String relativePath = (assetFile.endsWith("/") ? assetFile : assetFile + "/") + filename;
                    if (isDirectory(context, relativePath)) {
                        copy(context, relativePath, new File(dstFile, filename));
                    }
                    else copy(context, relativePath, dstFile);
                }
            }
            catch (IOException e) {}
        }
        else {
            if (dstFile.isDirectory()) dstFile = new File(dstFile, getName(assetFile));
            File parent = dstFile.getParentFile();
            if (parent != null && !parent.isDirectory()) parent.mkdirs();
            try (InputStream inStream = context.getAssets().open(assetFile);
                 BufferedOutputStream outStream = new BufferedOutputStream(new FileOutputStream(dstFile), BUFFER_SIZE)) {
                copy(inStream, outStream);
            }
            catch (IOException e) {}
        }
    }

    private static byte[] copyToByteArray(InputStream inStream) {
        if (inStream == null) return new byte[0];
        ByteArrayOutputStream outStream = new ByteArrayOutputStream(BUFFER_SIZE);
        copy(inStream, outStream);
        return outStream.toByteArray();
    }

    private static boolean copy(InputStream inStream, OutputStream outStream) {
        try {
            byte[] buffer = new byte[BUFFER_SIZE];
            int amountRead;
            while ((amountRead = inStream.read(buffer)) != -1) {
                outStream.write(buffer, 0, amountRead);
            }
            outStream.flush();
            return true;
        }
        catch (IOException e) {
            return false;
        }
    }
}
