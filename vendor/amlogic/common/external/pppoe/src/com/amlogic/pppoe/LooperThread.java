/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

package com.amlogic.pppoe;

import android.util.Log;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.MessageQueue;

import java.util.LinkedList;

public class LooperThread extends Thread {
    private static final String TAG = "LooperThread";
    private LinkedList<RunnableImpl> mQueue = new LinkedList();
    private Impl mHandler = null;
    private Looper mLooper = null;

    public LooperThread() {
        setName("PPPOE.LooperThread");
    }

    public void run() {
        mLooper = Looper.myLooper();
        Looper.prepare();
        mHandler = new Impl();
        Looper.loop();
    }

    public void post(RunnableImpl runnableimpl) {
        if(runnableimpl == null) {
            Log.w(TAG, "post runnableimpl is null!");
            return;
        }
        synchronized (mQueue) {
            mQueue.add(runnableimpl);
            if (mQueue.size() == 1) {
                scheduleNextLocked();
            }
        }
    }

    public RunnableImpl getLast() {
        synchronized (mQueue) {
            if (mQueue.size() > 0) {
                return mQueue.getLast();
            }
            else {
                return null;
            }
        }
    }

    public void cancelRunnable(RunnableImpl runnableimpl) {
        synchronized (mQueue) {
            while (mQueue.remove(runnableimpl)) { }
        }
    }

    public void cancel() {
        synchronized (mQueue) {
            mQueue.clear();
        }
    }

    private void scheduleNextLocked() {
        if (mQueue.size() > 0) {
            if(mHandler == null) {
                Log.w(TAG, "scheduleNextLocked, mHandler is null!");
                return;
            }
            mHandler.sendEmptyMessage(1);
        }
    }

    public void stopThread(){
        if (null != mLooper){
            mLooper.quit();
        }
    }

    private class Impl extends Handler {
        public void handleMessage(Message msg) {
            RunnableImpl r;
            synchronized (mQueue) {
                if (mQueue.size() == 0) {
                    Log.w(TAG, "mQueue is empty, don't do nothing!");
                    return;
                }
                r = mQueue.removeFirst();
            }
            if((r != null) && (r.mRunnable != null))
                r.mRunnable.run();
            synchronized (mQueue) {
                scheduleNextLocked();
            }
        }
    }
}
