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

package com.android.tv.settings.connectivity.setup;

import android.annotation.Nullable;
import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.support.v17.leanback.app.GuidedStepSupportFragment;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v17.leanback.widget.VerticalGridView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.GuidedActionsAlignUtil;


/**
 * Subclass of {@link GuidedStepSupportFragment} used in settings wifi setup.
 */
public class WifiConnectivityGuidedStepFragment extends GuidedStepSupportFragment {

    @Override
    protected void onProvideFragmentTransitions() {
        setEnterTransition(null);
        setExitTransition(null);
    }

    @Override
    public GuidanceStylist onCreateGuidanceStylist() {
        return new GuidanceStylist() {
            @Override
            public int onProvideLayoutId() {
                return R.layout.wifi_content;
            }
        };
    }

    @Override
    public GuidedActionsStylist onCreateActionsStylist() {
        return new GuidedActionsStylist() {
            @Override
            public ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
                LayoutInflater inflater = LayoutInflater.from(parent.getContext());
                View v = inflater.inflate(onProvideItemLayoutId(viewType), parent, false);
                return new GuidedActionsAlignUtil.SetupViewHolder(v);
            }
        };
    }

    /**
     * Get resources safely.
     *
     * @return resources.
     */
    @Nullable
    public Resources getResourcesSafely() {
        final Activity activity = getActivity();
        if (activity == null || activity.isFinishing()) {
            return null;
        }
        return activity.getResources();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        // action_fragment_root's padding cannot be set via attributes so we do it programmatically.
        final View actionFragmentRoot = view.findViewById(R.id.action_fragment_root);
        if (actionFragmentRoot != null) {
            actionFragmentRoot.setPadding(0, 0, 0, 0);
        }
        final Resources resources = getResourcesSafely();
        if (resources == null) {
            return view;
        }

        final VerticalGridView gridView = getGuidedActionsStylist().getActionsGridView();
        gridView.setItemSpacing(
                resources.getDimensionPixelSize(R.dimen.setup_list_item_margin));
        GuidedActionsAlignUtil.align(getGuidedActionsStylist());
        return view;
    }
}
