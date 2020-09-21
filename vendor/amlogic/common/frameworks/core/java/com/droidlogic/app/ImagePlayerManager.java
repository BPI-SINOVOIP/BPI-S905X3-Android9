/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC ImagePlayerManager
 */

package com.droidlogic.app;

import android.content.Context;
import android.net.Uri;
import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

public class ImagePlayerManager {
    private static final String TAG                 = "ImagePlayerManager";

    private static final String IMAGE_TOKEN         = "droidlogic.IImagePlayerService";
    public static final int ARGUMENTS_ERROR         = -0xfffe;
    public static final int REMOTE_EXCEPTION        = -0xffff;
    int TRANSACTION_INIT                            = IBinder.FIRST_CALL_TRANSACTION;
    int TRANSACTION_SET_DATA_SOURCE                 = IBinder.FIRST_CALL_TRANSACTION + 1;
    int TRANSACTION_SET_SAMPLE_SURFACE_SIZE         = IBinder.FIRST_CALL_TRANSACTION + 2;
    int TRANSACTION_SET_ROTATE                      = IBinder.FIRST_CALL_TRANSACTION + 3;
    int TRANSACTION_SET_SCALE                       = IBinder.FIRST_CALL_TRANSACTION + 4;
    int TRANSACTION_SET_ROTATE_SCALE                = IBinder.FIRST_CALL_TRANSACTION + 5;
    int TRANSACTION_SET_CROP_RECT                   = IBinder.FIRST_CALL_TRANSACTION + 6;
    int TRANSACTION_START                           = IBinder.FIRST_CALL_TRANSACTION + 7;
    int TRANSACTION_PREPARE                         = IBinder.FIRST_CALL_TRANSACTION + 8;
    int TRANSACTION_SHOW                            = IBinder.FIRST_CALL_TRANSACTION + 9;
    int TRANSACTION_RELEASE                         = IBinder.FIRST_CALL_TRANSACTION + 10;
    int TRANSACTION_PREPARE_BUF                     = IBinder.FIRST_CALL_TRANSACTION + 11;
    int TRANSACTION_SHOW_BUF                        = IBinder.FIRST_CALL_TRANSACTION + 12;
    int TRANSACTION_SET_DATA_SOURCE_URL             = IBinder.FIRST_CALL_TRANSACTION + 13;
    int TRANSACTION_NOTIFY_PROCESSDIED              = IBinder.FIRST_CALL_TRANSACTION + 14;
    int TRANSACTION_SET_TRANSLATE                   = IBinder.FIRST_CALL_TRANSACTION + 15;
    int TRANSACTION_SET_HWSCALE                     = IBinder.FIRST_CALL_TRANSACTION + 16;

    private Context mContext;
    private IBinder mIBinder = null;
    public ImagePlayerManager(Context context) {
        mContext = context;

        try {
            Object object = Class.forName("android.os.ServiceManager")
                    .getMethod("getService", new Class[] { String.class })
                    .invoke(null, new Object[] { "image.player" });
            mIBinder = (IBinder)object;
        }
        catch (Exception ex) {
            Log.e(TAG, "image player init fail:" + ex);
        }

        init();
        notifyProcessDied(new Binder());
    }

    private int init() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_INIT,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "init: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int  notifyProcessDied(IBinder cb) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeInt((cb != null)?1:0);
                if (cb != null) {
                    data.writeStrongBinder(cb);
                }
                mIBinder.transact(TRANSACTION_NOTIFY_PROCESSDIED,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "notifyProcessDied: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    private IBinder getHttpServiceBinder(String url) {
        /* can not get method, because method is package visiable region
        try {
            Object object = Class.forName("android.media.MediaHTTPService")
                    .getMethod("createHttpServiceBinderIfNecessary", new Class[] { String.class })
                    .invoke(null, new Object[] { url });
            return (IBinder)object;
        }
        catch (Exception ex) {
            Log.e(TAG, "get http service binder fail:" + ex);
        }

        return null;
        */

        //return (new android.media.MediaHTTPService()).asBinder();
        return null;
    }

    //url start with http:// or https://
    public int setDataSourceURL(String url) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);

                IBinder httpBinder = getHttpServiceBinder(url);
                data.writeInt((httpBinder != null)?1:0);
                if (httpBinder != null) {
                    data.writeStrongBinder(httpBinder);
                }
                data.writeString(url);
                mIBinder.transact(TRANSACTION_SET_DATA_SOURCE_URL,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "_setDataSource: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    //only used by local file
    public int setDataSource(String path) {
        if (path.startsWith("http://") || path.startsWith("https://"))
            return setDataSourceURL(path);//it is a network picture

        if (!path.startsWith("file://")) {
            path = "file://" + path;
        }

        return _setDataSource(path);
    }

    private int _setDataSource(String path) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeString(path);
                mIBinder.transact(TRANSACTION_SET_DATA_SOURCE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "_setDataSource: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setSampleSurfaceSize(int sampleSize, int surfaceW, int surfaceH) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeInt(sampleSize);
                data.writeInt(surfaceW);
                data.writeInt(surfaceH);
                mIBinder.transact(TRANSACTION_SET_SAMPLE_SURFACE_SIZE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setSampleSurfaceSize: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setRotate(float degrees, int autoCrop) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeFloat(degrees);
                data.writeInt(autoCrop);
                mIBinder.transact(TRANSACTION_SET_ROTATE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setRotate: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setScale(float sx, float sy, int autoCrop) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeFloat(sx);
                data.writeFloat(sy);
                data.writeInt(autoCrop);
                mIBinder.transact(TRANSACTION_SET_SCALE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setScale: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setHWScale(float sc) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeFloat(sc);
                mIBinder.transact(TRANSACTION_SET_HWSCALE, data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setHWScale: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setTranslate (float tx, float ty) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeFloat(tx);
                data.writeFloat(ty);
                mIBinder.transact(TRANSACTION_SET_TRANSLATE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setTranslate: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setRotateScale(float degrees, float sx, float sy, int autoCrop) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeFloat(degrees);
                data.writeFloat(sx);
                data.writeFloat(sy);
                data.writeInt(autoCrop);
                mIBinder.transact(TRANSACTION_SET_ROTATE_SCALE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setRotateScale: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int setCropRect(int cropX, int cropY, int cropWidth, int cropHeight) {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeInt(cropX);
                data.writeInt(cropY);
                data.writeInt(cropWidth);
                data.writeInt(cropHeight);
                mIBinder.transact(TRANSACTION_SET_CROP_RECT,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setCropRect: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int start() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_START,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int prepare() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_PREPARE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int show() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_SHOW,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "start: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int release() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_RELEASE,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "release: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int prepareBuf(String path) {
        return _prepareBuf("file://" + path);
    }

    private int _prepareBuf(String path){
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                data.writeString(path);
                mIBinder.transact(TRANSACTION_PREPARE_BUF,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "prepareBuf: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    public int showBuf() {
        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(IMAGE_TOKEN);
                mIBinder.transact(TRANSACTION_SHOW_BUF,
                                        data, reply, 0);
                int result = reply.readInt();
                reply.recycle();
                data.recycle();
                return result;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "release: ImagePlayerService is dead!:" + ex);
        }

        return REMOTE_EXCEPTION;
    }

    /**
     * Sets the {@link SurfaceHolder} to use for displaying the picture
     * that show in video layer
     *
     * Either a surface holder or surface must be set if a display is needed.
     * @param sh the SurfaceHolder to use for video display
     */
    public void setDisplay(SurfaceHolder sh) {
        SurfaceOverlay.setDisplay(sh);
    }
}
