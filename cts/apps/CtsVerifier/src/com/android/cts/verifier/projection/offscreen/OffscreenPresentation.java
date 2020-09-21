/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.cts.verifier.projection.offscreen;

import android.content.Context;
import android.os.Bundle;
import android.view.Display;

import com.android.cts.verifier.R;
import com.android.cts.verifier.projection.ProjectedPresentation;

/**
 * Draws a presentation that will be interacted with fake events while the screen is off
 */
public class OffscreenPresentation extends ProjectedPresentation {

    public OffscreenPresentation(Context outerContext, Display display) {
        super(outerContext, display);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.poa_touch);
    }

}
