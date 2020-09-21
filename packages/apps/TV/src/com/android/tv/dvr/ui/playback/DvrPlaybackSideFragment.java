/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.dvr.ui.playback;

import android.media.tv.TvTrackInfo;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.app.GuidedStepFragment;
import android.support.v17.leanback.widget.GuidedAction;
import android.text.TextUtils;
import android.transition.Transition;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.util.Log;

import com.android.tv.R;
import com.android.tv.util.TvSettings;
import com.android.tv.util.Utils;
import com.android.tv.dvr.ui.playback.DvrPlaybackControlHelper.TeletextControlCallback;

import com.android.tv.droidlogic.QuickKeyInfo;

import java.util.List;
import java.util.Locale;
import java.util.ArrayList;

/** Fragment for DVR playback closed-caption/multi-audio settings. */
public class DvrPlaybackSideFragment extends GuidedStepFragment {
    public static final String TAG = "DvrPlaybackSideFragment";
    /** The tag for passing track infos to side fragments. */
    public static final String TRACK_INFOS = "dvr_key_track_infos";
    /** The tag for passing selected track's ID to side fragments. */
    public static final String SELECTED_TRACK_ID = "dvr_key_selected_track_id";

    private static final int ACTION_ID_NO_SUBTITLE = -1;
    private static final int CHECK_SET_ID = 1;
    private static final int ACTION_ID_PARENT_SUBTITLE = 0;
    private static final int ACTION_ID_PARENT_TELETEXT = 1;
    private static final int ACTION_ID_PARENT_AUDIO = 2;
    private static final int ACTION_ID_CHILD_SUBTITLE = 3;
    private static final int ACTION_ID_CHILD_TELETEXT = 4;
    private static final int ACTION_ID_PARENT_SET_TELETEXT = -2;

