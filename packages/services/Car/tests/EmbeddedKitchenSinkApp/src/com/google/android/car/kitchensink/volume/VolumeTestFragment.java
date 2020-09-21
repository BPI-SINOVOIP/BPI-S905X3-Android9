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
package com.google.android.car.kitchensink.volume;

import android.car.Car;
import android.car.CarNotConnectedException;
import android.car.media.CarAudioManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.ServiceConnection;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.util.SparseIntArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.SeekBar;

import com.google.android.car.kitchensink.R;

public class VolumeTestFragment extends Fragment {
    private static final String TAG = "CarVolumeTest";
    private static final int MSG_VOLUME_CHANGED = 0;
    private static final int MSG_REQUEST_FOCUS = 1;
    private static final int MSG_FOCUS_CHANGED= 2;

    private AudioManager mAudioManager;
    private VolumeAdapter mAdapter;

    private CarAudioManager mCarAudioManager;
    private Car mCar;

    private SeekBar mFader;
    private SeekBar mBalance;

    private final Handler mHandler = new VolumeHandler();

    private class VolumeHandler extends Handler {
        private AudioFocusListener mFocusListener;

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_VOLUME_CHANGED:
                    initVolumeInfo();
                    break;
                case MSG_REQUEST_FOCUS:
                    int groupId = msg.arg1;
                    if (mFocusListener != null) {
                        mAudioManager.abandonAudioFocus(mFocusListener);
                        mVolumeInfos[mGroupIdIndexMap.get(groupId)].mHasFocus = false;
                        mAdapter.notifyDataSetChanged();
                    }

                    mFocusListener = new AudioFocusListener(groupId);
                    mAudioManager.requestAudioFocus(mFocusListener, groupId,
                            AudioManager.AUDIOFOCUS_GAIN);
                    break;
                case MSG_FOCUS_CHANGED:
                    int focusGroupId = msg.arg1;
                    mVolumeInfos[mGroupIdIndexMap.get(focusGroupId)].mHasFocus = true;
                    mAdapter.refreshVolumes(mVolumeInfos);
                    break;

            }
        }
    }

    private VolumeInfo[] mVolumeInfos = new VolumeInfo[0];
    private SparseIntArray mGroupIdIndexMap = new SparseIntArray();

    private class AudioFocusListener implements AudioManager.OnAudioFocusChangeListener {
        private final int mGroupId;
        public AudioFocusListener(int groupId) {
            mGroupId = groupId;
        }
        @Override
        public void onAudioFocusChange(int focusChange) {
            if (focusChange == AudioManager.AUDIOFOCUS_GAIN) {
                mHandler.sendMessage(mHandler.obtainMessage(MSG_FOCUS_CHANGED, mGroupId, 0));
            } else {
                Log.e(TAG, "Audio focus request failed");
            }
        }
    }

    public static class VolumeInfo {
        public int mGroupId;
        public String mId;
        public String mMax;
        public String mCurrent;
        public boolean mHasFocus;
    }

    private final ServiceConnection mCarConnectionCallback =
            new ServiceConnection() {
                @Override
                public void onServiceConnected(ComponentName name, IBinder binder) {
                    Log.d(TAG, "Connected to Car Service");
                    try {
                        mCarAudioManager = (CarAudioManager) mCar.getCarManager(Car.AUDIO_SERVICE);
                        initVolumeInfo();
                    } catch (CarNotConnectedException e) {
                        Log.e(TAG, "Car is not connected!", e);
                    }
                }

                @Override
                public void onServiceDisconnected(ComponentName name) {
                    Log.d(TAG, "Disconnect from Car Service");
                }
            };

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.volume_test, container, false);

        ListView volumeListView = v.findViewById(R.id.volume_list);
        mAudioManager = (AudioManager) getContext().getSystemService(Context.AUDIO_SERVICE);

        mAdapter = new VolumeAdapter(getContext(), R.layout.volume_item, mVolumeInfos, this);
        volumeListView.setAdapter(mAdapter);

        v.findViewById(R.id.refresh).setOnClickListener((view) -> initVolumeInfo());

        final SeekBar.OnSeekBarChangeListener seekListener = new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                final float percent = (progress - 100) / 100.0f;
                try {
                    if (seekBar.getId() == R.id.fade_bar) {
                        mCarAudioManager.setFadeTowardFront(percent);
                    } else {
                        mCarAudioManager.setBalanceTowardRight(percent);
                    }
                } catch (CarNotConnectedException e) {
                    Log.e(TAG, "Can't adjust fade or balance when car not connected", e);
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {}

            public void onStopTrackingTouch(SeekBar seekBar) {}
        };

        mFader = v.findViewById(R.id.fade_bar);
        mFader.setOnSeekBarChangeListener(seekListener);

        mBalance = v.findViewById(R.id.balance_bar);
        mBalance.setOnSeekBarChangeListener(seekListener);

        mCar = Car.createCar(getActivity(), mCarConnectionCallback);
        mCar.connect();
        return v;
    }

    public void adjustVolumeByOne(int groupId, boolean up) {
        if (mCarAudioManager == null) {
            Log.e(TAG, "CarAudioManager is null");
            return;
        }
        int current = 0;
        try {
            current = mCarAudioManager.getGroupVolume(groupId);
            int volume = current + (up ? 1 : -1);
            mCarAudioManager.setGroupVolume(groupId, volume,
                    AudioManager.FLAG_SHOW_UI | AudioManager.FLAG_PLAY_SOUND);
            Log.d(TAG, "Set group " + groupId + " volume " + volume);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "car not connected", e);
        }
    }

    public void requestFocus(int groupId) {
        mHandler.sendMessage(mHandler.obtainMessage(MSG_REQUEST_FOCUS, groupId));
    }

    private void initVolumeInfo() {
        try {
            int volumeGroupCount = mCarAudioManager.getVolumeGroupCount();
            mVolumeInfos = new VolumeInfo[volumeGroupCount + 1];
            mGroupIdIndexMap.clear();
            mVolumeInfos[0] = new VolumeInfo();
            mVolumeInfos[0].mId = "Group id";
            mVolumeInfos[0].mCurrent = "Current";
            mVolumeInfos[0].mMax = "Max";

            int i = 1;
            for (int groupId = 0; groupId < volumeGroupCount; groupId++) {
                mVolumeInfos[i] = new VolumeInfo();
                mVolumeInfos[i].mGroupId = groupId;
                mGroupIdIndexMap.put(groupId, i);
                mVolumeInfos[i].mId = String.valueOf(groupId);


                int current = mCarAudioManager.getGroupVolume(groupId);
                int max = mCarAudioManager.getGroupMaxVolume(groupId);
                mVolumeInfos[i].mCurrent = String.valueOf(current);
                mVolumeInfos[i].mMax = String.valueOf(max);

                Log.d(TAG, groupId + " max: " + mVolumeInfos[i].mMax + " current: "
                        + mVolumeInfos[i].mCurrent);
                i++;
            }
            mAdapter.refreshVolumes(mVolumeInfos);
        } catch (CarNotConnectedException e) {
            Log.e(TAG, "car not connected", e);
        }
    }

    @Override
    public void onDestroy() {
        if (mCar != null) {
            mCar.disconnect();
        }
        super.onDestroy();
    }
}
