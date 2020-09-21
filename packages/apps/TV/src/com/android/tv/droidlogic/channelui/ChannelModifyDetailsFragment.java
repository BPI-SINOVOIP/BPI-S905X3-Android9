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

import android.content.Intent;
import android.media.tv.TvContract.Channels;
import android.provider.Settings;
import android.util.Log;
import android.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View.OnClickListener;
import android.content.Context;
import android.view.View;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.TextView;
import android.widget.EditText;
import android.media.tv.TvContract;

import com.android.tv.MainActivity;
import com.android.tv.ChannelTuner;
import com.android.tv.R;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.RadioButtonItem;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.data.ChannelNumber;
import com.android.tv.TvSingletons;
import com.android.tv.data.api.Channel;

import java.util.ArrayList;
import java.util.List;
import java.util.HashMap;
import java.util.Collections;
import java.util.Comparator;

import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.ChannelInfo;

public class ChannelModifyDetailsFragment extends SideFragment {
    private static final String TAG = "ChannelModifyDetailsFragment";

    private static final String BROADCAST_DELETE_ALL_CHANNELS = "action.delete.all.channels";
    private static final boolean mDebug = false;
    private ChannelSettingsManager mChannelSettingsManager;
    private ChannelTuner mChannelTuner;
    private long mChannelId;
    private String mChannelType;
    private long mLastChannelId = -1;
    private long mNextChannelId = -1;
    private static final int[] ITEM_TYPE = {R.string.channel_edit_edit, R.string.channel_edit_skip, R.string.channel_edit_delete,
        R.string.channel_edit_favourite, R.string.channel_move_up, R.string.channel_move_down};
    private static final int SET_EDIT = 0;
    private static final int SET_SKIP = 1;
    private static final int SET_DELETE = 2;
    private static final int SET_FAVOURITE = 3;
    private static final int SET_MOVE_UP = 4;
    private static final int SET_MOVE_DOWN = 5;

    public ChannelModifyDetailsFragment(String type) {
        mChannelType = type;
    }

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        String title = getArguments().getString("title");
        if (mDebug) Log.d(TAG, "[getTitle] title = " + title);
        return title;
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        List<ActionItem> mActionItems = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        mChannelId = getArguments().getLong("type");
        mLastChannelId = getArguments().getLong("lastid");
        mNextChannelId = getArguments().getLong("nextid");
        if (mDebug)  Log.d(TAG, "[getItemList] mChannelId = " + mChannelId);
        if (ITEM_TYPE != null) {
            int num = ITEM_TYPE.length;
            if (!getMainActivity().mQuickKeyInfo.isDtmbModeCountry()) {
                num = ITEM_TYPE.length - 2;//not dtmb, move is not need
            }
            for (int i = 0; i < num; i++) {
                if (i == SET_DELETE && TvContract.Channels.TYPE_OTHER.equals(mChannelType)) {
                    continue;//skip delete function for other type of channels as sync reason
                } else if (i == SET_FAVOURITE) {
                    continue;//hide add fav function, and set it by new fav list
                }
                items.add(new EditOptionItem(getString(ITEM_TYPE[i]), i, mChannelId));
            }
        }

