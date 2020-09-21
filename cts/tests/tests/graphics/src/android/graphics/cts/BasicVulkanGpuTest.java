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
import android.support.test.InstrumentationRegistry;
import android.test.suitebuilder.annotation.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import java.util.Arrays;

@SmallTest
@RunWith(Parameterized.class)
public class BasicVulkanGpuTest {

    static {
        System.loadLibrary("ctsgraphics_jni");
    }

    @Parameters(name = "{0}")
    public static Iterable<Object[]> data() {
        return Arrays.asList(new Object[][] {
            {"AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM", 1},
            {"AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM", 2},
            {"AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM", 3},
            {"AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM", 4},
            {"AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM", 43},
            // TODO(ericrk): Test float and non-renderable formats.
        });
    }

    private int mFormat;

    public BasicVulkanGpuTest(String name, int format) {
        mFormat = format;
    }

    @Test
    public void testBasicBufferImportAndRenderingExplicitFormat() throws Exception {
        verifyBasicBufferImport(InstrumentationRegistry.getContext().getAssets(), mFormat, false);
    }

    @Test
    public void testBasicBufferImportAndRenderingExternalFormat() throws Exception {
        verifyBasicBufferImport(InstrumentationRegistry.getContext().getAssets(), mFormat, true);
    }

    private static native void verifyBasicBufferImport(AssetManager assetManager, int format,
                                                       boolean useExternalFormat);
}
