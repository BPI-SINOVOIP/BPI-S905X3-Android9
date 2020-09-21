/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiInManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.view.Surface;

public class HdmiInManager {
    private static final String TAG = "HdmiInManager";
    private Context mContext = null;

    private SystemControlManager mSystenControl;

    static {
        System.loadLibrary("hdmiin");
    }

    public HdmiInManager(Context context){
        mContext = context;

        mSystenControl = SystemControlManager.getInstance();
    }

    private boolean getHdmiInEnable() {
        return mSystenControl.getPropertyBoolean("ro.sys.hdmiin.enable",false);
    }

    /**
     * @hide
     */
    public void init(int source, boolean isFullscreen) {
        if (getHdmiInEnable())
            _init(source, isFullscreen);
    }

    /**
     * @hide
     */
    public void deinit() {
        if (getHdmiInEnable())
            _deinit();
    }

    /**
     * @hide
     */
    public int displayHdmi() {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _displayHdmi();
        return ret;
    }

    /**
     * @hide
     */
    public int displayAndroid() {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _displayAndroid();
        return ret;
    }

    /**
     * @hide
     */
    public void displayPip(int x, int y, int width, int height) {
        if (getHdmiInEnable())
            _displayPip(x, y, width, height);
    }

    /**
     * @hide
     */
    public int getHActive() {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _getHActive();
        return ret;
    }

    /**
     * @hide
     */
    public int getVActive() {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _getVActive();
        return ret;
    }

    /**
     * @hide
     */
    public String getHdmiInSize() {
        String ret = "";
        if (getHdmiInEnable())
            ret = _getHdmiInSize();
        return ret;
    }

    /**
     * @hide
     */
    public boolean isDvi() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _isDvi();
        return ret;
    }

    /**
     * @hide
     */
    public boolean isPowerOn() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _isPowerOn();
        return ret;
    }

    /**
     * @hide
     */
    public boolean isEnable() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _isEnable();
        return ret;
    }

    /**
     * @hide
     */
    public boolean isInterlace() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _isInterlace();
        return ret;
    }

    /**
     * @hide
     */
    public boolean hdmiPlugged() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _hdmiPlugged();
        return ret;
    }

    /**
     * @hide
     */
    public boolean hdmiSignal() {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _hdmiSignal();
        return ret;
    }

    /**
     * @hide
     */
    public void enableAudio(int flag) {
        if (getHdmiInEnable())
            _enableAudio(flag);
    }

    /**
     * @hide
     */
    public int handleAudio() {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _handleAudio();
        return ret;
    }

    /**
     * @hide
     */
    public void startMonitorUsbHostBusThread() {
        if (getHdmiInEnable())
            _startMonitorUsbHostBusThread();
    }

    /**
     * @hide
    */
    public int setAmaudioMusicGain(int gain) {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _setAmaudioMusicGain(gain);
        return ret;
    }

    /**
        * @hide
    */
    public int setAmaudioLeftGain(int gain) {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _setAmaudioLeftGain(gain);
        return ret;
    }

    /**
     * @hide
    */
    public int setAmaudioRightGain(int gain) {
        int ret = -1;
        if (getHdmiInEnable())
            ret = _setAmaudioRightGain(gain);
        return ret;
    }

    /**
     * @hide
     */
    public void setEnable(boolean enable) {
        if (getHdmiInEnable())
            _setEnable(enable);
    }

    /**
     * @hide
     */
    public void setMainWindowPosition(int x, int y) {
        if (getHdmiInEnable())
            _setMainWindowPosition(x, y);
    }

    /**
     * @hide
     */
    public void setMainWindowFull() {
        if (getHdmiInEnable())
            _setMainWindowFull();
    }

    /**
     * @hide
     */
    public int setSourceType() {
        int ret = -1;
        if(getHdmiInEnable())
            ret = _setSourceType();
        return ret;
    }

    /**
     * @hide
     */
    public boolean isSurfaceAvailable(Surface surface) {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _isSurfaceAvailable(surface);
        return ret;
    }

    /**
     * @hide
     */
    public void displayOSD(int width, int height) {
        if (getHdmiInEnable())
            _displayOSD(width, height);
    }

    /**
     * @hide
     */
    public boolean setPreviewWindow(Surface surface) {
        boolean ret = false;
        if (getHdmiInEnable())
            ret = _setPreviewWindow(surface);
        return ret;
    }

    /**
     * @hide
     */
    public int setCrop(int x, int y, int width, int height){
        int ret = -1;
        if (getHdmiInEnable())
            ret = _setCrop(x, y, width, height);
        return ret;
    }

    /**
     * @hide
     */
    public void startMov() {
        if (getHdmiInEnable())
            _startMov();
    }

    /**
     * @hide
     */
    public void stopMov() {
        if (getHdmiInEnable())
            _stopMov();
    }

    /**
     * @hide
     */
    public void pauseMov() {
        if (getHdmiInEnable())
            _pauseMov();
    }

    /**
     * @hide
     */
    public void resumeMov() {
        if (getHdmiInEnable())
            _resumeMov();
    }

    /**
     * @hide
     */
    public void startVideo() {
        if (getHdmiInEnable())
            _startVideo();
    }

    /**
     * @hide
     */
    public void stopVideo() {
        if (getHdmiInEnable())
            _stopVideo();
    }

    private native void _init(int source, boolean isFullscreen);
    private native void _deinit();
    private native int _displayHdmi();
    private native int _displayAndroid();
    private native void _displayPip(int x, int y, int width, int height);
    private native int _getHActive();
    private native int _getVActive();
    private native String _getHdmiInSize();
    private native boolean _isDvi();
    private native boolean _isPowerOn();
    private native boolean _isEnable();
    private native boolean _isInterlace();
    private native boolean _hdmiPlugged();
    private native boolean _hdmiSignal();
    private native void _enableAudio(int flag);
    private native int _handleAudio();
    private native void _startMonitorUsbHostBusThread();
    private native int _setAmaudioMusicGain(int gain);
    private native int _setAmaudioLeftGain(int gain);
    private native int _setAmaudioRightGain(int gain);
    private native void _setEnable(boolean enable);
    private native void _setMainWindowPosition(int x, int y);
    private native void _setMainWindowFull();
    private native int _setSourceType();
    private native void _displayOSD(int width, int height);
    private native boolean _isSurfaceAvailable(Surface surface);
    private native boolean _setPreviewWindow(Surface surface);
    private native int _setCrop(int x, int y, int width, int height);
    private native void _startMov();
    private native void _stopMov();
    private native void _pauseMov();
    private native void _resumeMov();
    private native void _startVideo();
    private native void _stopVideo();
}

