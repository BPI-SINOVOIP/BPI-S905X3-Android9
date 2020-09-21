/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SubtitleMiddleClientManager
 */

package com.droidlogic.SubTitleService;

import java.util.NoSuchElementException;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import android.os.Handler;
import android.os.HwBinder;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.RemoteException;

//import android.hidl.manager.V1_0.IServiceManager;
//import android.hidl.manager.V1_0.IServiceNotification;
import vendor.amlogic.hardware.subtitleserver.V1_0.ISubtitleServer;
import vendor.amlogic.hardware.subtitleserver.V1_0.ISubtitleServerCallback;
import vendor.amlogic.hardware.subtitleserver.V1_0.SubtitleHidlParcel;
import vendor.amlogic.hardware.subtitleserver.V1_0.ConnectType;
import vendor.amlogic.hardware.subtitleserver.V1_0.Result;

import com.tv.DTVSubtitleView;

public class SubtitleMiddleClientManager {
    private static final String TAG                             = "SubtitleMiddleClientManager";

    public static final String EVENT_TYPE                       = "event";

    private Context mContext = null;
    private EventHandler mEventHandler;
    private HALCallback mHALCallback;

    private SubTitleService mSubtitleService;

    private ISubtitleServer mProxy = null;
    // Mutex for all mutable shared state.
    private final Object mLock = new Object();

    private static final int SUBTITLESERVER_DEATH_COOKIE = 1000;

    // Notification object used to listen to the start of the subtitleserver daemon.
    //private final ServiceNotification mServiceNotification = new ServiceNotification();

    //add for CTC HIDL
    public static final int EVENT_SHOW_SUB = 0;
    public static final int EVENT_OPEN =1;
    public static final int EVENT_OPENIDX = 2;
    public static final int EVENT_CLOSE =3;
    public static final int EVENT_GETOTAL =4;
    public static final int EVENT_NEXT = 5;
    public static final int EVENT_PREVIOUS = 6;
    public static final int EVENT_OPTION = 7;
    public static final int EVENT_GETTYPE = 8;
    public static final int EVENT_GETTYPEDETIAL = 9;
    public static final int EVENT_SETTEXTCOLOR = 10;
    public static final int EVENT_SETTEXTSIZE = 11;
    public static final int EVENT_SETGRAVITY = 12;
    public static final int EVENT_SETTEXTSTYLE = 13;
    public static final int EVENT_SETPOSHEIGHT = 14;
    public static final int EVENT_SETIMGRATIO = 15;
    public static final int EVENT_CLEAR = 16;
    public static final int EVENT_RESETFORSEEK = 17;
    public static final int EVENT_HIDE = 18;
    public static final int EVENT_DISPLAY = 19;
    public static final int EVENT_GETCURNAME = 20;
    public static final int EVENT_GETNAME = 21;
    public static final int EVENT_GETLANAGUAGE = 22;
    public static final int EVENT_LOAD = 23;
    public static final int EVENT_SETSURFACEVIEWPARAM = 24;
    public static final int EVENT_CREAE = 25;
    public static final int EVENT_SETSUBPID = 26;
    public static final int EVENT_GETSUBHEIGHT = 27;
    public static final int EVENT_GETSUBWIDTH = 28;
    public static final int EVENT_SETSUBTYPE = 29;
    public static final int EVENT_ONSUBTITLEDATA_CALLBACK = 30;
    public static final int EVENT_ONSUBTITLEAVAILABLE_CALLBACK = 31;
    public static final int EVENT_DESTORY = 32;
    public static final int EVENT_SETPAGEID = 33;
    public static final int EVENT_SETANCPAGEID = 34;
    public static final int EVENT_SETCHANNELID = 35;
    public static final int EVENT_SETCLOSEDCAPTIONVFMT = 36;


