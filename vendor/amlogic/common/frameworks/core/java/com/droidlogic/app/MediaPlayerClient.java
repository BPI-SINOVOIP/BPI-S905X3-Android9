/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Author:  XiaoLiang.Wang <xiaoliang.wang@amlogic.com>
 * Date: 20160205
 */
package com.droidlogic.app;

import android.os.IBinder;
import android.os.Parcel;
import android.util.Log;

import com.droidlogic.app.MediaPlayerExt;

/** @hide */
public class MediaPlayerClient extends IMediaPlayerClient.Stub {
    private static final String TAG = "MediaPlayerClient";
    private static final boolean DEBUG = false;
    private MediaPlayerExt mMp;

    public MediaPlayerClient(MediaPlayerExt mp) {
        mMp = mp;
    }

    static IBinder create(MediaPlayerExt mp) {
        return (new MediaPlayerClient(mp)).asBinder();
    }

    public void notify(int msg, int ext1, int ext2, Parcel parcel) {
        if (DEBUG) Log.i(TAG, "[notify] msg:" + msg + ", ext1:" + ext1 + ", ext2:" + ext2 + ", parcel:" + parcel);
        mMp.postEvent(msg, ext1, ext2, parcel);
    }
}