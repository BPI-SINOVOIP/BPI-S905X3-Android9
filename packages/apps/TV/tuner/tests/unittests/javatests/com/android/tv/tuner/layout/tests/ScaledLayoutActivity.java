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

package com.android.tv.tuner.layout.tests;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.FrameLayout;
import com.android.tv.tuner.layout.ScaledLayout;

public class ScaledLayoutActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_scaled_layout_test);

        FrameLayout frameLayout = findViewById(R.id.root_layout);
        frameLayout.addView(createScaledLayoutTestcaseLayout());
        frameLayout.addView(createScaledLayoutTestcaseBounceY());
    }

    private ScaledLayout createScaledLayoutTestcaseLayout() {
        ScaledLayout scaledLayout = new ScaledLayout(this);
        scaledLayout.setLayoutParams(new FrameLayout.LayoutParams(100, 100));

        View view1 = new View(this);
        view1.setId(R.id.view1);
        view1.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0f, 0.5f, 0f, 0.5f));

        View view2 = new View(this);
        view2.setId(R.id.view2);
        view2.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0f, 0.5f, 0.5f, 1f));

        View view3 = new View(this);
        view3.setId(R.id.view3);
        view3.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0.5f, 1f, 0f, 0.5f));

        View view4 = new View(this);
        view4.setId(R.id.view4);
        view4.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0.5f, 1f, 0.5f, 1f));

        scaledLayout.addView(view1);
        scaledLayout.addView(view2);
        scaledLayout.addView(view3);
        scaledLayout.addView(view4);

        return scaledLayout;
    }

    private ScaledLayout createScaledLayoutTestcaseBounceY() {
        ScaledLayout scaledLayout = new ScaledLayout(this);
        scaledLayout.setLayoutParams(new FrameLayout.LayoutParams(100, 100));

        View view1 = new View(this);
        view1.setId(R.id.view1);
        view1.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0.7f, 0.9f, 0f, 1f));

        View view2 = new View(this);
        view2.setId(R.id.view2);
        view2.setLayoutParams(new ScaledLayout.ScaledLayoutParams(0.8f, 1f, 0f, 1f));

        scaledLayout.addView(view1);
        scaledLayout.addView(view2);

        return scaledLayout;
    }
}
