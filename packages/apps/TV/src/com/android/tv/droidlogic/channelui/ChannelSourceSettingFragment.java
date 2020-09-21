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

import android.view.View;
import android.widget.TextView;
import android.media.tv.TvInputInfo;
import android.media.tv.TvContract;

import com.android.tv.MainActivity;
import com.android.tv.R;
import com.android.tv.data.api.Channel;
import com.android.tv.dialog.PinDialogFragment;
import com.android.tv.dialog.PinDialogFragment.OnPinCheckedListener;
import com.android.tv.ui.sidepanel.ActionItem;
import com.android.tv.ui.sidepanel.Item;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.SubMenuItem;
import com.android.tv.ui.sidepanel.SwitchItem;
import com.android.tv.common.util.PermissionUtils;
import com.android.tv.ui.sidepanel.SideFragmentManager;
import com.android.tv.ui.sidepanel.parentalcontrols.ParentalControlsFragment;

import java.util.ArrayList;
import java.util.List;

import com.droidlogic.app.tv.TvControlManager;

public class ChannelSourceSettingFragment extends SideFragment {
    private static final String TAG = "ChannelSourceSettingFragment";
    private List<ActionItem> mActionItems;
    private ChannelSettingsManager mChannelSettingsManager;
    private static final int[] SWITCH_CHANNEL = {R.string.channel_switch_channel_static, R.string.channel_switch_channel_black};

    private final SideFragmentListener mSideFragmentListener = new SideFragmentListener() {
        @Override
        public void onSideFragmentViewDestroyed() {
            notifyDataSetChanged();
        }
    };

    @Override
    protected String getTitle() {
        return getString(R.string.channel_source_setting);
    }

    @Override
    public String getTrackerLabel() {
        return TAG;
    }

