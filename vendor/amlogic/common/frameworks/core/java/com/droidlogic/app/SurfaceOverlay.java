/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SurfaceOverlay
 */

package com.droidlogic.app;

import android.os.RemoteException;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

public class SurfaceOverlay {
    static {
        System.loadLibrary("surfaceoverlay_jni");
    }

    /**
     * Sets the {@link SurfaceHolder} to use for displaying the picture
     * that show in video layer
     *
     * Either a surface holder or surface must be set if a display is needed.
     * @param sh the SurfaceHolder to use for video display
     */
    public static void setDisplay(SurfaceHolder sh) {
        Surface surface;
        if (sh != null) {
            surface = sh.getSurface();
        } else {
            surface = null;
        }
        nativeSetSurface(surface);
    }

    /*
     * Update the ImagePlayer SurfaceTexture.
     * Call after setting a new display surface.
     */
    private static native void nativeSetSurface(Surface surface);
}
