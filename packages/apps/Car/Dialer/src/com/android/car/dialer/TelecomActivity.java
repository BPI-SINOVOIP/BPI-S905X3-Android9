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
package com.android.car.dialer;

import static com.android.car.dialer.ui.CallHistoryFragment.CALL_TYPE_KEY;

import android.content.Intent;
import android.graphics.PorterDuff;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.telecom.Call;
import android.telephony.PhoneNumberUtils;
import android.util.Log;

import androidx.car.drawer.CarDrawerActivity;
import androidx.car.drawer.CarDrawerAdapter;
import androidx.car.drawer.DrawerItemViewHolder;

import com.android.car.dialer.telecom.InMemoryPhoneBook;
import com.android.car.dialer.telecom.PhoneLoader;
import com.android.car.dialer.telecom.UiCall;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.dialer.ui.CallHistoryFragment;
import com.android.car.dialer.ui.ContactListFragment;
import com.android.car.dialer.ui.InCallFragment;

import java.util.stream.Stream;

/**
 * Main activity for the Dialer app. Displays different fragments depending on call and
 * connectivity status:
 * <ul>
 * <li>OngoingCallFragment
 * <li>NoHfpFragment
 * <li>DialerFragment
 * <li>StrequentFragment
 * </ul>
 */
public class TelecomActivity extends CarDrawerActivity implements CallListener {
    private static final String TAG = "TelecomActivity";

    private static final String ACTION_ANSWER_CALL = "com.android.car.dialer.ANSWER_CALL";
    private static final String ACTION_END_CALL = "com.android.car.dialer.END_CALL";

    private static final String DIALER_BACKSTACK = "DialerBackstack";
    private static final String CONTENT_FRAGMENT_TAG = "CONTENT_FRAGMENT_TAG";
    private static final String DIALER_FRAGMENT_TAG = "DIALER_FRAGMENT_TAG";

    private final UiBluetoothMonitor.Listener mBluetoothListener = this::updateCurrentFragment;

    private UiCallManager mUiCallManager;
    private UiBluetoothMonitor mUiBluetoothMonitor;

    /**
     * Whether or not it is safe to make transactions on the
     * {@link android.support.v4.app.FragmentManager}. This variable prevents a possible exception
     * when calling commit() on the FragmentManager.
     *
     * <p>The default value is {@code true} because it is only after
     * {@link #onSaveInstanceState(Bundle)} that fragment commits are not allowed.
     */
    private boolean mAllowFragmentCommits = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setToolbarElevation(0f);

        if (vdebug()) {
            Log.d(TAG, "onCreate");
        }

        setMainContent(R.layout.telecom_activity);
        getWindow().getDecorView().setBackgroundColor(getColor(R.color.phone_theme));
        updateTitle();

        mUiCallManager = UiCallManager.init(getApplicationContext());
        mUiBluetoothMonitor = new UiBluetoothMonitor(this);

        InMemoryPhoneBook.init(getApplicationContext());

        findViewById(R.id.search).setOnClickListener(
                v -> startActivity(new Intent(this, ContactSearchActivity.class)));

