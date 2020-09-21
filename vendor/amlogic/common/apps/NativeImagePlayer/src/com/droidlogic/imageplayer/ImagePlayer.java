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
package com.droidlogic.imageplayer;

import android.content.Context;
import android.os.HwBinder;
import android.os.RemoteException;
import android.util.Log;
import android.view.SurfaceHolder;

import vendor.amlogic.hardware.imageserver.V1_0.IImageService;


public class ImagePlayer {
    private static final String TAG = "ImagePlayer";
    public static final int REMOTE_EXCEPTION = -1;
    private static final int IMAGE_PLAYER_DEATH_COOKIE = 1000;
    private IImageService mProxy;
    private ImagePlayerDeathRecipient mDeathRecipient;

    /**
     * Method to create a ImagePlayer,use {@link #init()} function to init image_player_service
     *
     * @param context  maybe useful someday
     * @param callback interface
     */
    public ImagePlayer() {
        Log.d(TAG, "create ImagePlayer");
        mDeathRecipient = new ImagePlayerDeathRecipient();
        init();
    }

    private void init() {
        try {
            mProxy = IImageService.getService();
            if (null != mProxy) {
                mProxy.init();
                mProxy.linkToDeath(mDeathRecipient, IMAGE_PLAYER_DEATH_COOKIE);
            }
        } catch (Exception ex) {
            Log.e(TAG, "image player getService fail:" + ex);
        }
    }

    public int setDataSource(String path) {
        try {
            if (null != mProxy) {
                return mProxy.setDataSource(getFinalPath(path));
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setDataSource: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    private String getFinalPath(String path) {
        String finalPath = "";
        if (!path.startsWith("file://") && !(path.startsWith("http://") || path.startsWith("https://"))) {
            finalPath = "file://" + path;
        } else {
            finalPath = path;
        }
        return finalPath;
    }

    public int setSampleSurfaceSize(int sampleSize, int surfaceW, int surfaceH) {
        try {
            if (null != mProxy) {
                return mProxy.setSampleSurfaceSize(sampleSize, surfaceW, surfaceH);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setSampleSurfaceSize: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setRotate(float degrees, int autoCrop) {
        try {
            if (null != mProxy) {
                return mProxy.setRotate(degrees, autoCrop);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setRotate: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setScale(float sx, float sy, int autoCrop) {
        try {
            if (null != mProxy) {
                return mProxy.setScale(sx, sy, autoCrop);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setScale: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setHWScale(float sc) {
        try {
            if (null != mProxy) {
                return mProxy.setHWScale(sc);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setHWScale: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setTranslate(float tx, float ty) {
        try {
            if (null != mProxy) {
                return mProxy.setTranslate(tx, ty);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setTranslate: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setRotateScale(float degrees, float sx, float sy, int autoCrop) {
        try {
            if (null != mProxy) {
                return mProxy.setRotateScale(degrees, sx, sy, autoCrop);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setRotateScale: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int setCropRect(int cropX, int cropY, int cropWidth, int cropHeight) {
        try {
            if (null != mProxy) {
                return mProxy.setCropRect(cropX, cropY, cropWidth, cropHeight);
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setCropRect: ImagePlayerService is dead!:" + ex);
        }
        return REMOTE_EXCEPTION;
    }

    public int start() {
        int ret = REMOTE_EXCEPTION;
        try {
            if (null != mProxy) {
                ret = mProxy.start();
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }
        return ret;
    }

    /**
     * Prepares the ImagePlayer buffer for the picture.
     * <p>
     * After setting the datasource and the display surface, you need to
     * call prepare() to prepare buffer for the show.
     */
    public int prepare() {
        int ret = REMOTE_EXCEPTION;
        try {
            if (null != mProxy) {
                ret = mProxy.prepare();
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }
        return ret;
    }

    public int show() {
        int ret = REMOTE_EXCEPTION;
        try {
            if (null != mProxy) {
                ret = mProxy.show();
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }
        return ret;
    }

    public int showImage(String path) {
        int ret = REMOTE_EXCEPTION;
        try {
            if (null != mProxy) {
                ret = mProxy.showImage(getFinalPath(path));
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }
        return ret;
    }

    public int release() {
        int ret = REMOTE_EXCEPTION;
        try {
            if (null != mProxy) {
                ret = mProxy.release();
                mProxy.unlinkToDeath(mDeathRecipient);
                return ret;
            }
        } catch (Exception ex) {
            Log.e(TAG, "release fails:" + ex);
        }
        return ret;
    }

    /**
     * Sets the {@link SurfaceHolder} to use for displaying the picture
     * that show in video layer
     * <p>
     * Either a surface holder or surface must be set if a display is needed.
     *
     * @param sh the SurfaceHolder to use for video display
     */
    public void setDisplay(SurfaceHolder sh) {
        SurfaceOverlay.setDisplay(sh);
    }

    /**
     * Interface definition for a callback to be invoked when the display showing.
     * this function is for apk ui.you can remove it.
     */
    public interface ImagePlayerListener {
        public void onPrepared();

        public void onPlaying();

        public void onStoped();

        public void onShow();

        public void relseased();
    }

    final class ImagePlayerDeathRecipient implements HwBinder.DeathRecipient {
        @Override
        public void serviceDied(long cookie) {
            if (IMAGE_PLAYER_DEATH_COOKIE == cookie) {
                Log.e(TAG, "imageplayer service died, try to connect it again.");
                init();
            }
        }
    }
}