        //items.addAll(mActionItems);
        return items;
    }

    public void setChannelType(String type) {
        mChannelType = type;
    }

    private class StringComparator implements Comparator<ChannelInfo> {
        public int compare(ChannelInfo object1, ChannelInfo object2) {
            String p1 = object1.getDisplayNumber();
            String p2 = object2.getDisplayNumber();
            return ChannelNumber.compare(p1, p2);
        }
    }

    private class EditOptionItem extends RadioButtonItem {
        private final long mId;
        private final int mTrackId;

        private EditOptionItem(String title, int trackid, long id) {
            super(title);
            mId = id;
            mTrackId = trackid;
        }

        @Override
        protected void onSelected() {
            super.onSelected();
            if (mChannelSettingsManager == null) {
                mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
            }
            if (mChannelTuner == null) {
                mChannelTuner = getMainActivity().mQuickKeyInfo.getChannelTuner();
            }
            if (mDebug)  Log.d(TAG, "[onSelected] mTrackId = " + mTrackId + ", getMainActivity().getCurrentChannelId() = " + getMainActivity().getCurrentChannelId()
                + ", mId = " + mId);
            if (mTrackId == SET_EDIT) {
                createEditDialogUi(mId);
            } else if (mTrackId == SET_SKIP) {
                mChannelSettingsManager.skipChannel(mId);
                Channel next = null;
                if (getMainActivity().getCurrentChannelId() == mId) {
                    if (mChannelTuner.moveToAdjacentBrowsableChannel(true)) {
                        next = mChannelTuner.getCurrentChannel();
                        Log.d(TAG, "onSelected skip next = " + next);
                        if (next != null) {
                            getMainActivity().tuneToChannel(next);
                        }
                    }
                }
                closeFragment();
            } else if (mTrackId == SET_DELETE) {
                mChannelSettingsManager.deleteChannel(mId);
                boolean deleteall = false;
                Channel movedone = null;
                if (getMainActivity().getCurrentChannelId() == mId) {
                    if (mChannelTuner.moveToAdjacentBrowsableChannel(true)) {
                        movedone = mChannelTuner.getCurrentChannel();
                        Log.d(TAG, "onSelected movedone = " + movedone);
                        if (movedone == null || (movedone != null && movedone.getId() == mId)) {
                            deleteall = true;
                        } else {
                            getMainActivity().tuneToChannel(movedone);
                        }
                    } else {
                        deleteall = true;
                    }
                }
                if (deleteall) {
                    Intent intent = new Intent();
                    intent.setAction(BROADCAST_DELETE_ALL_CHANNELS);
                    getMainActivity().sendBroadcast(intent);
                }
                closeFragment();
            } else if (mTrackId == SET_FAVOURITE) {
                mChannelSettingsManager.setFavouriteChannel(mId);
                closeFragment();
            } else if (mTrackId == SET_MOVE_UP) {
                updatChannelOrder(true);
                closeFragment();
            } else if (mTrackId == SET_MOVE_DOWN) {
                updatChannelOrder(false);
                closeFragment();
            }
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            //getMainActivity().selectSubtitleTrack(mOption, mTrackId);
        }
    }


    @Override
    public void onPause() {
        super.onPause();
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
    }

    private long channelid = 0;
    AlertDialog mAlertDialog;

    private void createEditDialogUi (long id) {
        LayoutInflater inflater = (LayoutInflater)getMainActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = inflater.inflate(R.layout.option_item_edit_dialog, null);

        AlertDialog.Builder builder = new AlertDialog.Builder(getMainActivity());
        mAlertDialog = builder.create();
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view);
        //mAlertDialog.getWindow().setLayout(150, 320);
        mAlertDialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
        mAlertDialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);

        TextView button_ok = (TextView)view.findViewById(R.id.ok);
        EditText edit_text = (EditText)view.findViewById(R.id.editText);
        edit_text.requestFocus();
        channelid = id;
        button_ok.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                if (edit_text.getText() != null && edit_text.getText().length() > 0) {
                    mChannelSettingsManager.setChannelName(channelid, edit_text.getText().toString());
                    mAlertDialog.dismiss();
                    closeFragment();
                }
            }
        });
    }

    private void updatChannelOrder(boolean up) {
        final ChannelInfo lastchannel = mChannelSettingsManager.getChannelInfoById(mLastChannelId);
        final ChannelInfo currentchannel = mChannelSettingsManager.getChannelInfoById(mChannelId);
        final ChannelInfo nextchannel = mChannelSettingsManager.getChannelInfoById(mNextChannelId);
        if (up) {
            if (lastchannel != null && currentchannel != null) {
                String lastnumber = lastchannel.getDisplayNumber();
                String current = currentchannel.getDisplayNumber();
                currentchannel.setDisplayNumber(lastnumber);
                lastchannel.setDisplayNumber(current);
                mChannelSettingsManager.updateChannelOrder(currentchannel);
                mChannelSettingsManager.updateChannelOrder(lastchannel);
            }
        } else {
            if (nextchannel != null && currentchannel != null) {
                String current = currentchannel.getDisplayNumber();
                String nextnumber = nextchannel.getDisplayNumber();
                currentchannel.setDisplayNumber(nextnumber);
                nextchannel.setDisplayNumber(current);
                mChannelSettingsManager.updateChannelOrder(currentchannel);
                mChannelSettingsManager.updateChannelOrder(nextchannel);
            }
        }
    }
}
