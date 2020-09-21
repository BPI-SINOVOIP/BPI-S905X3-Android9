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

package com.android.tv.droidlogic.channelui;

import android.os.Bundle;
import android.util.Log;

import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SideFragmentManager;

public abstract class DynamicSubMenuItem extends DynamicActionItem {
    private final static String TAG = "DynamicSubMenuItem";
    private static final boolean DEBUG = false;
    private final SideFragmentManager mSideFragmentManager;
    private String mTitle;
    private long mType;
    private Bundle mBundle;

    public DynamicSubMenuItem(String title, String description, long num, SideFragmentManager fragmentManager) {
        this(title, description, num, 0, fragmentManager);
    }

    public DynamicSubMenuItem(String title, String description, long num, int iconId,
            SideFragmentManager fragmentManager) {
        super(title, description, iconId);
        mSideFragmentManager = fragmentManager;
        this.mType = num;
        this.mTitle = mTitle;
    }

    @Override
    protected void onSelected() {
        launchFragment();
    }

    public void setBundle(Bundle bundle) {
        mBundle = bundle;
    }

    protected void launchFragment() {
        SideFragment sidefragment = getFragment();
        if (mBundle == null) {
            mBundle = new Bundle();
        }
        mBundle.putLong("type", mType);
        mBundle.putString("title", mTitle);
        if (DEBUG) Log.d(TAG, "[getFragment] mTitle = " + mTitle + ", mType = "+ mType);
        sidefragment.setArguments(mBundle);
        mSideFragmentManager.show(sidefragment);
    }

    protected abstract SideFragment getFragment();
}
