/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

package com.amlogic.pppoe;
 
import android.util.Log;

public class PppoeOperation 
{
    public static final String TAG = "PppoeOperation";
    public static final int PPP_STATUS_CONNECTED = 0x10;
    public static final int PPP_STATUS_DISCONNECTED = 0x20;
    public static final int PPP_STATUS_CONNECTING = 0x40;

    private native boolean _connect(String ifname, String account, String passwd);
    private native boolean _disconnect();
    private native boolean _terminate();
    public native int isNetAdded(String ifname);
    public native int status(String ifname);
    private static LooperThread mLooper = null;
    static {
        System.loadLibrary("pppoejni");
        Log.d(TAG, "LooperThread init!");
        mLooper = new LooperThread();
        mLooper.start();
    }

    public PppoeOperation() {
        Log.d(TAG, "PppoeOperation init!");
    }

    public boolean connect(String ifname, String account, String passwd) {
        String func = "connect";
        Log.d(TAG, func + ", ifname: " + ifname + " account: " + account + " passwd: " + passwd);
        
        if((ifname == null) || (isNetAdded(ifname) == 0)) {
            Log.w(TAG, "The " + ifname + " is down or ifname is null, don't do connect!");
            return false;
        }
        RunnableImpl mLast = mLooper.getLast();
        if((mLast != null) && func.equals(mLast.mName)) {
            Log.w(TAG, "the last action is connect, don't do again!");
            return false;
        }
        final String Ifname = ifname;
        final String Account = account;
        final String Passwd = passwd;
        RunnableImpl mCur = new RunnableImpl();
        mCur.mName = func;
        mCur.mRunnable = new Runnable() {
            public void run() {
                _connect(Ifname, Account, Passwd);
            }
        };
        mLooper.post(mCur);
        return true;
    }

    public boolean disconnect() {
        String func = "disconnect";

        Log.d(TAG, func);
        RunnableImpl mLast = mLooper.getLast();
        if((mLast != null) && func.equals(mLast.mName)) {
            Log.w(TAG, "the last action is disconnect, don't do again!");
            return false;
        }
        RunnableImpl mCur = new RunnableImpl();
        mCur.mName = func;
        mCur.mRunnable = new Runnable() {
            public void run() {
                _disconnect();
            }
        };
        mLooper.post(mCur);
        return true;
    }
	
    public boolean terminate() {
        String func = "terminate";
        String connect = "connect";

        Log.d(TAG, func);
        if((isNetAdded("eth0") == 0) && (isNetAdded("wlan0") == 0) && (isNetAdded("usbnet0") == 0)) {
            Log.w(TAG, "The eht0/wlan0/usbnet0 is down, don't do terminate!");
            return false;
        }
        RunnableImpl mLast = mLooper.getLast();
        if(mLast != null) {
            if(func.equals(mLast.mName)) {
                Log.w(TAG, "the last action is terminate, don't do again!");
                return false;
            }
            else if(connect.equals(mLast.mName)) {
                Log.w(TAG, "the last action is connect, don't do terminate!");
                return false;
            }
        }
        RunnableImpl mCur = new RunnableImpl();
        mCur.mName = func;
        mCur.mRunnable = new Runnable() {
            public void run() {
                _terminate();
            }
        };
        mLooper.post(mCur);
        return true;
    }
}
