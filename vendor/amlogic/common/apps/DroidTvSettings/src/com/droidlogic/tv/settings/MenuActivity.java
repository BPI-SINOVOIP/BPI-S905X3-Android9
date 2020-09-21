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

package com.droidlogic.tv.settings;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.drawable.Drawable;
import android.support.v17.leanback.app.BrowseFragment;
import android.support.v17.leanback.widget.ListRow;
import android.support.v17.leanback.widget.ListRowPresenter;
import android.support.v17.leanback.widget.ClassPresenterSelector;
import android.support.v17.leanback.widget.ObjectAdapter;

/**
 * Activity showing a menu of settings.
 */
public abstract class MenuActivity extends Activity {

    private static final String TAG_BROWSE_FRAGMENT = "browse_fragment";

    private BrowseFragment mBrowseFragment;

    protected abstract String getBrowseTitle();

    protected abstract Drawable getBadgeImage();

    protected abstract BrowseInfoFactory getBrowseInfoFactory();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mBrowseFragment = (BrowseFragment) getFragmentManager().findFragmentByTag(
                TAG_BROWSE_FRAGMENT);
        if (mBrowseFragment == null) {
            mBrowseFragment = new BrowseFragment();
            getFragmentManager().beginTransaction()
                    .replace(android.R.id.content, mBrowseFragment, TAG_BROWSE_FRAGMENT)
                    .commit();
        }

        ClassPresenterSelector rowPresenterSelector = new ClassPresenterSelector();
        rowPresenterSelector.addClassPresenter(ListRow.class, new ListRowPresenter());

        ObjectAdapter adapter = getBrowseInfoFactory().getRows();
        adapter.setPresenterSelector(rowPresenterSelector);

        mBrowseFragment.setAdapter(adapter);
        updateBrowseParams();
    }

    protected void updateBrowseParams() {
        mBrowseFragment.setTitle(getBrowseTitle());
        mBrowseFragment.setBadgeDrawable(getBadgeImage());
        mBrowseFragment.setHeadersState(BrowseFragment.HEADERS_DISABLED);
    }
}