    public SubtitleMiddleClientManager(Context context) {
        Log.i(TAG, "SubtitleMiddleClientManager....");
        mContext = context;
        Looper looper = Looper.myLooper();
        if (looper != null) {
            mEventHandler = new EventHandler(looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(looper);
        } else {
            mEventHandler = null;
            Log.e(TAG, "looper is null, so can not do anything");
        }
        mHALCallback = new HALCallback(this);

        /*try {
            boolean ret = IServiceManager.getService()
                    .registerForNotifications("vendor.amlogic.hardware.subtitleserver@1.0::ISubtitleServer", "", mServiceNotification);
            if (!ret) {
                Log.e(TAG, "Failed to register service start notification");
            }
        } catch (RemoteException e) {
            Log.e(TAG, "Failed to register service start notification", e);
        }*/
        connectToProxy();
        DTVSubtitleView.setCallback(mSubtitleHidlCallback);
    }

    private void connectToProxy() {
        synchronized (mLock) {
            if (mProxy != null) {
                return;
            }

            try {
                mProxy = ISubtitleServer.getService();
                mProxy.linkToDeath(new DeathRecipient(), SUBTITLESERVER_DEATH_COOKIE);
                mProxy.setCallback(mHALCallback, ConnectType.TYPE_EXTEND);
            } catch (NoSuchElementException e) {
                Log.e(TAG, "connectToProxy: subtitleserver HIDL service not found."
                        + " Did the service fail to start?", e);
            } catch (RemoteException e) {
                Log.e(TAG, "connectToProxy: subtitleserver HIDL service not responding", e);
            }
        }

        Log.i(TAG, "connect to subtitleserver HIDL service success");
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
        }

        @Override
        public void serviceDied(long cookie) {
            if (SUBTITLESERVER_DEATH_COOKIE == cookie) {
                Log.e(TAG, "subtitleserver HIDL service died cookie: " + cookie);
                synchronized (mLock) {
                    mProxy = null;
                }
            }
        }
    }

    /*final class ServiceNotification extends IServiceNotification.Stub {
        @Override
        public void onRegistration(String fqName, String name, boolean preexisting) {
            Log.i(TAG, "subtitleserver HIDL service started " + fqName + " " + name);
            connectToProxy();
        }
    }*/

    public DTVSubtitleView.SubtitleDataListener  mSubtitleHidlCallback = new DTVSubtitleView.SubtitleDataListener()  {
        public void onSubTitleEvent(int event, int id) {
            Log.i(TAG, "[onSubTitleEvent]event:" + event + ",id:" + id);
            subTitleIdCallback(event, id);
        }

        public void onSubTitleAvailable(int available)  {
            Log.i(TAG, "[onSubTitleAvailable]available:" + available);
            subTitleAvailableCallback(available);
        }
    };


    class EventHandler extends Handler {

        public EventHandler(Looper looper) {
            super(looper);

        }

