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

package com.google.android.car.kitchensink.activityview;

import android.app.ActivityView;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.google.android.car.kitchensink.R;

import java.util.Random;

/**
 * This fragment exercises the capabilities of virtual display.
 */
public class ActivityViewTestFragment extends Fragment {
    private ActivityView mActivityView;
    private ViewGroup mActivityViewParent;

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View root = inflater.inflate(R.layout.activity_view_test_fragment, container, false);

        root.findViewById(R.id.av_launch_activity)
                .setOnClickListener(this::onLaunchActivityClicked);
        root.findViewById(R.id.av_resize)
                .setOnClickListener(this::onResizeVirtualDisplayClicked);

        mActivityView = root.findViewById(R.id.av_activityview);
        mActivityViewParent = ((ViewGroup) mActivityView.getParent());

        return root;
    }

    private void onResizeVirtualDisplayClicked(View v) {
        mActivityViewParent.setPadding(20, 20, 20,
                new Random(SystemClock.elapsedRealtimeNanos()).nextInt(200));
    }

    private void onLaunchActivityClicked(View v) {
        Intent intent = new Intent();
        intent.setComponent(ComponentName.createRelative("com.android.settings", ".Settings"));
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivityView.startActivity(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mActivityView.release();
    }
}
