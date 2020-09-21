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

import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.media.tv.TvTrackInfo;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.tv.ui.sidepanel.SwitchItem;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.DividerItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.util.CaptionSettings;

import java.util.List;
import java.util.ArrayList;
import java.util.Locale;

import com.android.tv.R;

public class TeleTextFragment extends SideFragment {
    private static final String TRACKER_LABEL ="teletext" ;
    private List<Item> mItems;
    private boolean mPaused;

    public TeleTextFragment() {
        super(KeyEvent.KEYCODE_CAPTIONS, KeyEvent.KEYCODE_S);
    }

    @Override
    protected String getTitle() {
        return getString(R.string.side_panel_title_teletext);
    }

    @Override
    public String getTrackerLabel() {
        return TRACKER_LABEL;
    }

    private String getLabel(TvTrackInfo track, Integer trackIndex) {
        if (track == null) {
            return getString(R.string.closed_caption_option_item_off);
        } else if (track.getLanguage() != null) {
            return new Locale(track.getLanguage()).getDisplayName();
        }
        return getString(R.string.closed_caption_unknown_language, trackIndex + 1);
    }

    @Override
    protected List<Item> getItemList() {
        CaptionSettings captionSettings = getMainActivity().getCaptionSettings();
        mItems = new ArrayList<>();

        List<TvTrackInfo> tracks = getMainActivity().getTracks(TvTrackInfo.TYPE_SUBTITLE);
        if (tracks != null && !tracks.isEmpty()) {
            String trackId = captionSettings.isEnabled()
                            ? getMainActivity().getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE)
                            : null;
            boolean isEnabled = trackId != null;

            TeleTextOptionItem item1 = new TeleTextOptionItem(null, null);
            if (trackId == null) {
                item1.setChecked(true);
                setSelectedPosition(0);
            }
            int position = 1;
            List<Item> items2 = new ArrayList<>();
            TeleTextOptionItem item2 = null;
            boolean hasteletext = false;

            for (int i = 0; i < tracks.size(); i++) {
                if (!getMainActivity().mQuickKeyInfo.isTeletextSubtitleTrack(tracks.get(i).getId())) {
                    continue;
                }
                item2 = new TeleTextOptionItem(tracks.get(i), position);
                if (isEnabled && tracks.get(i).getId().equals(trackId)) {
                    item2.setChecked(true);
                    setSelectedPosition(position);
                }
                if (!hasteletext) {
                    hasteletext = true;
                }
                position++;
                items2.add(item2);
            }
            if (hasteletext) {
                mItems.add(item1);
                mItems.addAll(items2);
            }
        }

        //mItems.add(new DividerItem(getString(R.string.subtitle_teletext_settings)));
        mItems.add(new SubMenuItem(getString(R.string.subtitle_teletext_open),
                null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new TeleTextSettingFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });

        return mItems;
    }

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return super.onCreateView(inflater, container, savedInstanceState);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mPaused) {
            resetItemList(getItemList());
        }
        mPaused = false;
    }

    @Override
    public void onPause() {
        super.onPause();
        mPaused = true;
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
    }

    private class TeleTextOptionItem extends RadioButtonItem {
        private final int mOption;
        private final String mTrackId;

        private TeleTextOptionItem(TvTrackInfo track, Integer trackIndex) {
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
            getMainActivity().selectSubtitleTrack(mOption, mTrackId);
            //closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
        }
    }
}