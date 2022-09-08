package com.example.SubtitleSimpleDemo;

import android.util.Log;

public class SubtitleAPI {

    public static final int E_SUBTITLE_FMQ   = 0;   /* for soft support subtitle data, use hidl FastMessageQueue */
    public static final int E_SUBTITLE_DEV   = 1;      /* use /dev/amstream_sub_read as the IO source */
    public static final int E_SUBTITLE_FILE  = 2;      /* for external subtitle file */
    public static final int E_SUBTITLE_SOCK  = 3;      /* deprecated, android not permmit to use anymore */
    public static final int E_SUBTITLE_DEMUX = 4;     /* use aml hwdemux as the data source */

    private static final String TAG = "SubtitleAPI";
    private long mHandle;
    static {
        try {
            System.loadLibrary("subtitleApiTestJni");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Error load:", e);
        }
    }
    SubtitleAPI() {
        mHandle = 0;
    }

    public long subtitleCreate() {
        mHandle = nativeSubtitleCreate();
        return mHandle;
    }

    // DVB subtitles, can for dvb, teletext, scte27
    public boolean subtitleDvbSubs(int ioType, int SubType, int pid, int ancId, int cmpositionId) {
        Log.d(TAG, "subtitleDvbSubs:" + Long.toHexString(mHandle));
        return nativeSubtitleOpen(mHandle, ioType, SubType, pid, 0, 0, ancId, cmpositionId);
    }

    // for CC subtitle
    public boolean subtitleCcSubs(int ioType, int SubType, int videoFmt, int channelId) {
        Log.d(TAG, "subtitleCcSubs:" + Long.toHexString(mHandle));
        return nativeSubtitleOpen(mHandle, ioType, SubType, 0, videoFmt, channelId, 0, 0);
    }

    public boolean show() {
        Log.d(TAG, "show:" + Long.toHexString(mHandle));
        return nativeShow(mHandle);
    }

    public boolean hide() {
        Log.d(TAG, "hide:" + Long.toHexString(mHandle));
        return nativeHide(mHandle);
    }

    public boolean setDisplayRect(int x, int y, int w, int h) {
        Log.d(TAG, "setDisplayRect:" + Long.toHexString(mHandle));
        return nativeDisplayRect(mHandle, x, y, w, h);
    }

    public boolean subtitleDestroy() {
        Log.d(TAG, "subtitleDestroy:" + Long.toHexString(mHandle));
        return nativeSubtitleDestroy(mHandle);
    }

    public boolean subtitleClose() {
        Log.d(TAG, "subtitleClose:" + Long.toHexString(mHandle));
        return nativeSubtitleClose(mHandle);
    }

    public boolean teletextGotoPage(int pageNo, int subPageNo) {
        return nativeTTgotoPage(mHandle, pageNo, subPageNo);
    }

    public boolean teletextGoHome() {
        return nativeTTgoHome(mHandle);
    }

    public boolean teletextNextPage(boolean isIncrease) {
        return nativeTTnextPage(mHandle, isIncrease);
    }

    public boolean teletextNextSubPage(boolean isIncrease) {
        return nativeTTnextSubPage(mHandle, isIncrease);
    }

    private native long nativeSubtitleCreate();
    private native boolean nativeSubtitleDestroy(long handle);
    private native boolean nativeSubtitleOpen(long handle, int ioType, int SubType,
           int pid, int videoFmt, int channelId, int ancId, int cmpositionId);
    private native boolean nativeSubtitleClose(long handle);
    private native boolean nativeShow(long handle);
    private native boolean nativeHide(long handle);
    private native boolean nativeDisplayRect(long handle, int x, int y, int w, int h);

    private native boolean nativeTTgoHome(long handle);
    private native boolean nativeTTnextPage(long handle, boolean dir);
    private native boolean nativeTTnextSubPage(long handle, boolean dir);
    private native boolean nativeTTgotoPage(long handle, int pageNo, int subPageNo);

}
