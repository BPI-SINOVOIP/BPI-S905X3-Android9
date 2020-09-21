/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.FileBrower;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.lang.reflect.Method;
import android.provider.MediaStore.Images;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.media.ThumbnailUtils;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.os.Process;
import android.util.Log;
import android.os.Environment;

import com.droidlogic.FileBrower.FileBrowerDatabase.ThumbnailCursor;
import com.droidlogic.app.FileListManager;

public class ThumbnailScannerService extends Service implements Runnable {
    private static final String TAG = "ThumbnailScannerService";
    public static final String ACTION_THUMBNAIL_SCANNER_FINISHED
        = "com.droidlogic.FileBrower.THUMBNAIL_SCANNER_FINISHED";
    private static FileBrowerDatabase db;
    private static boolean stop_scanner = false;

    private volatile Looper mServiceLooper;
    private volatile ServiceHandler mServiceHandler;
    private PowerManager.WakeLock mWakeLock;

    @Override
    public void onCreate()
    {
        db = new FileBrowerDatabase(this);
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, TAG);

        // Start up the thread running the service.  Note that we create a
        // separate thread because the service normally runs in the process's
        // main thread, which we don't want to block.
        Thread thr = new Thread(null, this, "ThumbnailScannerService");
        thr.start();
        stop_scanner = false;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId)
    {
        while (mServiceHandler == null) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                }
            }
        }

        if (intent == null) {
            Log.e(TAG, "Intent is null in onStartCommand: ",
                new NullPointerException());
            return Service.START_NOT_STICKY;
        }

        Message msg = mServiceHandler.obtainMessage();
        msg.arg1 = startId;
        msg.obj = intent.getExtras();
        mServiceHandler.sendMessage(msg);

        // Try again later if we are killed before we can finish scanning.
        return Service.START_REDELIVER_INTENT;
    }

    @Override
    public void onDestroy()
    {
        stop_scanner = true;
        if (db != null) db.close();
        // Make sure thread has started before telling it to quit.
        while (mServiceLooper == null) {
            synchronized (this) {
                try {
                    wait(100);
                } catch (InterruptedException e) {
                }
            }
        }
        mServiceLooper.quit();
    }

    public void run()
    {
        // reduce priority below other background threads to avoid interfering
        // with other services at boot time.
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND +
                Process.THREAD_PRIORITY_LESS_FAVORABLE);
        Looper.prepare();

        mServiceLooper = Looper.myLooper();
        mServiceHandler = new ServiceHandler();

        Looper.loop();
    }

    private final class ServiceHandler extends Handler
    {
        @Override
        public void handleMessage(Message msg)
        {
            Bundle arguments = (Bundle) msg.obj;
            String dir_path = arguments.getString("dir_path");
            String scan_type = arguments.getString("scan_type");

            try {
                if (scan_type != null) {
                    if (scan_type.equals("all")) {
                        long start_time, end_time;
                        start_time = System.currentTimeMillis();

                        File dir = new File(FileListManager.STORAGE);
                        if (dir.exists() && dir.isDirectory()) {
                            if (dir.listFiles() != null) {
                                if (dir.listFiles().length > 0) {
                                    for (File file : dir.listFiles()) {
                                        if (file.isDirectory()) {
                                            String path = file.getAbsolutePath();
                                            if (path.equals(FileListManager.NAND)) {
                                                if (createThumbnailsInDir(path) > 0) {
                                                    sendBroadcast(new Intent(ACTION_THUMBNAIL_SCANNER_FINISHED));
                                                }
                                            }
                                        }
                                    }

                                    for (File file : dir.listFiles()) {
                                        if (file.isDirectory()) {
                                            String path = file.getAbsolutePath();
                                            if (path.equals(FileListManager.NAND)) {
                                                createAllThumbnailsInDir(path);
                                                sendBroadcast(new Intent(ACTION_THUMBNAIL_SCANNER_FINISHED));
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        end_time = System.currentTimeMillis();
                    }  else if (scan_type.equals("dev")) {
                        if (dir_path != null) {
                            long start_time, end_time;
                            start_time = System.currentTimeMillis();
                            if (createThumbnailsInDir(dir_path) > 0) {
                                sendBroadcast(new Intent(ACTION_THUMBNAIL_SCANNER_FINISHED));
                            }
                            createAllThumbnailsInDir(dir_path);
                            end_time = System.currentTimeMillis();
                            sendBroadcast(new Intent(ACTION_THUMBNAIL_SCANNER_FINISHED));
                        }
                    } else if (scan_type.equals("dir")) {
                        if (dir_path != null) {
                            if (createThumbnailsInDir(dir_path) > 0) {
                                sendBroadcast(new Intent(ACTION_THUMBNAIL_SCANNER_FINISHED));
                            }
                        }
                    } else if (scan_type.equals("clean")) {
                        long start_time, end_time;
                        start_time = System.currentTimeMillis();
                        cleanThumbnails();
                        end_time = System.currentTimeMillis();
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception in handleMessage", e);
            }

            stopSelf(msg.arg1);
        }
    }

    @Override
    public IBinder onBind(Intent arg0) {
        // TODO Auto-generated method stub
        return null;
    }

    private void cleanThumbnails() {
        if (db != null) {
            ThumbnailCursor cc = null;
            try {
                cc = db.checkThumbnail();
                if (cc != null && cc.moveToFirst()) {
                    if (cc.getCount() > 0) {
                        for (int i = 0; i < cc.getCount(); i++) {
                            cc.moveToPosition(i);
                            String file_path = cc.getColFilePath();
                            if (file_path != null) {
                                if (!new File(file_path).exists()) {
                                    db.deleteThumbnail(file_path);
                                }
                            }
                        }
                    }
                }
            } finally {
                if (cc != null) cc.close();
            }
        }
    }

    private int createThumbnail(String file_path) throws IOException {
        if (stop_scanner) return 0;
        int count = 0;
        if (file_path != null && db != null) {
            File file = new File(file_path);
            if (FileListManager.isPhoto(file_path) &&  file != null && file.exists()) {
                //OutOfMemoryError, ignore file size >15MB
                if (file.length() > 1024*1024*15 || file.length() <= 0) {
                    return 0;
                }

                ThumbnailCursor cc = null;
                try {
                    cc = db.checkThumbnailByPath(file_path);
                    if (cc != null && cc.moveToFirst()) {
                        if (cc.getCount() > 0)
                            return 0;
                    }
                } finally {
                    if (cc != null) cc.close();
                }
                Bitmap bitmap = null;
                Bitmap thumb = null;
                bitmap = createImageThumbnail(file_path,
                Images.Thumbnails.MINI_KIND);
                if (bitmap != null) {
                    thumb = ThumbnailUtils.extractThumbnail(bitmap, 96, 96);
                    if (!bitmap.isRecycled())
                        bitmap.recycle();

                    if (thumb != null) {
                        ByteArrayOutputStream os = new ByteArrayOutputStream();
                        thumb.compress(Bitmap.CompressFormat.PNG, 100, os);
                        if (db != null) {
                            db.addThumbnail(file_path, os.toByteArray());
                            count++;
                         }
                         try {
                             os.close();
                         } catch (IOException e) {
                             // TODO Auto-generated catch block
                             e.printStackTrace();
                         }
                         if (!thumb.isRecycled())
                             thumb.recycle();
                    }
                }
            }
        }
        return count;
    }

    public Bitmap createImageThumbnail(String file_path,int kind) {
        try {
            Class<?> sm = Class.forName("android.media.ThumbnailUtils");
            Method createThumbnail = sm.getMethod("createImageThumbnail", String.class,int.class);
            Bitmap bitmap = (Bitmap) createThumbnail.invoke(null, file_path, kind);
            return bitmap;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private int createThumbnailsInDir(String dir_path) {
        // don't sleep while scanning
        mWakeLock.acquire();
        int count = 0;
        long start_time, end_time;
        start_time = System.currentTimeMillis();
        if (dir_path != null) {
            if (!dir_path.startsWith(FileListManager.NAND))
                return 0;
            File dir = new File(dir_path);
            if (dir.exists() && dir.isDirectory()) {
                if (dir.listFiles() != null) {
                    if (dir.listFiles().length > 0) {
                        for (File file : dir.listFiles()) {
                            if (file.exists() && file.isFile() && FileListManager.isPhoto(file.getName())) {
                                try {
                                    count += createThumbnail(file.getAbsolutePath());
                                } catch (IOException e) {
                                    // TODO Auto-generated catch block
                                    e.printStackTrace();
                                }
                            }
                        }
                    }
                }
            }
        }
        end_time = System.currentTimeMillis();
        mWakeLock.release();
        return count;
    }

    private void createAllThumbnailsInDir(String dir_path) {
        // don't sleep while scanning
        mWakeLock.acquire();
        if (dir_path != null) {
            if (!dir_path.startsWith(FileListManager.NAND))
                return;
            File dir = new File(dir_path);
            if (dir.exists() && dir.isDirectory()) {
                if (dir.listFiles() != null) {
                    if (dir.listFiles().length > 0) {
                        for (File file : dir.listFiles()) {
                            if (file.exists())
                            if (file.isDirectory()) {
                                createAllThumbnailsInDir(file.getAbsolutePath());
                            } else if (file.isFile() && FileListManager.isPhoto(file.getName())) {
                                try {
                                    createThumbnail(file.getAbsolutePath());
                                } catch (IOException e) {
                                    // TODO Auto-generated catch block
                                    e.printStackTrace();
                                }
                            }
                        }
                    }
                }
            }
        }
        mWakeLock.release();
    }
}