    @Override
    protected List<Item> getItemList() {
        List<Item> items = new ArrayList<>();
        mActionItems = new ArrayList<>();
        mChannelSettingsManager = new ChannelSettingsManager(getMainActivity());
        //set by select country or search type
        /*mActionItems.add(new SubMenuItem(getString(R.string.channel_dtv_type),
                mChannelSettingsManager.getDtvType(), getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new DtvTypeFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });*/
        mActionItems.add(new ActionItem(getString(R.string.channel_search_channel), null) {
            @Override
            protected void onSelected() {
                //get tuner tvinputinfo and then search channel
                TvInputInfo tunerinputinfo = getMainActivity().mQuickKeyInfo.getTunerInput();
                if (tunerinputinfo != null) {
                    getMainActivity().startSetupActivity(tunerinputinfo, true);
                }
             }
        });
        mActionItems.add(new ActionItem(getString(R.string.channels_item_dvr), null) {
            @Override
            protected void onSelected() {
                //show dvr manager
                getMainActivity().getOverlayManager().showDvrManager();
            }
        });
        mActionItems.add(new SubMenuItem(getString(R.string.channel_channel_edit),
                null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new ChannelEditFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });
        Channel currentChannel = getMainActivity().getCurrentChannel();
        if (PermissionUtils.hasModifyParentalControls(getMainActivity())) {
            if (currentChannel == null
                || (currentChannel != null && !currentChannel.getType().equals(TvContract.Channels.TYPE_DTMB))) {
                mActionItems.add(new ActionItem(getString(R.string.settings_parental_controls),
                        getString(getMainActivity().getParentalControlSettings().isParentalControlsEnabled()
                                ? R.string.option_toggle_parental_controls_on
                                : R.string.option_toggle_parental_controls_off)) {
                    @Override
                    protected void onSelected() {
                        final MainActivity tvActivity = getMainActivity();
                        final SideFragmentManager sideFragmentManager = tvActivity.getOverlayManager()
                                .getSideFragmentManager();
                        sideFragmentManager.hideSidePanel(true);
                        PinDialogFragment fragment = PinDialogFragment
                                .create(PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN);
                        getMainActivity().getOverlayManager()
                                .showDialogFragment(PinDialogFragment.DIALOG_TAG, fragment, true);
                    }
                });
            }
        } else {
            // Note: parental control is turned off, when MODIFY_PARENTAL_CONTROLS is not granted.
            // But, we may be able to turn on channel lock feature regardless of the permission.
            // It's TBD.
        }
        final int[] status = {R.string.channel_sound_channel_stereo, R.string.channel_sound_channel_left,
            R.string.channel_sound_channel_right};
        mActionItems.add(new SubMenuItem(getString(R.string.channel_sound_channel),
                getMainActivity().getResources().getString(status[mChannelSettingsManager.getSoundChannelStatus()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new SoundChannelFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });

        if (getMainActivity().mQuickKeyInfo.isAtvSource()) {
            final int[] videoStatus = {R.string.channel_video_pal, R.string.channel_video_ntsc, R.string.channel_video_secam};
            mActionItems.add(new SubMenuItem(getString(R.string.channel_video),
                    getMainActivity().getResources().getString(videoStatus[mChannelSettingsManager.getChannelVideoStatus()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ChannelVideoFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });
            final int[] audioStatus = {R.string.channel_audio_dk, R.string.channel_audio_i, R.string.channel_audio_bg, R.string.channel_audio_m, R.string.channel_audio_l};
            mActionItems.add(new SubMenuItem(getString(R.string.channel_audio),
                    getMainActivity().getResources().getString(audioStatus[mChannelSettingsManager.getChannelAudioStatus()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ChannelAudioFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });
        }
        //if (currentChannel == null || (currentChannel != null && !currentChannel.getType().equals(TvContract.Channels.TYPE_DTMB))) {
            //add in droidtvsetting to act as a product prop
            /*final int[] switchstatus = {R.string.channel_audio_ad_switch_off, R.string.channel_audio_ad_switch_on};
            mActionItems.add(new SubMenuItem(getString(R.string.channel_audio_ad_switch),
                    getMainActivity().getResources().getString(switchstatus[mChannelSettingsManager.getADSwitchStatus()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new AudioAdSwitchFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });*/
            if (mChannelSettingsManager.getADSwitchStatus() > 0) {
                mActionItems.add(new SubMenuItem(getString(R.string.channel_audio_ad_mix_level),
                        (mChannelSettingsManager.getADMixStatus() + "%"), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                    @Override
                    protected SideFragment getFragment() {
                        SideFragment fragment = new AudioAdMixLevelFragment();
                        fragment.setListener(mSideFragmentListener);
                        return fragment;
                    }
                });
            }
        //}
        mActionItems.add(new SubMenuItem(getString(R.string.channel_volume_compensate),
                String.valueOf(mChannelSettingsManager.getVolumeCompensateStatus() * 5)  + "%", getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new VolumeCompensateFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });
        if (getMainActivity().mQuickKeyInfo.isAtvSource()) {
            mActionItems.add(new SubMenuItem(getString(R.string.channel_fine_tune),
                    mChannelSettingsManager.getFineTuneStatus() + "%", getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ChannelFineTuneFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });
        }
        if (currentChannel != null && (!currentChannel.isRadioChannel() && !currentChannel.isOtherChannel())) {
            mActionItems.add(new SubMenuItem(getString(R.string.channel_switch_channel),
                getString(SWITCH_CHANNEL[mChannelSettingsManager.getSwitchChannelStatus()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ChannelSwitchFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });
        }

        if (mChannelSettingsManager.getCurentTvSource() == TvControlManager.SourceInput_Type.SOURCE_TYPE_TV) {
            final int[] stereostatus = {R.string.channel_audio_outmode_mono, R.string.channel_audio_outmode_stereo,
                R.string.channel_audio_outmode_sap};
            mActionItems.add(new SubMenuItem(getString(R.string.channel_audio_mts),
                    getString(stereostatus[mChannelSettingsManager.getAudioOutmode()]), getMainActivity().getOverlayManager().getSideFragmentManager()) {
                @Override
                protected SideFragment getFragment() {
                    SideFragment fragment = new ChannelStereoFragment();
                    fragment.setListener(mSideFragmentListener);
                    return fragment;
                }
            });
        }
        mActionItems.add(new SubMenuItem(getString(R.string.channel_info),
                null, getMainActivity().getOverlayManager().getSideFragmentManager()) {
            @Override
            protected SideFragment getFragment() {
                SideFragment fragment = new ChannelInfoFragment();
                fragment.setListener(mSideFragmentListener);
                return fragment;
            }
        });

        items.addAll(mActionItems);

        return items;
    }
}
