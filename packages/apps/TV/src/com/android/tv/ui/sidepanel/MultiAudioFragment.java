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
import android.text.TextUtils;
import android.view.KeyEvent;
import com.android.tv.R;
import com.android.tv.util.Utils;
import java.util.ArrayList;
import java.util.List;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.ChannelInfo;

import android.util.Log;

public class MultiAudioFragment extends SideFragment {
    private static final String TAG = "MultiAudioFragment";
    private int mInitialSelectedPosition = INVALID_POSITION;
    private String mSelectedTrackId;
    private String mFocusedTrackId;

    private static final int[] STEREOSTATUS_BTSC = {
        R.string.channel_audio_outmode_mono,
        R.string.channel_audio_outmode_stereo,
        R.string.channel_audio_outmode_sap
    };

    private static final int[] STEREOSTATUS_NICAM = {
        R.string.channel_audio_outmode_mono,
        R.string.channel_audio_outmode_nicam_mono,
        R.string.channel_audio_outmode_stereo,
        R.string.channel_audio_outmode_dualI,
        R.string.channel_audio_outmode_dualII,
        R.string.channel_audio_outmode_dualI_II
    };

    private static final int[] STEREOSTATUS_A2 = {
        R.string.channel_audio_outmode_mono,
        R.string.channel_audio_outmode_stereo,
        R.string.channel_audio_outmode_dualI,
        R.string.channel_audio_outmode_dualII
    };

    private static final int[] MONO_ONLY = {
        R.string.channel_audio_outmode_mono,
    };

    public MultiAudioFragment() {
        super(KeyEvent.KEYCODE_MEDIA_AUDIO_TRACK, KeyEvent.KEYCODE_A);
    }

    @Override
    protected String getTitle() {
        return getString(R.string.side_panel_title_multi_audio);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        if (getMainActivity().mQuickKeyInfo.isAtvSource()) {
            return getAtvTrackList();
        } else {
            return getDefaultTrackList();
        }
    }

    private final int BASC_MONO   = 0;
    private final int BASC_STEREO = 1;
    private final int BASC_SAP    = 2;

    private final int A2_MONO     = 0;
    private final int A2_STEREO   = 1;
    private final int A2_DualI    = 2;
    private final int A2_DualII   = 3;
    private final int A2_DualI_II = 4;

    private final int NICAM_MONO     = 0;
    private final int NICAM_MONO1    = 1;
    private final int NICAM_STEREO   = 2;
    private final int NICAM_DualI    = 3;
    private final int NICAM_DualII   = 4;
    private final int NICAM_DualI_II = 5;

    private ChannelInfo mAtvChannelInfo;
    private int audio_std = 0;

