/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
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
package com.droidlogic.mediacenter.dlna;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.lang.ref.SoftReference;

import android.graphics.Bitmap;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.widget.ImageView;

public class AsynImageLoader {
    private static final String TAG = "AsynImageLoader";
    public static final String CACHE_DIR = "dlna";
    // cache images map loaded
    private Map<String, SoftReference<Bitmap>> caches;
    // task list
    private HashMap<String,Task> taskQueue;
    private ArrayList<String> taskQueueName;
    private boolean isRunning = false;

    private ImageCallback getImageCallback(final ImageView imageView, final int resId) {

        return new ImageCallback() {

            @Override
            public void loadImage(String path, Bitmap bitmap) {
                if (path.equals(imageView.getTag().toString())) {
                    imageView.setImageBitmap(bitmap);
                } else {
                    imageView.setImageResource(resId);
                }
            }

        };

    }

    private Runnable runnable = new Runnable() {
        public void run() {
            while (isRunning) {
                while ( taskQueueName != null && taskQueueName.size() > 0) {
                    String key = taskQueueName.remove(0);
                    Task task = taskQueue.remove(key);
                    task.bitmap = PicUtil.getbitmapAndwrite(task.path);
                    caches.put(task.path, new SoftReference<Bitmap>(task.bitmap));
                    if (handler != null) {
                        Message msg = handler.obtainMessage();
                        msg.obj = task;
                        handler.sendMessage(msg);
                    }
                }
                synchronized (this) {
                    try {
                        this.wait();
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    };
    private Handler handler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            Task task = (Task) msg.obj;
            task.callback.loadImage(task.path, task.bitmap);
        }

    };

    public AsynImageLoader() {
        // init
        caches = new HashMap<String, SoftReference<Bitmap>>();
        taskQueue = new HashMap<String,AsynImageLoader.Task>();
        taskQueueName = new ArrayList<String>();
        isRunning = true;
        new Thread(runnable).start();
    }

    public void showImageAsyn(ImageView imageView, String url, int resId) {
        imageView.setTag(url);
        Bitmap bitmap = loadImageAsyn(url, getImageCallback(imageView, resId));

        if (bitmap == null) {
            imageView.setImageResource(resId);
        } else {
            imageView.setImageBitmap(bitmap);
        }
    }

    /**
     * @Description TODO
     * @param url
     * @param imageCallback
     * @return
     */
    private synchronized Bitmap loadImageAsyn(String path, ImageCallback imageCallback) {
        if (caches.containsKey(path)) {
            SoftReference<Bitmap> rf = caches.get(path);
            Bitmap bitmap = rf.get();
            if (bitmap == null) {
                caches.remove(path);
            } else {
                Log.i(TAG, "return image in cache" + path);
                return bitmap;
            }
        } else {
            if (!taskQueue.containsKey(path)) {
                Task task = new Task();
                task.path = path;
                task.callback = imageCallback;
                taskQueueName.add(path);
                taskQueue.put(path,task);
                synchronized (runnable) {
                    runnable.notify();
                }
            }
        }
        return null;
    }

    public interface ImageCallback {
        void loadImage(String path, Bitmap bitmap);
    }

    class Task {
        String path;
        Bitmap bitmap;
        ImageCallback callback;

        @Override
        public boolean equals(Object o) {
            Task objTask = (Task) o;
            return objTask.path.equals(path);
        }

    }
}