        getDrawerController().setRootAdapter(new DialerRootAdapter());
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (vdebug()) {
            Log.d(TAG, "onDestroy");
        }
        mUiBluetoothMonitor.tearDown();
        InMemoryPhoneBook.tearDown();
        mUiCallManager.tearDown();
        mUiCallManager = null;
    }

    @Override
    protected void onStop() {
        super.onStop();
        mUiCallManager.removeListener(this);
        mUiBluetoothMonitor.removeListener(mBluetoothListener);
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        // A transaction can only be committed with this method prior to its containing activity
        // saving its state.
        mAllowFragmentCommits = false;
        super.onSaveInstanceState(outState);
    }

    @Override
    protected void onNewIntent(Intent i) {
        super.onNewIntent(i);
        setIntent(i);
    }

    @Override
    protected void onStart() {
        if (vdebug()) {
            Log.d(TAG, "onStart");
        }
        super.onStart();

        // Fragment commits are not allowed once the Activity's state has been saved. Once
        // onStart() has been called, the FragmentManager should now allow commits.
        mAllowFragmentCommits = true;

        // Update the current fragment before handling the intent so that any UI updates in
        // handleIntent() is not overridden by updateCurrentFragment().
        updateCurrentFragment();
        handleIntent();

        mUiCallManager.addListener(this);
        mUiBluetoothMonitor.addListener(mBluetoothListener);
    }

    private void handleIntent() {
        Intent intent = getIntent();
        String action = intent != null ? intent.getAction() : null;

        if (vdebug()) {
            Log.d(TAG, "handleIntent, intent: " + intent + ", action: " + action);
        }

        if (action == null || action.length() == 0) {
            return;
        }

        String number;
        UiCall ringingCall;
        switch (action) {
            case ACTION_ANSWER_CALL:
                ringingCall = mUiCallManager.getCallWithState(Call.STATE_RINGING);
                if (ringingCall == null) {
                    Log.e(TAG, "Unable to answer ringing call. There is none.");
                } else {
                    mUiCallManager.answerCall(ringingCall);
                }
                break;

            case ACTION_END_CALL:
                ringingCall = mUiCallManager.getCallWithState(Call.STATE_RINGING);
                if (ringingCall == null) {
                    Log.e(TAG, "Unable to end ringing call. There is none.");
                } else {
                    mUiCallManager.disconnectCall(ringingCall);
                }
                break;

            case Intent.ACTION_DIAL:
                number = PhoneNumberUtils.getNumberFromIntent(intent, this);
                if (!(getCurrentFragment() instanceof NoHfpFragment)) {
                    showDialer(number);
                }
                break;

            case Intent.ACTION_CALL:
                number = PhoneNumberUtils.getNumberFromIntent(intent, this);
                mUiCallManager.safePlaceCall(number, false /* bluetoothRequired */);
                break;

            default:
                // Do nothing.
        }

        setIntent(null);
    }

    /**
     * Updates the content fragment of this Activity based on the state of the application.
     */
    private void updateCurrentFragment() {
        if (vdebug()) {
            Log.d(TAG, "updateCurrentFragment()");
        }

        boolean callEmpty = mUiCallManager.getCalls().isEmpty();
        if (!mUiBluetoothMonitor.isBluetoothEnabled() && callEmpty) {
            showNoHfpFragment(R.string.bluetooth_disabled);
        } else if (!mUiBluetoothMonitor.isBluetoothPaired() && callEmpty) {
            showNoHfpFragment(R.string.bluetooth_unpaired);
        } else if (!mUiBluetoothMonitor.isHfpConnected() && callEmpty) {
            showNoHfpFragment(R.string.no_hfp);
        } else {
            UiCall ongoingCall = mUiCallManager.getPrimaryCall();

            if (vdebug()) {
                Log.d(TAG, "ongoingCall: " + ongoingCall + ", mCurrentFragment: "
                        + getCurrentFragment());
            }

            if (ongoingCall == null && getCurrentFragment() instanceof InCallFragment) {
                showSpeedDialFragment();
            } else if (ongoingCall != null) {
                showOngoingCallFragment();
            } else {
                showSpeedDialFragment();
            }
        }

        if (vdebug()) {
            Log.d(TAG, "updateCurrentFragment: done");
        }
    }

    private void showSpeedDialFragment() {
        if (vdebug()) {
            Log.d(TAG, "showSpeedDialFragment");
        }

        if (!mAllowFragmentCommits || getCurrentFragment() instanceof StrequentsFragment) {
            return;
        }

        Fragment fragment = StrequentsFragment.newInstance();
        setContentFragment(fragment);
    }

    private void showOngoingCallFragment() {
        if (vdebug()) {
            Log.d(TAG, "showOngoingCallFragment");
        }
        if (!mAllowFragmentCommits || getCurrentFragment() instanceof InCallFragment) {
            // in case the dialer is still open, (e.g. when dialing the second phone during
            // a phone call), close it
            maybeHideDialer();
            getDrawerController().closeDrawer();
            return;
        }
        Fragment fragment = InCallFragment.newInstance();
        setContentFragmentWithFadeAnimation(fragment);
        getDrawerController().closeDrawer();
    }

    private void showDialer() {
        if (vdebug()) {
            Log.d(TAG, "showDialer");
        }

        showDialer(null /* dialNumber */);
    }

    /**
     * Displays the {@link DialerFragment} and initialize it with the given phone number.
     */
    private void showDialer(@Nullable String dialNumber) {
        if (vdebug()) {
            Log.d(TAG, "showDialer with number: " + dialNumber);
        }

        if (!mAllowFragmentCommits ||
                getSupportFragmentManager().findFragmentByTag(DIALER_FRAGMENT_TAG) != null) {
            return;
        }

        Fragment fragment = DialerFragment.newInstance(dialNumber);
        // Add the dialer fragment to the backstack so that it can be popped off to dismiss it.
        setContentFragment(fragment);
    }

    /**
     * Checks if the dialpad fragment is opened and hides it if it is.
     */
    private void maybeHideDialer() {
        // The dialer is the only fragment to be added to the back stack. Dismiss the dialer by
        // removing it from the back stack.
        if (getSupportFragmentManager().getBackStackEntryCount() > 0) {
            getSupportFragmentManager().popBackStack();
        }
    }

    private void showNoHfpFragment(@StringRes int stringResId) {
        if (!mAllowFragmentCommits) {
            return;
        }

        String errorMessage = getString(stringResId);
        Fragment currentFragment = getCurrentFragment();

        if (currentFragment instanceof NoHfpFragment) {
            ((NoHfpFragment) currentFragment).setErrorMessage(errorMessage);
        } else {
            setContentFragment(NoHfpFragment.newInstance(errorMessage));
        }
    }

    private void setContentFragmentWithSlideAndDelayAnimation(Fragment fragment) {
        if (vdebug()) {
            Log.d(TAG, "setContentFragmentWithSlideAndDelayAnimation, fragment: " + fragment);
        }
        setContentFragmentWithAnimations(fragment,
                R.anim.telecom_slide_in_with_delay, R.anim.telecom_slide_out);
    }

    private void setContentFragmentWithFadeAnimation(Fragment fragment) {
        if (vdebug()) {
            Log.d(TAG, "setContentFragmentWithFadeAnimation, fragment: " + fragment);
        }
        setContentFragmentWithAnimations(fragment,
                R.anim.telecom_fade_in, R.anim.telecom_fade_out);
    }

    private void setContentFragmentWithAnimations(Fragment fragment, int enter, int exit) {
        if (vdebug()) {
            Log.d(TAG, "setContentFragmentWithAnimations: " + fragment);
        }

        maybeHideDialer();

        getSupportFragmentManager().beginTransaction()
                .setCustomAnimations(enter, exit)
                .replace(R.id.content_fragment_container, fragment, CONTENT_FRAGMENT_TAG)
                .commitNow();
    }

    /**
     * Sets the fragment that will be shown as the main content of this Activity. Note that this
     * fragment is not always visible. In particular, the dialer fragment can show up on top of this
     * fragment.
     */
    private void setContentFragment(Fragment fragment) {
        maybeHideDialer();
        getSupportFragmentManager().beginTransaction()
                .replace(R.id.content_fragment_container, fragment, CONTENT_FRAGMENT_TAG)
                .commitNow();
        updateTitle();
    }

    /**
     * Returns the fragment that is currently being displayed as the content view. Note that this
     * is not necessarily the fragment that is visible. For example, the returned fragment
     * could be the content, but the dial fragment is being displayed on top of it. Check for
     * the existence of the dial fragment with the TAG {@link #DIALER_FRAGMENT_TAG}.
     */
    @Nullable
    private Fragment getCurrentFragment() {
        return getSupportFragmentManager().findFragmentByTag(CONTENT_FRAGMENT_TAG);
    }

    private static boolean vdebug() {
        return Log.isLoggable(TAG, Log.DEBUG);
    }

    @Override
    public void onAudioStateChanged(boolean isMuted, int route, int supportedRouteMask) {
        fragmentsToPropagateCallback().forEach(fragment -> ((CallListener) fragment)
                .onAudioStateChanged(isMuted, route, supportedRouteMask));
    }

    @Override
    public void onCallStateChanged(UiCall call, int state) {
        if (vdebug()) {
            Log.d(TAG, "onCallStateChanged");
        }
        updateCurrentFragment();

        fragmentsToPropagateCallback().forEach(fragment -> ((CallListener) fragment)
                .onCallStateChanged(call, state));
    }

    @Override
    public void onCallUpdated(UiCall call) {
        if (vdebug()) {
            Log.d(TAG, "onCallUpdated");
        }
        updateCurrentFragment();

        fragmentsToPropagateCallback().forEach(fragment -> ((CallListener) fragment)
                .onCallUpdated(call));
    }

    @Override
    public void onCallAdded(UiCall call) {
        if (vdebug()) {
            Log.d(TAG, "onCallAdded");
        }
        updateCurrentFragment();

        fragmentsToPropagateCallback().forEach(fragment -> ((CallListener) fragment)
                .onCallAdded(call));
    }

    @Override
    public void onCallRemoved(UiCall call) {
        if (vdebug()) {
            Log.d(TAG, "onCallRemoved");
        }
        updateCurrentFragment();

        fragmentsToPropagateCallback().forEach(fragment -> ((CallListener) fragment)
                .onCallRemoved(call));
    }

    private static boolean shouldPropagateCallback(Fragment fragment) {
        return fragment instanceof CallListener && fragment.isAdded();
    }

    private Stream<Fragment> fragmentsToPropagateCallback() {
        return getSupportFragmentManager().getFragments().stream()
                .filter(fragment -> shouldPropagateCallback(fragment));
    }

    private class DialerRootAdapter extends CarDrawerAdapter {
        private static final int ITEM_FAVORITES = 0;
        private static final int ITEM_CALLLOG_ALL = 1;
        private static final int ITEM_CALLLOG_MISSED = 2;
        private static final int ITEM_CONTACT = 3;
        private static final int ITEM_DIAL = 4;

        private static final int ITEM_COUNT = 5;

        DialerRootAdapter() {
            super(TelecomActivity.this, false /* showDisabledListOnEmpty */);
        }

        @Override
        protected int getActualItemCount() {
            return ITEM_COUNT;
        }

        @Override
        public void populateViewHolder(DrawerItemViewHolder holder, int position) {
            final int iconColor = getResources().getColor(R.color.car_tint);
            int textResId, iconResId;
            switch (position) {
                case ITEM_DIAL:
                    textResId = R.string.calllog_dial_number;
                    iconResId = R.drawable.ic_drawer_dialpad;
                    break;
                case ITEM_CALLLOG_ALL:
                    textResId = R.string.calllog_all;
                    iconResId = R.drawable.ic_drawer_history;
                    break;
                case ITEM_CALLLOG_MISSED:
                    textResId = R.string.calllog_missed;
                    iconResId = R.drawable.ic_call_missed;
                    break;
                case ITEM_FAVORITES:
                    textResId = R.string.calllog_favorites;
                    iconResId = R.drawable.ic_favorite;
                    break;
                case ITEM_CONTACT:
                    textResId = R.string.contact_menu_label;
                    iconResId = R.drawable.ic_contact;
                    break;
                default:
                    Log.wtf(TAG, "Unexpected position: " + position);
                    return;
            }
            holder.getTitle().setText(textResId);
            Drawable drawable = getDrawable(iconResId);
            drawable.setColorFilter(iconColor, PorterDuff.Mode.SRC_IN);
            holder.getIcon().setImageDrawable(drawable);
        }

        @Override
        public void onItemClick(int position) {
            getDrawerController().closeDrawer();
            switch (position) {
                case ITEM_DIAL:
                    showDialer();
                    break;
                case ITEM_CALLLOG_ALL:
                    showCallHistory(PhoneLoader.CallType.CALL_TYPE_ALL);
                    break;
                case ITEM_CALLLOG_MISSED:
                    showCallHistory(PhoneLoader.CallType.MISSED_TYPE);
                    break;
                case ITEM_FAVORITES:
                    showSpeedDialFragment();
                    break;
                case ITEM_CONTACT:
                    showContact();
                    break;
                default:
                    Log.w(TAG, "Invalid position in ROOT menu! " + position);
            }
            setTitle(getTitleString());
        }
    }

    private void showCallHistory(@PhoneLoader.CallType int callType) {
        setContentFragment(CallHistoryFragment.newInstance(callType));
    }

    private void showContact() {
        setContentFragment(ContactListFragment.newInstance());
    }

    private void updateTitle() {
        setTitle(getTitleString());
    }

    private String getTitleString() {
        Fragment currentFragment = getCurrentFragment();

        int titleResId = R.string.phone_app_name;

        if (currentFragment instanceof StrequentsFragment) {
            titleResId = R.string.contacts_title;
        } else if (currentFragment instanceof CallHistoryFragment) {
            int callType = currentFragment.getArguments().getInt(CALL_TYPE_KEY);
            if (callType == PhoneLoader.CallType.MISSED_TYPE) {
                titleResId = R.string.missed_call_title;
            } else {
                titleResId = R.string.call_history_title;
            }
        } else if (currentFragment instanceof ContactListFragment) {
            titleResId = R.string.contacts_title;
        } else if (currentFragment instanceof DialerFragment) {
            titleResId = R.string.dialpad_title;
        } else if (currentFragment instanceof InCallFragment
                || currentFragment instanceof OngoingCallFragment) {
            titleResId = R.string.in_call_title;
        }

        return getString(titleResId);
    }
}
