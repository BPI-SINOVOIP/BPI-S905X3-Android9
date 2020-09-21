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

package com.droidlogic.droidlivetv.ui;



import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import android.app.Activity;
import android.app.Fragment;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.graphics.drawable.RippleDrawable;
import android.os.Bundle;
import android.support.v17.leanback.widget.VerticalGridView;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Adapter;
import android.widget.TextView;

import com.droidlogic.app.DroidLogicKeyEvent;

import com.droidlogic.droidlivetv.R;
import com.droidlogic.droidlivetv.SetParameters;

public class MultiOptionFragment extends SideFragment {
    private static final String TAG = "MultiOptionFragment";
    private int mInitialSelectedPosition = INVALID_POSITION;
    private String mSelectedTrackId, mListSelectedTrackId;
    private String mFocusedTrackId, mListFocusedTrackId;
    private int mKeyValue;

    private final static String[] TYPE = {"PMODE", "SMODE", "RATIO", "FAV", "LIST", "SLEEP"};
    private final static String[] LISTKEYWORD = {"PROGLIST", "FAVLIST"};
    private final static int[] KEYVALUE = {DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE, DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE
        , DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE, DroidLogicKeyEvent.KEYCODE_FAV,
        DroidLogicKeyEvent.KEYCODE_LIST, DroidLogicKeyEvent.KEYCODE_TV_SLEEP};
    private final static int[] PICTRUEMODE = {R.string.picture_mode, R.string.standard, R.string.vivid, R.string.soft, R.string.sport, R.string.movie, R.string.user};
    private final static int[] HDMI_PICTRUEMODE = {R.string.picture_mode, R.string.standard, R.string.vivid, R.string.soft, R.string.sport, R.string.movie, R.string.monitor, R.string.user};
    private final static int[] SOUNDMODE = {R.string.sound_mode, R.string.standard, R.string.music, R.string.news, R.string.movie, R.string.game, R.string.user};
    private final static int[] RATIOMODE = {R.string.aspect_ratio, R.string.auto, R.string.four2three, R.string.panorama, R.string.full_screen,};
    private final static int[] FAV = {R.string.favourite_list, R.string.tv, R.string.radio};
    private final static int[] LIST = {R.string.channel_list, R.string.tv, R.string.radio};
    private final static int[] SLEEPMODE = {R.string.sleep_timer, R.string.off, R.string.time_15min, R.string.time_30min, R.string.time_45min, R.string.time_60min,
        R.string.time_90min, R.string.time_120min};
    private final static int TVLIST = 0;
    private final static int RADIOLIST = 1;
    private final static int HIDELIST = -1;
    private boolean isListShow = false;
    private Bundle mBundle;
    private int mDeviceId;
    private long mChannelId;
    private Context mContext;
    private SetParameters mSetParameters;

