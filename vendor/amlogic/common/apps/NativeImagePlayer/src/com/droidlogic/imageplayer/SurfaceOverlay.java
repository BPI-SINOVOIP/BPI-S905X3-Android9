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

import android.view.Surface;
import android.view.SurfaceHolder;

public class SurfaceOverlay {
    static {
        System.loadLibrary("surfaceoverlay_jni");
    }

    /**
     * Sets the {@link SurfaceHolder} to use for displaying the picture
     * that show in video layer
     * <p>
     * Either a surface holder or surface must be set if a display is needed.
     *
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
