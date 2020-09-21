/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

package com.droidlogic.app;

import android.content.Context;
import android.os.HwBinder;
import android.os.RemoteException;
import android.graphics.Rect;
import android.graphics.Bitmap;
import java.util.NoSuchElementException;
import android.util.Log;
import android.graphics.BitmapFactory;
import java.io.File;
import android.os.Environment;
import java.lang.reflect.Method;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Arrays;

public class ScreenControlManager {
    private static final String TAG = "ScreenControlManager";

    private static final int SCREEN_CONTROL_DEATH_COOKIE = 1001;

    public static final int REMOTE_EXCEPTION = -0xffff;


    private static ScreenControlManager mInstance = null;
    private Context mContext;

    // Mutex for all mutable shared state.
    private final Object mLock = new Object();

	static {
		System.loadLibrary("screencontrol_jni");
	}

	private native void native_ConnectScreenControl();
	private native int native_ScreenCap(int left, int top, int right, int bottom, int width, int height, int sourceType, String filename);
	private native int native_ScreenRecord(int width, int height, int frameRate, int bitRate, int limitTimeSec, int sourceType, String filename);
	private native byte[] native_ScreenCapBuffer(int left, int top, int right, int bottom, int width, int height, int sourceType);

    public ScreenControlManager(Context context) {
        mContext = context;

		native_ConnectScreenControl();
    }

    public ScreenControlManager() {
		native_ConnectScreenControl();
    }

    public static ScreenControlManager getInstance() {
         if (null == mInstance) mInstance = new ScreenControlManager();
         return mInstance;
    }

    private void connectToProxy() {
    }

    public int startScreenRecord(int width, int height, int frameRate, int bitRate, int limitTimeSec, int sourceType, String filename) {
	Log.d(TAG, "startScreenRecord wdith:" + width + ",height:"+ height + ",frameRate:" + frameRate + ",bitRate:" + bitRate + ",limitTimeSec:" + limitTimeSec + ",sourceType:" + sourceType + ",filename:" + filename);
	synchronized (mLock) {
            try {
                return native_ScreenRecord(width, height, frameRate, bitRate, limitTimeSec, sourceType, filename);
            } catch (Exception e) {
                Log.e(TAG, "startScreenRecord: ScreenControlService is dead!:" + e);
            }
	}

        return REMOTE_EXCEPTION;
    }

    public int startScreenCap(int left, int top, int right, int bottom, int width, int height, int sourceType, String filename) {
        Log.d(TAG, "startScreenCap left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:" + height + ",sourceType:" + sourceType + ",filename:" + filename);
		synchronized (mLock) {
			 if (sourceType == 2) { // osd only
			 	File file = new File(filename);
		        try {
		            Class clz = Class.forName("android.view.SurfaceControl");
		            Method screenshot = clz.getMethod("screenshot", Rect.class, int.class, int.class, int.class);
		            Bitmap mBitmap = (Bitmap)screenshot.invoke(null, new Rect(left, top, right, bottom), width, height, 0);
		            //mBitmap = SurfaceControl.screenshot(new Rect(left, top, right, bottom), width, height, 0);
		            if (mBitmap != null) {
		                // Convert to a software bitmap so it can be set in an ImageView.
			            Bitmap swBitmap = mBitmap.copy(Bitmap.Config.ARGB_8888, true);
			            saveBitmapAsJPEG(swBitmap, file);
		            }
		        } catch (Exception e) {
		            e.printStackTrace();
		        }
	        } else {
	            try {
					return native_ScreenCap(left, top, right, bottom, width, height, sourceType, filename);
	            } catch (Exception e) {
	                Log.e(TAG, "startScreenCap: ScreenControlService is dead!:" + e);
	            }
	        }
	}

        return REMOTE_EXCEPTION;
    }

    public byte[] startScreenCapBuffer(int left, int top, int right, int bottom, int width, int height, int sourceType) {
	    Log.d(TAG, "startScreenCapBuffer left:" + left + ",top:"+ top + ",right:" + right + ",bottom:" + bottom + ",width:" + width + ",height:" + height + ",sourceType:" + sourceType);
		ByteBuffer byteBuffer;
		byte[] byteArray = null;
		synchronized (mLock) {
			if (sourceType == 2) { // osd only
				try {
					Class clz = Class.forName("android.view.SurfaceControl");
					Method screenshot = clz.getMethod("screenshot", Rect.class, int.class, int.class, int.class);
					Bitmap mBitmap = (Bitmap)screenshot.invoke(null, new Rect(left, top, right, bottom), width, height, 0);
					//mBitmap = SurfaceControl.screenshot(new Rect(left, top, right, bottom), width, height, 0);
					if (mBitmap != null) {
						// Convert to a software bitmap so it can be set in an ImageView.
						Bitmap swBitmap = mBitmap.copy(Bitmap.Config.ARGB_8888, true);
						byteBuffer = ByteBuffer.allocate(swBitmap.getByteCount());
						swBitmap.copyPixelsToBuffer(byteBuffer);
						byteArray = byteBuffer.array();
						return byteArray;
					}
				} catch (Exception e) {
					e.printStackTrace();
				}
			} else {
				try {
					return native_ScreenCapBuffer(left, top, right, bottom, width, height, sourceType);
				} catch (Exception e) {
					Log.e(TAG, "startScreenCapBuffer: ScreenControlService is dead!:" + e);
				}
			}

		}

	    return null;
	}

	private void saveBitmapAsJPEG(Bitmap bmp, File f) {
        try {
	        FileOutputStream out = new FileOutputStream(f);
	        bmp.compress(Bitmap.CompressFormat.JPEG, 100, out);
            out.flush();
            out.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    final class DeathRecipient implements HwBinder.DeathRecipient {
        DeathRecipient() {
        }

        @Override
        public void serviceDied(long cookie) {
            if (SCREEN_CONTROL_DEATH_COOKIE == cookie) {
                Log.e(TAG, "screen control service died cookie: " + cookie);
            }
        }
    }
}
