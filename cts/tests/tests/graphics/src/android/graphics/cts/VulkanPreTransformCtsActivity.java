/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.graphics.cts;

import android.app.Activity;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;

/**
 * Activity for VulkanPreTransformTest.
 */
public class VulkanPreTransformCtsActivity extends Activity {
    static {
        System.loadLibrary("ctsgraphics_jni");
    }

    private static final String TAG = "vulkan";

    protected Surface mSurface;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate!");
        setContentView(R.layout.vulkan_pretransform_layout);
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surfaceview);
        mSurface = surfaceView.getHolder().getSurface();
    }

    public void testVulkanPreTransform(boolean setPreTransform) {
        nCreateNativeTest(getAssets(), mSurface, setPreTransform);
    }

    private static native void nCreateNativeTest(
            AssetManager manager, Surface surface, boolean setPreTransform);
}
