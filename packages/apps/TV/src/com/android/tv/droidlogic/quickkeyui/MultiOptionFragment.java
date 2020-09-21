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

package com.android.tv.droidlogic.quickkeyui;

import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import android.view.KeyEvent;
import android.os.Bundle;
import com.android.tv.R;

import com.droidlogic.app.DroidLogicKeyEvent;

import java.util.ArrayList;
import java.util.List;

public class MultiOptionFragment extends SideFragment {
    private static final String TAG = "MultiOptionFragment";
    private int mKeyValue;
    private List<ActionItem> mActionItems;

    public final static int LIST_CHANNEL = 0;
    public final static int LIST_FAV = 1;
    public final static int TYPE_VIDEO = 0;
    public final static int TYPE_RADIO = 1;

    private final static int[] FUNCTION_LIST = {R.string.favourite_list, R.string.channel_list};
    private final static int[] TYPE_LIST = {R.string.tv, R.string.radio};

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    public MultiOptionFragment(int keyvalue) {
        super(keyvalue, KeyEvent.KEYCODE_UNKNOWN);
        this.mKeyValue = keyvalue;
    }

    @Override
    protected String getTitle() {
        return getTitle(mKeyValue);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    private String getTitle(int value) {
        String title = null;
        switch (value) {
            case DroidLogicKeyEvent.KEYCODE_FAV: {
                title = getString(FUNCTION_LIST[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_LIST: {
                title = getString(FUNCTION_LIST[1]);
                break;
            }
        }
        return title;
    }

    private int getListValue(int value) {
        int result = -1;
        switch (value) {
            case DroidLogicKeyEvent.KEYCODE_FAV: {
                result = LIST_FAV;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_LIST: {
                result = LIST_CHANNEL;
                break;
            }
        }
        return result;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mActionItems = new ArrayList<>();
        for (int i = 0; i < TYPE_LIST.length; i++) {
            SubMenuItem submenu = new SubMenuItem(getString(TYPE_LIST[i]),
                    null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ListFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            };
            Bundle args = new Bundle();
            args.putInt("list", getListValue(mKeyValue));
            args.putInt("type", i);
            submenu.setparameters(args);
            mActionItems.add(submenu);
        }

        items.addAll(mActionItems);
        return items;
    }
}