        @Override
        public void handleMessage(Message msg) {
            int i = 0, loop_count = 0, tmp_val = 0;

            SubtitleHidlParcel parcel = ((SubtitleHidlParcel) (msg.obj));
            switch (msg.what) {
                case EVENT_CREAE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService = new SubTitleService(mContext);
                    break;
                case EVENT_SHOW_SUB:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.showSub(parcel.bodyInt.get(0));
                    break;
                case EVENT_OPEN:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyString.get(0));
                    mSubtitleService.setIOType(0);   //for ctc type
                    mSubtitleService.open(parcel.bodyString.get(0));
                    break;
                case EVENT_OPENIDX:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    Log.i(TAG, "handleMessage -1-");
                    mSubtitleService.openIdx(parcel.bodyInt.get(0));
                    break;
                case EVENT_CLOSE:
                    mSubtitleService.close();
                    break;
                case EVENT_GETOTAL:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    int total;
                    total = mSubtitleService.getSubTotal();
                    setTotal(total);
                     break;
                case EVENT_NEXT:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.nextSub();
                    break;
                case EVENT_PREVIOUS:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.preSub();
                    break;
                case EVENT_OPTION:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    //mSubtitleService.previous();
                    break;
                case EVENT_GETTYPE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    int type = mSubtitleService.getSubType();
                    setType(type);
                    break;
                case EVENT_GETTYPEDETIAL:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    int typeDetial = mSubtitleService.getSubTypeDetial();
                    setType(typeDetial);
                    break;
                case EVENT_SETTEXTCOLOR:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setTextColor(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETTEXTSIZE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setTextSize(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETGRAVITY:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setGravity(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETTEXTSTYLE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setTextStyle(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETPOSHEIGHT:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setPosHeight(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETIMGRATIO:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + "value:" + parcel.bodyInt.get(0));
                    mSubtitleService.setPosHeight(parcel.bodyInt.get(0));
                    break;
                case EVENT_CLEAR:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.clear();
                    break;
                case EVENT_RESETFORSEEK:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.resetForSeek();
                    break;
                case EVENT_HIDE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.hide();
                    break;
                case EVENT_DISPLAY:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.display();
                    break;
                case EVENT_GETCURNAME:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.getCurName();
                    break;
                case EVENT_GETNAME:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    break;
                case EVENT_GETLANAGUAGE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    String language = mSubtitleService.getSubLanguage(0);
                    setLanguage(language);
                    break;
                case EVENT_LOAD:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.load(parcel.bodyString.get(0));
                    break;
                case EVENT_SETSURFACEVIEWPARAM:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType + ",x:" + parcel.bodyInt.get(0));
                    mSubtitleService.setSurfaceViewParam(parcel.bodyInt.get(0), parcel.bodyInt.get(1), parcel.bodyInt.get(2), parcel.bodyInt.get(3));
                    break;
                case EVENT_SETSUBPID:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setSubPid(parcel.bodyInt.get(0));
                    break;
                case EVENT_GETSUBHEIGHT:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    int height = mSubtitleService.getSubHeight();
                    setSubHeight(height);
                    break;
                case EVENT_GETSUBWIDTH:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    int width = mSubtitleService.getSubWidth();
                    setSubWidth(width);
                    break;
                case EVENT_SETSUBTYPE:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setSubType(parcel.bodyInt.get(0));
                    break;
                case EVENT_DESTORY:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.destory();
                    break;
                case EVENT_SETPAGEID:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setPageId(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETANCPAGEID:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setAncPageId(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETCHANNELID:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setChannelId(parcel.bodyInt.get(0));
                    break;
                case EVENT_SETCLOSEDCAPTIONVFMT:
                    Log.i(TAG, "handleMessage type:" + parcel.msgType);
                    mSubtitleService.setClosedCaptionVfmt(parcel.bodyInt.get(0));
                    break;
                 default:
                     Log.e(TAG, "Unknown message type " + msg.what);
                     break;
            }
        }
    }

    public int setType(int type) {
        synchronized (mLock) {
            try {
                 mProxy.setType(type);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "setType:" + e);
            }
        }
        return -1;
    }

    public int setTotal(int total) {
        synchronized (mLock) {
            try {
                 mProxy.setTotal(total);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "setType:" + e);
            }
        }
        return -1;
    }

    public int setSubHeight(int height) {
        synchronized (mLock) {
            try {
                 mProxy.setSubHeight(height);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "setSubHeight:" + e);
            }
        }
        return -1;
    }

    public int setSubWidth(int width) {
        synchronized (mLock) {
            try {
                 mProxy.setSubWidth(width);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "setSubWidth:" + e);
            }
        }
        return -1;
    }

    public int setLanguage(String language) {
        if (language == null)
            return -1;
        synchronized (mLock) {
            try {
                 mProxy.setLanguage(language);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "setLanguage:" + e);
            }
        }
        return -1;
    }
     public  int subTitleIdCallback(int event, int id) {
        Log.i(TAG, "[subTitleIdCallback]event:" + event + ",id:" + id);
        synchronized (mLock) {
            try {
                 mProxy.subTitleIdCallback(event, id);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "subTitleIdCallback:" + e);
            }
        }
        return -1;
    }

     public  int subTitleAvailableCallback(int available) {
        Log.i(TAG, "[onSubTitleAvailable]available:" + available);
        synchronized (mLock) {
            try {
                 mProxy.subTitleAvailableCallback(available);
                 return 0;
            } catch (RemoteException e) {
                Log.e(TAG, "subTitleIdCallback:" + e);
            }
        }
        return -1;
    }
    public  class HALCallback extends ISubtitleServerCallback.Stub {
        SubtitleMiddleClientManager subSerEvt;
        HALCallback(SubtitleMiddleClientManager sse) {
            subSerEvt = sse;
        }

        public void notifyCallback(SubtitleHidlParcel parcel) {
            Log.i(TAG, "notifyCallback msg type:" + parcel.msgType);
            if (subSerEvt.mEventHandler != null) {
                Message msg = subSerEvt.mEventHandler.obtainMessage(parcel.msgType, 0, 0, parcel);
                subSerEvt.mEventHandler.sendMessage(msg);
            }
        }
    }

    /*public class SubtitleHidlParcel {
        int msgType;
        int[] bodyInt;
        String[] bodyString;
        Float[] bodyFloat;
    }*/
}