    private List<Item> getAtvTrackList() {
        List<Item> items = new ArrayList<>();
        mAtvChannelInfo = getMainActivity().mQuickKeyInfo.getCurrentChannelInfo();
        int value = getMainActivity().mQuickKeyInfo.getAtvAudioPriorityMode();
        int input = getMainActivity().mQuickKeyInfo.getAtvAudioStreamOutmode();
        int readmode = getMainActivity().mQuickKeyInfo.getAtvAudioOutMode();

        Log.d(TAG, "value 0x" + Integer.toHexString(value) + ", input 0x" + Integer.toHexString(input) + ", readmode " + readmode);

        audio_std = (input >> 8) & 0xFF;

        switch (audio_std) {
            case TvControlManager.AUDIO_STANDARD_BTSC: {
                boolean[] status = {false, false, false};// mono stereo sap
                switch (input & 0xFF) {
                    case TvControlManager.AUDIO_INMODE_MONO:
                        status[BASC_MONO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_STEREO:
                        status[BASC_MONO] = true;
                        status[BASC_STEREO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_MONO_SAP:
                        status[BASC_MONO] = true;
                        status[BASC_SAP] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_STEREO_SAP:
                        status[BASC_MONO] = true;
                        status[BASC_STEREO] = true;
                        status[BASC_SAP] = true;
                        break;
                }

                int pos = 0;
                for (int i = 0; i < STEREOSTATUS_BTSC.length; i++) {
                    String info = getString(STEREOSTATUS_BTSC[i]);
                    RadioButtonItem item = new MultiAudioOptionItem(info, String.valueOf(i));
                    if (readmode == i) {
                        if (readmode != (value & 0xFF)) {
                            setAudioMode(String.valueOf(readmode));
                        }
                        item.setChecked(true);
                        items.add(item);
                        mInitialSelectedPosition = pos;
                        mSelectedTrackId = mFocusedTrackId = String.valueOf(i);
                    } else {
                        if (!status[i]) {
                            item.setEnabled(false);
                        } else {
                            items.add(item);
                        }
                    }

                    ++pos;
                }
            }
                break;
            case TvControlManager.AUDIO_STANDARD_A2_K:
            case TvControlManager.AUDIO_STANDARD_A2_BG:
            case TvControlManager.AUDIO_STANDARD_A2_DK1:
            case TvControlManager.AUDIO_STANDARD_A2_DK2:
            case TvControlManager.AUDIO_STANDARD_A2_DK3: {
                boolean[] status = {false, false, false, false};// mono stereo dual
                switch (input & 0xFF) {
                    case TvControlManager.AUDIO_INMODE_MONO:
                        status[A2_MONO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_STEREO:
                        status[A2_MONO] = true;
                        status[A2_STEREO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_DUAL:
                        //status[A2_MONO] = true;
                        status[A2_DualI] = true;
                        status[A2_DualII] = true;
                        break;
                }

                int pos = 0;
                for (int i = 0; i < STEREOSTATUS_A2.length; i++) {
                    String info = getString(STEREOSTATUS_A2[i]);
                    RadioButtonItem item = new MultiAudioOptionItem(info, String.valueOf(i));
                    if (readmode == i) {
                        if (readmode != (value & 0xFF)) {
                            setAudioMode(String.valueOf(readmode));
                        }
                        item.setChecked(true);
                        items.add(item);
                        mInitialSelectedPosition = pos;
                        mSelectedTrackId = mFocusedTrackId = String.valueOf(i);
                    } else {
                        if (!status[i]) {
                            item.setEnabled(false);
                        } else {
                            items.add(item);
                        }
                    }

                    ++pos;
                }
            }
                break;
            case TvControlManager.AUDIO_STANDARD_NICAM_I:
            case TvControlManager.AUDIO_STANDARD_NICAM_BG:
            case TvControlManager.AUDIO_STANDARD_NICAM_L:
            case TvControlManager.AUDIO_STANDARD_NICAM_DK: {
                boolean[] status = {false, false, false, false, false, false};// mono stereo dual
                switch (input & 0xFF) {
                    case TvControlManager.AUDIO_INMODE_MONO:
                        status[NICAM_MONO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_STEREO:
                        status[NICAM_MONO] = true;
                        status[NICAM_STEREO] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_DUAL:
                        status[NICAM_MONO] = true;
                        status[NICAM_DualI] = true;
                        status[NICAM_DualII] = true;
                        status[NICAM_DualI_II] = true;
                        break;
                    case TvControlManager.AUDIO_INMODE_NICAM_MONO:
                        status[NICAM_MONO] = true;
                        status[NICAM_MONO1] = true;
                        break;
                }

                int pos = 0;
                for (int i = 0; i < STEREOSTATUS_NICAM.length; i++) {
                    String info = getString(STEREOSTATUS_NICAM[i]);
                    RadioButtonItem item = new MultiAudioOptionItem(info, String.valueOf(i));
                    if (readmode == i) {
                        if (readmode != (value & 0xFF)) {
                            setAudioMode(String.valueOf(readmode));
                        }
                        item.setChecked(true);
                        items.add(item);
                        mInitialSelectedPosition = pos;
                        mSelectedTrackId = mFocusedTrackId = String.valueOf(i);
                    } else {
                        if (!status[i]) {
                            item.setEnabled(false);
                        } else {
                            items.add(item);
                        }
                    }

                    ++pos;
                }
            }
                break;
            case TvControlManager.AUDIO_STANDARD_MONO_BG:
            case TvControlManager.AUDIO_STANDARD_MONO_DK:
            case TvControlManager.AUDIO_STANDARD_MONO_I:
            case TvControlManager.AUDIO_STANDARD_MONO_M:
            case TvControlManager.AUDIO_STANDARD_MONO_L:
            {
                int pos = 0;
                RadioButtonItem item = new MultiAudioOptionItem(getString(MONO_ONLY[0]), String.valueOf(pos));
                item.setChecked(true);
                items.add(item);
                if (readmode != (value & 0xFF)) {
                    setAudioMode(String.valueOf(readmode));
                }
                mInitialSelectedPosition = pos;
                mSelectedTrackId = mFocusedTrackId = String.valueOf(pos);
            }
                break;
            default:
                Log.d(TAG, "Unsupport audio std: 0x" + Integer.toHexString(readmode));
                break;
        }

        return items;
    }

    protected List<Item> getDefaultTrackList() {
        List<TvTrackInfo> tracks = getMainActivity().getTracks(TvTrackInfo.TYPE_AUDIO);
        mSelectedTrackId = getMainActivity().getSelectedTrack(TvTrackInfo.TYPE_AUDIO);
        final String AD_AUDIO_DISCRIPTION = "ad";
        final String AD_AUDIO_SUF = " ad";

        List<Item> items = new ArrayList<>();
        if (tracks != null) {
            boolean needToShowSampleRate = Utils.needToShowSampleRate(getActivity(), tracks);
            int pos = 0;
            for (final TvTrackInfo track : tracks) {
                CharSequence des = track.getDescription();
                String descrip = (des != null ? des.toString() : null);
                boolean isAdAudio = AD_AUDIO_DISCRIPTION.equalsIgnoreCase(descrip);
                RadioButtonItem item = new MultiAudioOptionItem(
                    Utils.getMultiAudioWithFormat(getMainActivity(), track) + (isAdAudio ? AD_AUDIO_SUF : ""), track.getId());
                if (track.getId().equals(mSelectedTrackId)) {
                    item.setChecked(true);
                    mInitialSelectedPosition = pos;
                    mSelectedTrackId = mFocusedTrackId = track.getId();
                }
                items.add(item);
                ++pos;
            }
        }
        return items;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mInitialSelectedPosition != INVALID_POSITION) {
            setSelectedPosition(mInitialSelectedPosition);
        }
    }

    private void setAudioMode(String trackid) {
        if (getMainActivity().mQuickKeyInfo.isAtvSource()) {
            int mode = Integer.parseInt(trackid);
            getMainActivity().mQuickKeyInfo.setAtvAudioOutmode((audio_std << 8) | (mode & 0xFF));
            if (mAtvChannelInfo == null) {
                mAtvChannelInfo = getMainActivity().mQuickKeyInfo.getCurrentChannelInfo();
            }
            mAtvChannelInfo.setAudioOutPutMode((audio_std << 8) | (mode & 0xFF));
            getMainActivity().mQuickKeyInfo.updateChnnelInfoToDb(mAtvChannelInfo);
        } else {
            getMainActivity().selectAudioTrack(String.valueOf(trackid));
        }
    }

    private class MultiAudioOptionItem extends RadioButtonItem {
        private final String mTrackId;

        private MultiAudioOptionItem(String title, String trackId) {
            super(title);
            mTrackId = trackId;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            mSelectedTrackId = mFocusedTrackId = mTrackId;

            setAudioMode(mTrackId);

            closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            mFocusedTrackId = mTrackId;
            //only deal when Selected
            //getMainActivity().selectAudioTrack(mTrackId);
        }
    }

    @Override
    public void onDetach() {
        if (!TextUtils.equals(mSelectedTrackId, mFocusedTrackId)) {
            //only deal when Selected
            //getMainActivity().selectAudioTrack(mSelectedTrackId);
        }
        super.onDetach();
    }
}
