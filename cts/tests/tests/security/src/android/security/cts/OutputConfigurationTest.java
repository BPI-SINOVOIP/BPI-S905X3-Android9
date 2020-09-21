/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.security.cts;

import android.graphics.SurfaceTexture;
import android.hardware.camera2.params.OutputConfiguration;
import android.os.Parcel;
import android.platform.test.annotations.SecurityTest;
import android.test.AndroidTestCase;
import android.util.Size;
import android.view.Surface;
import android.view.TextureView;

/**
 * Verify that OutputConfiguration's fields propagate through parcel properly.
 */
@SecurityTest
public class OutputConfigurationTest extends AndroidTestCase {
    public void testSharedSurfaceOutputConfigurationBasic() throws Exception {
        SurfaceTexture outputTexture = new SurfaceTexture(/* random texture ID */ 5);
        Surface surface = new Surface(outputTexture);

        //Test OutputConfiguration with a surface.
        OutputConfiguration outputConfig = new OutputConfiguration(surface);
        outputConfig.enableSurfaceSharing();
        Parcel p;
        p = Parcel.obtain();
        outputConfig.writeToParcel(p, 0);
        p.setDataPosition(0);
        OutputConfiguration parcelledOutput = OutputConfiguration.CREATOR.createFromParcel(p);
        assertEquals("Number of surfaces should be 1",
                parcelledOutput.getSurfaces().size(),
                1);
        assertEquals("Number of surfaces shouldn't change",
                parcelledOutput.getSurfaces().size(),
                outputConfig.getSurfaces().size());
        assertEquals("surfaceGroupId shouldn't change",
                parcelledOutput.getSurfaceGroupId(),
                outputConfig.getSurfaceGroupId());
        // addSurface shouldn't throw exception because surface sharing is enabled.
        SurfaceTexture outputTexture2 = new SurfaceTexture(/* random texture ID */ 6);
        Surface surface2 = new Surface(outputTexture2);
        parcelledOutput.addSurface(surface2);
        p.recycle();

        //Test OutputConfiguration with surface group id.
        outputConfig = new OutputConfiguration(
                /* random surface groupd id */1, surface);
        p = Parcel.obtain();
        outputConfig.writeToParcel(p, 0);
        p.setDataPosition(0);
        parcelledOutput = OutputConfiguration.CREATOR.createFromParcel(p);
        assertEquals("Number of surfaces should be 1",
                parcelledOutput.getSurfaces().size(),
                1);
        assertEquals("Number of surfaces shouldn't change",
                parcelledOutput.getSurfaces().size(),
                outputConfig.getSurfaces().size());
        assertEquals("surfaceGroupdId shouldn't change",
                parcelledOutput.getSurfaceGroupId(),
                outputConfig.getSurfaceGroupId());
        try {
            parcelledOutput.addSurface(surface);
            fail("should get IllegalStateException due to OutputConfiguration not shared");
        } catch (IllegalStateException e) {
            // expected exception
        }
        p.recycle();

        //Test OutputConfiguration with deferred surface.
        Size surfaceSize = new Size(10, 10);
        outputConfig = new OutputConfiguration(
                surfaceSize, SurfaceTexture.class);
        p = Parcel.obtain();
        outputConfig.writeToParcel(p, 0);
        p.setDataPosition(0);
        parcelledOutput = OutputConfiguration.CREATOR.createFromParcel(p);
        assertEquals("Number of surfaces should be 0",
                parcelledOutput.getSurfaces().size(), 0);
        assertEquals("Number of surfaces shouldn't change",
                parcelledOutput.getSurfaces().size(),
                outputConfig.getSurfaces().size());
        assertEquals("surfaceGroupdId shouldn't change",
                parcelledOutput.getSurfaceGroupId(),
                outputConfig.getSurfaceGroupId());
        p.recycle();
    }
}
