/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.camera.one.v2.autofocus;

import android.graphics.Rect;
import android.hardware.camera2.CaptureRequest;
import android.hardware.camera2.params.MeteringRectangle;

import javax.annotation.ParametersAreNonnullByDefault;

@ParametersAreNonnullByDefault
public interface MeteringParameters {
    /**
     * @param cropRegion The current crop region, see
     *            {@link CaptureRequest#SCALER_CROP_REGION}.
     * @return The current auto-focus metering regions.
     */
    public MeteringRectangle[] getAFRegions(Rect cropRegion);

    /**
     * @param cropRegion The current crop region, see
     *            {@link CaptureRequest#SCALER_CROP_REGION}.
     * @return The current auto-exposure metering regions.
     */
    public MeteringRectangle[] getAERegions(Rect cropRegion);
}
