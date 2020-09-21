/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.tv.settings.connectivity.setup;

import android.arch.lifecycle.ViewModelProviders;
import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v17.leanback.widget.GuidanceStylist;
import android.support.v17.leanback.widget.GuidedAction;
import android.support.v17.leanback.widget.GuidedActionsStylist;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.text.TextUtils;
import android.util.Pair;
import android.view.View;

import com.android.settingslib.wifi.AccessPoint;
import com.android.tv.settings.R;
import com.android.tv.settings.connectivity.util.State;
import com.android.tv.settings.connectivity.util.StateMachine;
import com.android.tv.settings.connectivity.util.WifiSecurityUtil;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;

/**
 * State responsible for selecting Wi-Fi.
 */
public class SelectWifiState implements State {
    private Fragment mFragment;
    private FragmentActivity mActivity;

    public SelectWifiState(FragmentActivity wifiSetupActivity) {
        mActivity = wifiSetupActivity;
    }

    @Override
    public void processForward() {
        mFragment = new SelectWifiFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, true);
        }
    }

    @Override
    public void processBackward() {
        mFragment = new SelectWifiFragment();
        FragmentChangeListener listener = (FragmentChangeListener) mActivity;
        if (listener != null) {
            listener.onFragmentChange(mFragment, false);
        }
    }

    @Override
    public Fragment getFragment() {
        return mFragment;
    }

    /**
     * Fragment that shows a list of Wi-Fi for users to choose from.
     */
    public static class SelectWifiFragment extends WifiConnectivityGuidedStepFragment {
        private static final int RESULT_NETWORK_SKIPPED = 3;
        private NetworkListInfo mNetworkListInfo;
        private StateMachine mStateMachine;
        private UserChoiceInfo mUserChoiceInfo;
        private WifiGuidedActionComparator mWifiComparator = new WifiGuidedActionComparator();

        @Override
        public void onCreate(Bundle savedInstanceState) {
            mNetworkListInfo = ViewModelProviders.of(getActivity())
                    .get(NetworkListInfo.class);
            mUserChoiceInfo = ViewModelProviders
                    .of(getActivity())
                    .get(UserChoiceInfo.class);
            mStateMachine = ViewModelProviders
                    .of(getActivity())
                    .get(StateMachine.class);
            super.onCreate(savedInstanceState);
        }

        void updateNetworkList() {
            int lastSelectedActionPosition = getSelectedActionPosition();
            CharSequence lastWifiTitle = null;
            if (lastSelectedActionPosition != -1) {
                lastWifiTitle = getActions().get(lastSelectedActionPosition).getTitle();
            }
            List<WifiGuidedAction> newWifiActionList = getNetworks();
            List<GuidedAction> list = new ArrayList<>(newWifiActionList);
            setActions(list);
            moveToPosition(lastWifiTitle);
        }

        private void moveToPosition(CharSequence title) {
            if (title == null) return;
            for (int i = 0; i < getActions().size(); i++) {
                if (TextUtils.equals(getActions().get(i).getTitle(), title)) {
                    setSelectedActionPosition(i);
                    break;
                }
            }
        }

        @NonNull
        @Override
        public GuidanceStylist.Guidance onCreateGuidance(Bundle savedInstanceState) {
            return new GuidanceStylist.Guidance(
                    getString(R.string.title_select_wifi_network),
                    null,
                    null,
                    null);
        }

        @Override
        public GuidedActionsStylist onCreateActionsStylist() {
            GuidedActionsStylist guidedActionsStylist = new GuidedActionsStylist() {
                @Override
                public void onBindViewHolder(ViewHolder vh, GuidedAction action) {
                    super.onBindViewHolder(vh, action);
                    WifiGuidedAction wifiAction = (WifiGuidedAction) action;
                    boolean hasIconLevel = wifiAction.hasIconLevel();
                    if (hasIconLevel) {
                        vh.getIconView().setImageLevel(wifiAction.getIconLevel());
                    }
                }
            };
            return guidedActionsStylist;
        }

        @Override
        public void onCreateActions(@NonNull List<GuidedAction> actions,
                Bundle savedInstanceState) {
            actions.addAll(getNetworks());
        }

        private ArrayList<WifiGuidedAction> getNetworks() {
            Context context = getActivity();
            ArrayList<WifiGuidedAction> actions = new ArrayList<>();

            final List<ScanResult> results =
                    mNetworkListInfo.getWifiTracker().getManager().getScanResults();
            final HashMap<Pair<String, Integer>, ScanResult> consolidatedScanResults =
                    new HashMap<>();
            if (results == null) return new ArrayList<>();
            for (ScanResult result : results) {
                if (TextUtils.isEmpty(result.SSID)) {
                    continue;
                }

                Pair<String, Integer> key =
                        new Pair<>(result.SSID, WifiSecurityUtil.getSecurity(result));
                ScanResult existing = consolidatedScanResults.get(key);
                if (existing == null || existing.level < result.level) {
                    consolidatedScanResults.put(key, result);
                }
            }
            for (ScanResult result : consolidatedScanResults.values()) {
                int iconResource = AccessPoint.SECURITY_NONE == WifiSecurityUtil.getSecurity(result)
                        ? R.drawable.setup_wifi_signal_open
                        : R.drawable.setup_wifi_signal_lock;
                actions.add(new WifiGuidedAction.Builder(context)
                        .title(result.SSID)
                        .icon(iconResource)
                        .setHasIconLevel(true)
                        .setIconLevel(WifiManager.calculateSignalLevel(result.level, 4))
                        .setScanResult(result)
                        .build());
            }

            actions.sort(mWifiComparator);

            if (mNetworkListInfo.isShowSkipNetwork()) {
                actions.add(new WifiGuidedAction.Builder(context)
                        .title(R.string.skip_network)
                        .id(GuidedAction.ACTION_ID_CANCEL)
                        .icon(R.drawable.ic_arrow_forward)
                        .setHasIconLevel(false)
                        .build());
            }

            actions.add(new WifiGuidedAction.Builder(context)
                    .title(R.string.other_network)
                    .icon(R.drawable.ic_wifi_add)
                    .setHasIconLevel(false)
                    .build());

            return actions;
        }

        @Override
        public void onViewCreated(@NonNull View view, Bundle savedInstanceState) {
            super.onViewCreated(view, savedInstanceState);
            CharSequence title = mUserChoiceInfo.getPageSummary(UserChoiceInfo.SELECT_WIFI);
            if (title != null) {
                moveToPosition(title);
            }
        }

        @Override
        public void onGuidedActionFocused(GuidedAction action) {
            mNetworkListInfo.updateNextNetworkRefreshTime();
        }

        @Override
        public void onGuidedActionClicked(GuidedAction action) {
            if (action.getId() == GuidedAction.ACTION_ID_CANCEL) {
                mStateMachine.finish(RESULT_NETWORK_SKIPPED);
            } else {
                mUserChoiceInfo.put(UserChoiceInfo.SELECT_WIFI, action.getTitle().toString());
                mUserChoiceInfo.setChosenNetwork(((WifiGuidedAction) action).getScanResult());

                mStateMachine.getListener().onComplete(
                        StateMachine.ADD_PAGE_BASED_ON_NETWORK_CHOICE);
            }
        }

        private static class WifiGuidedActionComparator implements Comparator<WifiGuidedAction> {
            @Override
            public int compare(WifiGuidedAction o1, WifiGuidedAction o2) {
                int levelDiff = o2.getIconLevel() - o1.getIconLevel();
                if (levelDiff != 0) {
                    return levelDiff;
                }
                return o1.getTitle().toString().compareTo(o2.getTitle().toString());
            }
        }

        private static class WifiGuidedAction extends GuidedAction {
            ScanResult mScanResult;
            int mIconLevel;
            boolean mHasIconLevel;

            ScanResult getScanResult() {
                return mScanResult;
            }

            void setScanResult(ScanResult scanResult) {
                this.mScanResult = scanResult;
            }

            int getIconLevel() {
                return mIconLevel;
            }

            void setIconLevel(int iconLevel) {
                this.mIconLevel = iconLevel;
            }

            void setHasIconLevel(boolean hasIconLevel) {
                this.mHasIconLevel = hasIconLevel;
            }

            boolean hasIconLevel() {
                return mHasIconLevel;
            }

            static class Builder extends BuilderBase<Builder> {
                ScanResult mScanResult;
                int mIconLevel;
                boolean mHasIconLevel;

                private Builder(Context context) {
                    super(context);
                }

                Builder setScanResult(ScanResult scanResult) {
                    this.mScanResult = scanResult;
                    return this;
                }

                Builder setIconLevel(int iconLevel) {
                    this.mIconLevel = iconLevel;
                    return this;
                }

                Builder setHasIconLevel(boolean hasIconLevel) {
                    this.mHasIconLevel = hasIconLevel;
                    return this;
                }

                SelectWifiState.SelectWifiFragment.WifiGuidedAction build() {
                    SelectWifiState.SelectWifiFragment.WifiGuidedAction

                            action = new SelectWifiState.SelectWifiFragment.WifiGuidedAction();
                    action.setScanResult(mScanResult);
                    action.setHasIconLevel(mHasIconLevel);
                    action.setIconLevel(mIconLevel);
                    applyValues(action);
                    return action;
                }
            }
        }
    }
}
