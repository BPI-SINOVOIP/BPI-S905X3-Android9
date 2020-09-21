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

package com.android.tv.droidlogic.subtitleui;

import android.media.tv.TvTrackInfo;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.content.Context;
import android.provider.Settings;

import com.android.tv.R;
import com.android.tv.util.CaptionSettings;
import com.android.tv.util.Utils;
import com.android.tv.ui.sidepanel.ClosedCaptionFragment;
import com.android.tv.ui.sidepanel.SwitchItem;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.droidlogic.channelui.EditTextItem;
import com.android.tv.util.CaptionSettings;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.DividerItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.droidlogic.QuickKeyInfo;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class SubtitleFragment extends SideFragment {
    private static final String TRACKER_LABEL ="subtitle" ;
    private List<Item> mItems;

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    public SubtitleFragment() {
        super(KeyEvent.KEYCODE_CAPTIONS, KeyEvent.KEYCODE_S);
    }

    @Override
    protected String getTitle() {
        return getString(R.string.side_panel_subtitle);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    @Override
    protected List<Item> getItemList() {
        final boolean subswitch = getCaptionsEnabled(getMainActivity());

        mItems = new ArrayList<>();
        /*
        mItems.add(new SwitchItem(getString(R.string.subtitle_teletext_switch_on),
                getString(R.string.subtitle_teletext_switch_off)) {
            @Override
            protected void onUpdate() {
                super.onUpdate();
                setChecked(subswitch);
            }

            @Override
            protected void onSelected() {
                super.onSelected();
                boolean checked = isChecked();
                setCaptionsEnabled(getMainActivity(), checked);
            }
        });
        */

        mItems.add(new SubMenuItem(getString(R.string.side_panel_title_closed_caption),
                null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new ClosedCaptionFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });

        if (getMainActivity().mQuickKeyInfo.isDtmbModeCountry()) {
            return mItems;
        }
        mItems.add(new SubMenuItem(getString(R.string.side_panel_title_teletext),
                null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new TeleTextFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });

        return mItems;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
    }

    @Override
    public void onPause() {
        super.onPause();

    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    public static boolean isTeleTextEnabled(Context context) {
        return Settings.Secure.getInt(context.getContentResolver(), "accessibility_captioning_teletext_enabled", 0) != 0;
    }

    public static void setTeleTextEnabled(Context context, boolean enabled) {
        Settings.Secure.putInt(context.getContentResolver(), "accessibility_captioning_teletext_enabled", enabled ? 1 : 0);
    }

    public static boolean getCaptionsEnabled(Context context) {
        return Settings.Secure.getInt(context.getContentResolver(),
            "accessibility_captioning_enabled"/*Settings.Secure.ACCESSIBILITY_CAPTIONING_ENABLED*/, 0) != 0;
    }

    public static void setCaptionsEnabled(Context context, boolean enabled) {
        Settings.Secure.putInt(context.getContentResolver(),
                "accessibility_captioning_enabled"/*Settings.Secure.ACCESSIBILITY_CAPTIONING_ENABLED*/, enabled ? 1 : 0);
        //can set by write data base
    }
}