    private List<TvTrackInfo> mTrackInfos;
    private String mSelectedTrackId;
    private TvTrackInfo mSelectedTrack;
    private int mTrackType;
    private DvrPlaybackOverlayFragment mOverlayFragment;
    private TeletextControlCallback mTeletextControlCallback;
    private boolean mSubAction = false;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        mTrackInfos = getArguments().getParcelableArrayList(TRACK_INFOS);
        mTrackType = mTrackInfos.get(0).getType();
        mSelectedTrackId = getArguments().getString(SELECTED_TRACK_ID);
        mOverlayFragment =
                ((DvrPlaybackOverlayFragment)
                        getFragmentManager().findFragmentById(R.id.dvr_playback_controls_fragment));
        super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateBackgroundView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View backgroundView = super.onCreateBackgroundView(inflater, container, savedInstanceState);
        backgroundView.setBackgroundColor(
                getResources().getColor(R.color.lb_playback_controls_background_light));
        return backgroundView;
    }

    @Override
    public void onCreateActions(@NonNull List<GuidedAction> actions, Bundle savedInstanceState) {
        List<GuidedAction> subtitleActions = new ArrayList<GuidedAction>();
        List<GuidedAction> teletextActions = new ArrayList<GuidedAction>();
        List<GuidedAction> teletextSetActions = new ArrayList<GuidedAction>();
        List<GuidedAction> audioActions = new ArrayList<GuidedAction>();
        boolean isSubTitle = (mTrackType == TvTrackInfo.TYPE_SUBTITLE);
        boolean[] subtitleStatus = checkSubtitleTeletextStatus();
        if (isSubTitle) {
            if (subtitleStatus[0]) {
                subtitleActions.add(
                    new GuidedAction.Builder(getActivity())
                            .id(ACTION_ID_NO_SUBTITLE)
                            .title(getString(R.string.closed_caption_option_item_off))
                            .checkSetId(CHECK_SET_ID)
                            .checked(mSelectedTrackId == null)
                            .build());
            }
            if (subtitleStatus[1]) {
                teletextActions.add(
                    new GuidedAction.Builder(getActivity())
                            .id(ACTION_ID_NO_SUBTITLE)
                            .title(getString(R.string.closed_caption_option_item_off))
                            .checkSetId(CHECK_SET_ID)
                            .checked(mSelectedTrackId == null)
                            .build());
                teletextSetActions.add(
                    new GuidedAction.Builder(getActivity())
                            .id(ACTION_ID_PARENT_SET_TELETEXT)
                            .title(getString(R.string.subtitle_teletext_settings))
                            .checkSetId(GuidedAction.NO_CHECK_SET)
                            .build());
            }
        }
        for (int i = 0; i < mTrackInfos.size(); i++) {
            TvTrackInfo info = mTrackInfos.get(i);
            boolean isTeletext = QuickKeyInfo.isTeletextSubtitleTrack(info.getId());
            boolean checked = TextUtils.equals(info.getId(), mSelectedTrackId);
            GuidedAction action =
                    new GuidedAction.Builder(getActivity())
                            .id(i)
                            .title(getTrackLabel(info, i))
                            .checkSetId(CHECK_SET_ID)
                            .checked(checked)
                            .build();
            if (isSubTitle) {
                if (isTeletext) {
                    teletextActions.add(action);
                } else {
                    subtitleActions.add(action);
                }
            } else {
                audioActions.add(action);
            }
            if (checked) {
                mSelectedTrack = info;
            }
        }
        if (isSubTitle) {
            mSubAction = false;
            Log.d(TAG, "onCreateActions sub = " + subtitleActions.size() + ", tele = " + teletextActions.size());
            if (teletextActions.size() > 1 && subtitleActions.size() > 1) {//both
                mSubAction = true;
                actions.add(new GuidedAction.Builder()
                       .id(ACTION_ID_PARENT_SUBTITLE)
                       .title(getString(R.string.side_panel_subtitle))
                       .subActions(subtitleActions)
                       .build());
                teletextActions.addAll(teletextSetActions);
                actions.add(new GuidedAction.Builder()
                       .id(ACTION_ID_PARENT_TELETEXT)
                       .title(getString(R.string.side_panel_title_teletext))
                       .subActions(teletextActions)
                       .build());
            } else if (teletextActions.size() == 0 && subtitleActions.size() > 1) {//subtitle
                actions.addAll(subtitleActions);
            } else if (teletextActions.size() > 1 && subtitleActions.size() == 0) {//teletext
                teletextActions.addAll(teletextSetActions);
                actions.addAll(teletextActions);
            } else {
                Log.d(TAG, "onCreateActions can't find subtitle");
            }
        } else {
            actions.addAll(audioActions);
        }
    }

    @Override
    public void onGuidedActionFocused(GuidedAction action) {
        int actionId = (int) action.getId();
        Log.d(TAG, "onGuidedActionFocused actionId= " + actionId);
        //mOverlayFragment.selectTrack(mTrackType, actionId < 0 ? null : mTrackInfos.get(actionId));
    }

    @Override
    public void onGuidedActionClicked(GuidedAction action) {
        int actionId = (int) action.getId();
        if (!mSubAction) {
            if (actionId == ACTION_ID_PARENT_SET_TELETEXT) {
                if (mTeletextControlCallback != null) {
                    mTeletextControlCallback.onTeletextControlCallback(DvrPlaybackControlHelper.STATUS_SET_TELETEXT);
                } else {
                    Log.d(TAG, "onSubGuidedActionClicked mTeletextControlCallback null");
                }
            } else {
                mSelectedTrack = actionId < 0 ? null : mTrackInfos.get(actionId);
                mOverlayFragment.selectTrack(mTrackType, mSelectedTrack);
                TvSettings.setDvrPlaybackTrackSettings(getContext(), mTrackType, mSelectedTrack);
            }
            getFragmentManager().popBackStack();
        }
        Log.d(TAG, "onGuidedActionClicked actionId= " + actionId + ", mSubAction = " + mSubAction + ", mSelectedTrack = " + mSelectedTrack);
    }

    @Override
    public boolean onSubGuidedActionClicked(GuidedAction action) {
        // Check for which action was clicked, and handle as needed
        int actionId = (int) action.getId();
        if (mSubAction) {
            if (actionId == ACTION_ID_PARENT_SET_TELETEXT) {
                if (mTeletextControlCallback != null) {
                    mTeletextControlCallback.onTeletextControlCallback(DvrPlaybackControlHelper.STATUS_SET_TELETEXT_IN_SUB);
                } else {
                    Log.d(TAG, "onSubGuidedActionClicked mTeletextControlCallback null");
                }
            } else {
                mSelectedTrack = actionId < 0 ? null : mTrackInfos.get(actionId);
                mOverlayFragment.selectTrack(mTrackType, mSelectedTrack);
                TvSettings.setDvrPlaybackTrackSettings(getContext(), mTrackType, mSelectedTrack);
            }
            getFragmentManager().popBackStack();
        }
        // Return true to collapse the subactions drop-down list, or
        // false to keep the drop-down list expanded.
        Log.d(TAG, "onSubGuidedActionClicked actionId= " + actionId + ", mSubAction = " + mSubAction + ", mSelectedTrack = " + mSelectedTrack);
        return true;
    }

    @Override
    public void onStart() {
        super.onStart();
        // Workaround: when overlay fragment is faded out, any focus will lost due to overlay
        // fragment's implementation. So we disable overlay fragment's fading here to prevent
        // losing focus while users are interacting with the side fragment.
        mOverlayFragment.setFadingEnabled(false);
    }

    @Override
    public void onStop() {
        super.onStop();
        // We disable fading of overlay fragment to prevent side fragment from losing focus,
        // therefore we should resume it here.
        mOverlayFragment.setFadingEnabled(true);
        //mOverlayFragment.selectTrack(mTrackType, mSelectedTrack);
    }

    private String getTrackLabel(TvTrackInfo track, int trackIndex) {
        if (track.getLanguage() != null) {
            if (track.getType() == TvTrackInfo.TYPE_SUBTITLE) {
                String flag = QuickKeyInfo.getStringFromTeletextSubtitleTrack("flag", track.getId());
                if (TextUtils.isEmpty(flag)) {
                    flag = "";
                } else if ("none".equals(flag)) {
                    flag = "";
                } else {
                    flag = " [" + flag + "]";
                }
                return new Locale(track.getLanguage()).getDisplayName() + flag;
            } else if (track.getType() == TvTrackInfo.TYPE_AUDIO) {
                final String AD_AUDIO_DISCRIPTION = "ad";
                final String AD_AUDIO_SUF = " ad";
                CharSequence des = track.getDescription();
                String descrip = (des != null ? des.toString() : null);
                boolean isAdAudio = AD_AUDIO_DISCRIPTION.equalsIgnoreCase(descrip);
                return Utils.getTrackInfoFormat(getActivity(), track) + (isAdAudio ? AD_AUDIO_SUF : "");
            }
        }
        return track.getType() == TvTrackInfo.TYPE_SUBTITLE
                ? getString(R.string.closed_caption_unknown_language, trackIndex + 1)
                : getString(R.string.multi_audio_unknown_language);
    }

    @Override
    protected void onProvideFragmentTransitions() {
        super.onProvideFragmentTransitions();
        // Excludes the background scrim from transition to prevent the blinking caused by
        // hiding the overlay fragment and sliding in the side fragment at the same time.
        Transition t = getEnterTransition();
        if (t != null) {
            t.excludeTarget(R.id.guidedstep_background, true);
        }
    }

    public void setTeletextControlCallback(TeletextControlCallback callback) {
        mTeletextControlCallback = callback;
    }

    private boolean[] checkSubtitleTeletextStatus() {
        boolean[] result = {false, false};
        boolean isSubTitle = (mTrackType == TvTrackInfo.TYPE_SUBTITLE);
        if (isSubTitle) {
            for (int i = 0; i < mTrackInfos.size(); i++) {
                Log.d(TAG, "checkSubtitleTeletextStatus i = " + i + "->" + mTrackInfos.get(i).getId());
                boolean isTeletext = QuickKeyInfo.isTeletextSubtitleTrack(mTrackInfos.get(i).getId());
                if (isTeletext) {
                    result[1] = true;
                } else {
                    result[0] = true;
                }
            }
        }
        return result;
    }
}
