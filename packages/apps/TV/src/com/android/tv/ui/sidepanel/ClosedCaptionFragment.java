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

package com.android.tv.ui.sidepanel;

import android.media.tv.TvTrackInfo;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import com.android.tv.R;
import com.android.tv.util.CaptionSettings;
import com.android.tv.droidlogic.QuickKeyInfo;
import com.android.tv.data.api.Channel;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class ClosedCaptionFragment extends SideFragment {
    private static final String TRACKER_LABEL = "closed caption";
    private static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";
    private boolean mResetClosedCaption;
    private int mClosedCaptionOption;
    private String mClosedCaptionLanguage;
    private String mClosedCaptionTrackId;
    private ClosedCaptionOptionItem mSelectedItem;
    private Item mCaptionsSetting = null;

    private static final String CC_OPTION = "CC_OPTION";
    private static final String CC_TRACKID = "CC_TRACKID";
    private static final String CC_LANGUAGE = "CC_LANGUAGE";

    public ClosedCaptionFragment() {
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
        CaptionSettings captionSettings = getMainActivity().getCaptionSettings();
        mResetClosedCaption = true;
        mClosedCaptionOption = captionSettings.getEnableOption();
        mClosedCaptionLanguage = captionSettings.getLanguage();
        mClosedCaptionTrackId = captionSettings.getTrackId();

        List<Item> items1 = new ArrayList<>();
        mSelectedItem = null;

        List<TvTrackInfo> tracks = getMainActivity().getTracks(TvTrackInfo.TYPE_SUBTITLE);
        if (tracks != null && !tracks.isEmpty()) {
            String selectedTrackId =
                    captionSettings.isEnabled()
                            ? getMainActivity().getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE)
                            : null;
            ClosedCaptionOptionItem item1 = new ClosedCaptionOptionItem(null, null);
            if (selectedTrackId == null) {
                mSelectedItem = item1;
                item1.setChecked(true);
                setSelectedPosition(0);
            }
            int position = 1;
            List<Item> items2 = new ArrayList<>();
            ClosedCaptionOptionItem item2 = null;
            boolean hassubtitle = false;
            for (int i = 0; i < tracks.size(); i++) {
                if (QuickKeyInfo.isTeletextSubtitleTrack(tracks.get(i).getId())) {
                    continue;
                }
                item2 = new ClosedCaptionOptionItem(tracks.get(i), position);
                if (TextUtils.equals(selectedTrackId, tracks.get(i).getId())) {
                    mSelectedItem = item2;
                    item2.setChecked(true);
                    setSelectedPosition(position);
                }
                if (!hassubtitle) {
                    hassubtitle = true;
                }
                position++;
                items2.add(item2);
            }
            if (hassubtitle) {
                Channel current = getMainActivity().getCurrentChannel();
                //use captionmanager to control subtitle and hide it for dtvkit
                if (current != null && !DTVKIT_PACKAGE.equals(current.getPackageName())) {
                    items1.add(item1);
                }
                items1.addAll(items2);
            }
        }
        items1.add(new SwitchItem(getString(R.string.cc_enable_cc_style),
                getString(R.string.cc_disable_cc_style)) {
            @Override
            protected void onUpdate() {
                super.onUpdate();
                setChecked(getMainActivity().getCaptionSettings().isCaptionsStyleEnabled());
            }

            @Override
            protected void onSelected() {
                super.onSelected();
                boolean checked = isChecked();
                getMainActivity().getCaptionSettings().setCaptionsStyleEnabled(checked);
                if (mCaptionsSetting != null) {
                    mCaptionsSetting.setEnabled(checked);
                }
            }
        });
        if (getMainActivity().hasCaptioningSettingsActivity()) {
            items1.add(
                    mCaptionsSetting = new ActionItem(
                            getString(R.string.closed_caption_system_settings),
                            getString(R.string.closed_caption_system_settings_description)) {
                        @Override
                        protected void onSelected() {
                            getMainActivity().startSystemCaptioningSettingsActivity();
                        }

                        @Override
                        protected void onFocused() {
                            super.onFocused();
                            if (mSelectedItem != null) {
                                getMainActivity()
                                        .selectSubtitleTrack(
                                                mSelectedItem.mOption, mSelectedItem.mTrackId);
                            }
                        }
                    });
            mCaptionsSetting.setEnabled(getMainActivity().getCaptionSettings().isCaptionsStyleEnabled());
        }
        return items1;
    }

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onDestroyView() {
        if (mResetClosedCaption) {
            getMainActivity()
                    .selectSubtitleLanguage(
                            mClosedCaptionOption, mClosedCaptionLanguage, mClosedCaptionTrackId);
        }
        super.onDestroyView();
    }

    private String getLabel(TvTrackInfo track, Integer trackIndex) {
        if (track == null) {
            return getString(R.string.closed_caption_option_item_off);
        } else if (track.getLanguage() != null) {
            String flag = QuickKeyInfo.getStringFromTeletextSubtitleTrack("flag", track.getId());
            if (TextUtils.isEmpty(flag)) {
                flag = "";
            } else if ("none".equals(flag)) {
                flag = "";
            } else {
                flag = " [" + flag + "]";
            }
            return new Locale(track.getLanguage()).getDisplayName() + flag;
        }
        return getString(R.string.closed_caption_unknown_language, trackIndex);
    }

    private class ClosedCaptionOptionItem extends RadioButtonItem {
        private final int mOption;
        private final String mTrackId;

        private ClosedCaptionOptionItem(TvTrackInfo track, Integer trackIndex) {
            super(getLabel(track, trackIndex));
            if (track == null) {
                mOption = CaptionSettings.OPTION_OFF;
                mTrackId = null;
            } else {
                mOption = CaptionSettings.OPTION_ON;
                mTrackId = track.getId();
            }
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            mSelectedItem = this;
            if (getMainActivity().getCaptionSettings().isSystemSettingEnabled()) {
                getMainActivity().selectSubtitleTrack(mOption, mTrackId);
            } else {
                String captionOff = getMainActivity().getString(R.string.options_item_closed_caption)
                        + " " + getMainActivity().getString(R.string.closed_caption_option_item_off);
                Toast.makeText(
                                getMainActivity(),
                                captionOff,
                                Toast.LENGTH_SHORT)
                        .show();
            }
            //sub index is saved in db of each channel
            /*Settings.System.putInt(getActivity().getContentResolver(), CC_OPTION, mOption);
            Settings.System.putString(getActivity().getContentResolver(), CC_TRACKID, mTrackId);*/
            mResetClosedCaption = false;
            closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            //getMainActivity().selectSubtitleTrack(mOption, mTrackId);
        }
    }
}
