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

import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class MediaVulkanGpuTest {

    static {
        System.loadLibrary("ctsgraphics_jni");
    }

    private int[] getFramePixels() throws Exception {
        AssetManager assets = InstrumentationRegistry.getContext().getAssets();
        Bitmap bitmap = BitmapFactory.decodeStream(assets.open("test_video_frame.png"));
        int[] framePixels = new int[bitmap.getWidth() * bitmap.getHeight()];
        bitmap.getPixels(framePixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(),
                         bitmap.getHeight());
        return framePixels;
    }

    @Test
    public void testMediaImportAndRendering() throws Exception {
        loadMediaAndVerifyFrameImport(InstrumentationRegistry.getContext().getAssets(),
                                      "test_video.mp4", getFramePixels());
    }

    private static native void loadMediaAndVerifyFrameImport(AssetManager assetManager,
                                                             String filename,
                                                             int[] referencePixels);
}
