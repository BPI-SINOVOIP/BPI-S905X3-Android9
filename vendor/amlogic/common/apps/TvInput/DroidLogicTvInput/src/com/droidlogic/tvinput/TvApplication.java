/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.tvinput;

import android.app.Application;
import android.content.Context;
import android.content.res.AssetManager;
import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.zip.ZipFile;
import java.util.zip.ZipException;
import java.util.zip.ZipEntry;
import java.util.Enumeration;

public class TvApplication extends Application{
    private static Context mContext;
    private static String unZipDirStr = "vendorfont";
    private static String uncryptDirStr = "font";
    private static String fonts = "font";


    @Override
    public void onCreate() {
        super.onCreate();
        mContext = getApplicationContext();
        File uncryteDir = new File(mContext.getDataDir(),uncryptDirStr);
        if (uncryteDir == null || uncryteDir.listFiles() == null || uncryteDir.listFiles().length == 0 ) {
            try {
                InputStream instream = getAssets().open(fonts);
                if (instream != null) {
                    FileUtils.unzipUncry(uncryteDir,instream);
                }
            }catch (IOException ex) {}
        }
    }

    static class FileUtils {

        static {
            System.loadLibrary("jnifont");
        }
        static void unzipUncry(File uncryteDir,InputStream instream) {
            File zipFile = new File(mContext.getDataDir(),"fonts.zip");
            File unZipDir = new File(mContext.getDataDir(),unZipDirStr);
            try {
                copyToFileOrThrow(instream,zipFile);
                upZipFile(zipFile,unZipDir.getCanonicalPath());
                uncryteDir.mkdirs();
                nativeUnCrypt(unZipDir.getCanonicalPath()+"/",uncryteDir.getCanonicalPath()+"/");
                zipFile.delete();
                if (unZipDir.exists() && unZipDir.listFiles() != null) {
                    for (File f:unZipDir.listFiles()) {
                        f.delete();
                    }
                    unZipDir.delete();
                }
            }catch(IOException ex){
                ex.printStackTrace();
            }

        }

        private static void copyToFileOrThrow(InputStream inputStream, File destFile)
                throws IOException {
            if (destFile.exists()) {
                destFile.delete();
            }
            FileOutputStream out = new FileOutputStream(destFile);
            try {
                byte[] buffer = new byte[4096];
                int bytesRead;
                while ((bytesRead = inputStream.read(buffer)) >= 0) {
                    out.write(buffer, 0, bytesRead);
                }
            } finally {
                out.flush();
                try {
                    out.getFD().sync();
                } catch (IOException e) {
                }
                out.close();
            }
        }

        private static void upZipFile(File zipFile, String folderPath)
                throws ZipException, IOException {
                File desDir = new File(folderPath);
                if (!desDir.exists()) {
                    desDir.mkdirs();
                }
                ZipFile zf = new ZipFile(zipFile);
                for (Enumeration<?> entries = zf.entries(); entries.hasMoreElements();) {
                    ZipEntry entry = ((ZipEntry) entries.nextElement());
                    InputStream in = zf.getInputStream(entry);
                    String str = folderPath;
                    // str = new String(str.getBytes("8859_1"), "GB2312");
                    File desFile = new File(str, java.net.URLEncoder.encode(
                            entry.getName(), "UTF-8"));
                    if (!desFile.exists()) {
                        File fileParentDir = desFile.getParentFile();
                        if (!fileParentDir.exists()) {
                            fileParentDir.mkdirs();
                        }
                    }

                    OutputStream out = new FileOutputStream(desFile);
                    byte buffer[] = new byte[1024 * 1024];
                    int realLength = in.read(buffer);
                    while (realLength != -1) {
                        out.write(buffer, 0, realLength);
                        realLength = in.read(buffer);
                    }

                    out.close();
                    in.close();

                }
            }
        private static native void nativeUnCrypt(String src,String dest);

    }
}