    public MultiOptionFragment(Bundle bundle, Context context) {
        super(bundle.getInt("eventkey"));
        this.mKeyValue = bundle.getInt("eventkey");
        this.mBundle = bundle;
        this.mDeviceId = bundle.getInt("deviceid");
        this.mChannelId = bundle.getLong("channelid");
        this.mContext = context;
        this.mSetParameters = new SetParameters(context, bundle);
        Log.d(TAG, "mKeyValue: " + mKeyValue + ", mDeviceId: " + mDeviceId + ", mChannelId: " + mChannelId);
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
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE: {
                title = getString(PICTRUEMODE[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE: {
                title = getString(SOUNDMODE[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE: {
                title = getString(RATIOMODE[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_FAV: {
                title = getString(FAV[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_LIST: {
                title = getString(LIST[0]);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP: {
                title = getString(SLEEPMODE[0]);
                break;
            }
        }
        return title;
    }

    private String getKeyWords(int value) {
        String keywords = null;
        switch (value) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE: {
                keywords = TYPE[0];
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE: {
                keywords = TYPE[1];
            	break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE: {
                keywords = TYPE[2];
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_FAV: {
                keywords = TYPE[3];
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_LIST: {
                keywords = TYPE[4];
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP: {
                keywords = TYPE[5];
                break;
            }
        }
        return keywords;
    }

    private String[] getAllOption(int Value) {
        int[] id = null;
        switch (Value) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE: {
                id = mSetParameters.isHdmiSource() ? HDMI_PICTRUEMODE : PICTRUEMODE;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE: {
                id = SOUNDMODE;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE: {
                id = RATIOMODE;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_FAV: {
                id = FAV;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_LIST: {
                id = LIST;
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP: {
                id = SLEEPMODE;
                break;
            }
        }
        if (id != null) {
            String[] idtostring = new String[id.length];
            for (int i = 0; i < id.length; i++) {
                idtostring[i] = getString(id[i]);
            }
            return idtostring;
        }
        return null;
    }

    @Override
    protected List<Item> getItemList() {
        String[] getoption = getAllOption(mKeyValue);
        //String slectedindextest = read(getKeyWords(mKeyValue));
        String slectedindex = String.valueOf(getParameter(mKeyValue));
        List<Item> items = new ArrayList<>();
        if (getoption != null) {
            int pos = 0;
            for (int i = 1; i < getoption.length; i++) {
                RadioButtonItem item = new MultiAudioOptionItem(
                    getoption[i], String.valueOf(i - 1));
                if (slectedindex.equals(String.valueOf(i - 1))) {
                    item.setChecked(true);
                    mInitialSelectedPosition = pos;
                    mSelectedTrackId = mFocusedTrackId = String.valueOf(i - 1);
                } else if (slectedindex.equals("-1") && (i - 1) == 0) {
                    item.setChecked(true);
                    mInitialSelectedPosition = pos;
                    mSelectedTrackId = mFocusedTrackId = String.valueOf(i - 1);
                }
                items.add(item);
                ++pos;
            }
        }
        return items;
    }

    private void setParameter(int keyvalue, int mode) {
        Log.d(TAG, "[setParameter] keyvalue: " + keyvalue + ", mode: " + mode);
        switch (keyvalue) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE: {
                mSetParameters.setPictureMode(mode);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE: {
           	    mSetParameters.setSoundMode(mode);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE: {
                mSetParameters.setAspectRatio(mode);
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP: {
                mSetParameters.setSleepTimer(mode);
                break;
            }
        }
    }

    private int getParameter(int keyvalue) {
        int value = -1;
        switch (keyvalue) {
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VIEWMODE: {
                value = mSetParameters.getPictureModeStatus();
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_VOICEMODE: {
                value = mSetParameters.getSoundModeStatus();
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE: {
                value = mSetParameters.getAspectRatioStatus();
                break;
            }
            case DroidLogicKeyEvent.KEYCODE_TV_SLEEP: {
                value = mSetParameters.getSleepTimerStatus();
                break;
            }
        }
        Log.d(TAG, "[getParameter] keyvalue: " + keyvalue + ", value: " + value);
        return value;
    }

    @Override
    public void onResume() {
        super.onResume();
        if (mInitialSelectedPosition != INVALID_POSITION) {
            setSelectedPosition(mInitialSelectedPosition);
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
	        //String previous = read(getKeyWords(mKeyValue));
            //store(getKeyWords(mKeyValue), mTrackId);
            setParameter(mKeyValue, Integer.parseInt(mTrackId));
            
            //closeFragment();
        }

        @Override
        protected void onFocused() {
            super.onFocused();
            mFocusedTrackId = mTrackId;
        }
    }

    @Override
    public void onDetach() {
        super.onDetach();
    }
    /*
    private void store(String keyword, String content) { 
        SharedPreferences DealData = getMainActivity().getApplicationContext().getSharedPreferences("database", 0);
        Editor editor = DealData.edit();
        editor.putString(keyword, content);
        editor.commit(); 
        Log.d(TAG, "STORE keyword: " + keyword + ",content: " + content);
    }

    private String read(String keyword) {
        SharedPreferences DealData = getMainActivity().getApplicationContext().getSharedPreferences("database", 0);
        Log.d(TAG, "READ keyword: " + keyword + ",value: " + DealData.getString(keyword, "0"));
        return DealData.getString(keyword, "-1"); 
    }*/
}