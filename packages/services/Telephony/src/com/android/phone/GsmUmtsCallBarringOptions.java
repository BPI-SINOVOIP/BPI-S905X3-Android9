/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.phone;

import android.app.ActionBar;
import android.app.Dialog;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.Preference;
import android.preference.PreferenceScreen;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.util.Log;
import android.view.MenuItem;
import android.widget.Toast;

import com.android.internal.telephony.CommandException;
import com.android.internal.telephony.CommandsInterface;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.imsphone.ImsPhone;
import com.android.phone.settings.fdn.EditPinPreference;

import java.util.ArrayList;

/**
 * Implements the preference to enable/disable calling barring options and
 * the dialogs to change the passward.
 */
public class GsmUmtsCallBarringOptions extends TimeConsumingPreferenceActivity
        implements EditPinPreference.OnPinEnteredListener {
    private static final String LOG_TAG = "GsmUmtsCallBarringOptions";
    private static final boolean DBG = (PhoneGlobals.DBG_LEVEL >= 2);

    // String keys for preference lookup
    // Preference is handled solely in xml.
    // Block all outgoing calls
    private static final String BUTTON_BAOC_KEY = "button_baoc_key";
    // Block all outgoing international calls
    private static final String BUTTON_BAOIC_KEY = "button_baoic_key";
    // Block all outgoing international roaming calls
    private static final String BUTTON_BAOICxH_KEY = "button_baoicxh_key";
    // Block all incoming calls
    private static final String BUTTON_BAIC_KEY = "button_baic_key";
    // Block all incoming international roaming calls
    private static final String BUTTON_BAICr_KEY = "button_baicr_key";
    // Disable all barring
    private static final String BUTTON_BA_ALL_KEY = "button_ba_all_key";
    // Change passward
    private static final String BUTTON_BA_CHANGE_PW_KEY = "button_change_pw_key";

    private static final String PW_CHANGE_STATE_KEY = "pin_change_state_key";
    private static final String OLD_PW_KEY = "old_pw_key";
    private static final String NEW_PW_KEY = "new_pw_key";
    private static final String DIALOG_MESSAGE_KEY = "dialog_message_key";
    private static final String DIALOG_PW_ENTRY_KEY = "dialog_pw_enter_key";
    private static final String KEY_STATUS = "toggle";
    private static final String PREFERENCE_ENABLED_KEY = "PREFERENCE_ENABLED";
    private static final String PREFERENCE_SHOW_PASSWORD_KEY = "PREFERENCE_SHOW_PASSWORD";
    private static final String SAVED_BEFORE_LOAD_COMPLETED_KEY = "PROGRESS_SHOWING";

    private CallBarringEditPreference mButtonBAOC;
    private CallBarringEditPreference mButtonBAOIC;
    private CallBarringEditPreference mButtonBAOICxH;
    private CallBarringEditPreference mButtonBAIC;
    private CallBarringEditPreference mButtonBAICr;
    private CallBarringDeselectAllPreference mButtonDisableAll;
    private EditPinPreference mButtonChangePW;

    // State variables
    private int mPwChangeState;
    private String mOldPassword;
    private String mNewPassword;
    private int mPwChangeDialogStrId;

    private static final int PW_CHANGE_OLD = 0;
    private static final int PW_CHANGE_NEW = 1;
    private static final int PW_CHANGE_REENTER = 2;

    private static final int BUSY_READING_DIALOG = 100;
    private static final int BUSY_SAVING_DIALOG = 200;

    // Password change complete event
    private static final int EVENT_PW_CHANGE_COMPLETE = 100;
    // Disable all complete event
    private static final int EVENT_DISABLE_ALL_COMPLETE = 200;

    private static final int PW_LENGTH = 4;

    private Phone mPhone;
    private ArrayList<CallBarringEditPreference> mPreferences =
            new ArrayList<CallBarringEditPreference>();
    private int mInitIndex = 0;
    private boolean mFirstResume;
    private Bundle mIcicle;

    private SubscriptionInfoHelper mSubscriptionInfoHelper;
    private Dialog mProgressDialog;

    @Override
    public void onPinEntered(EditPinPreference preference, boolean positiveResult) {
        if (preference == mButtonChangePW) {
            updatePWChangeState(positiveResult);
        } else if (preference == mButtonDisableAll) {
            disableAllBarring(positiveResult);
        }
    }

    /**
     * Display a toast for message.
     */
    private void displayMessage(int strId) {
        Toast.makeText(this, getString(strId), Toast.LENGTH_SHORT).show();
    }

    /**
     * Attempt to disable all for call barring settings.
     */
    private void disableAllBarring(boolean positiveResult) {
        if (!positiveResult) {
            // Return on cancel
            return;
        }

        String password = null;
        if (mButtonDisableAll.isPasswordShown()) {
            password = mButtonDisableAll.getText();
            // Validate the length of password first, before submitting it to the
            // RIL for CB disable.
            if (!validatePassword(password)) {
                mButtonDisableAll.setText("");
                displayMessage(R.string.call_barring_right_pwd_number);
                return;
            }
        }

        // Submit the disable all request
        mButtonDisableAll.setText("");
        Message onComplete = mHandler.obtainMessage(EVENT_DISABLE_ALL_COMPLETE);
        mPhone.setCallBarring(CommandsInterface.CB_FACILITY_BA_ALL, false, password, onComplete, 0);
        this.onStarted(mButtonDisableAll, false);
    }

    /**
     * Attempt to change the password for call barring settings.
     */
    private void updatePWChangeState(boolean positiveResult) {
        if (!positiveResult) {
            // Reset the state on cancel
            resetPwChangeState();
            return;
        }

        // Progress through the dialog states, generally in this order:
        // 1. Enter old password
        // 2. Enter new password
        // 3. Re-Enter new password
        // In general, if any invalid entries are made, the dialog re-
        // appears with text to indicate what the issue is.
        switch (mPwChangeState) {
            case PW_CHANGE_OLD:
                mOldPassword = mButtonChangePW.getText();
                mButtonChangePW.setText("");
                if (validatePassword(mOldPassword)) {
                    mPwChangeState = PW_CHANGE_NEW;
                    displayPwChangeDialog();
                } else {
                    displayPwChangeDialog(R.string.call_barring_right_pwd_number, true);
                }
                break;
            case PW_CHANGE_NEW:
                mNewPassword = mButtonChangePW.getText();
                mButtonChangePW.setText("");
                if (validatePassword(mNewPassword)) {
                    mPwChangeState = PW_CHANGE_REENTER;
                    displayPwChangeDialog();
                } else {
                    displayPwChangeDialog(R.string.call_barring_right_pwd_number, true);
                }
                break;
            case PW_CHANGE_REENTER:
                // If the re-entered password is not valid, display a message
                // and reset the state.
                if (!mNewPassword.equals(mButtonChangePW.getText())) {
                    mPwChangeState = PW_CHANGE_NEW;
                    mButtonChangePW.setText("");
                    displayPwChangeDialog(R.string.call_barring_pwd_not_match, true);
                } else {
                    // If the password is valid, then submit the change password request
                    mButtonChangePW.setText("");
                    Message onComplete = mHandler.obtainMessage(EVENT_PW_CHANGE_COMPLETE);
                    ((GsmCdmaPhone) mPhone).changeCallBarringPassword(
                            CommandsInterface.CB_FACILITY_BA_ALL,
                            mOldPassword, mNewPassword, onComplete);
                    this.onStarted(mButtonChangePW, false);
                }
                break;
            default:
                if (DBG) {
                    Log.d(LOG_TAG, "updatePWChangeState: Unknown password change state: "
                            + mPwChangeState);
                }
                break;
        }
    }

    /**
     * Handler for asynchronous replies from the framework layer.
     */
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar = (AsyncResult) msg.obj;
            switch (msg.what) {
                // Handle the response message for password change from the framework layer.
                case EVENT_PW_CHANGE_COMPLETE: {
                    onFinished(mButtonChangePW, false);
                    // Unsuccessful change, display a toast to user with failure reason.
                    if (ar.exception != null) {
                        if (DBG) {
                            Log.d(LOG_TAG,
                                    "change password for call barring failed with exception: "
                                            + ar.exception);
                        }
                        onException(mButtonChangePW, (CommandException) ar.exception);
                        mButtonChangePW.setEnabled(true);
                    } else if (ar.userObj instanceof Throwable) {
                        onError(mButtonChangePW, RESPONSE_ERROR);
                    } else {
                        // Successful change.
                        displayMessage(R.string.call_barring_change_pwd_success);
                    }
                    resetPwChangeState();
                    break;
                }
                // When disabling all call barring, either fail and display a toast,
                // or just update the UI.
                case EVENT_DISABLE_ALL_COMPLETE: {
                    onFinished(mButtonDisableAll, false);
                    if (ar.exception != null) {
                        if (DBG) {
                            Log.d(LOG_TAG, "can not disable all call barring with exception: "
                                    + ar.exception);
                        }
                        onException(mButtonDisableAll, (CommandException) ar.exception);
                        mButtonDisableAll.setEnabled(true);
                    } else if (ar.userObj instanceof Throwable) {
                        onError(mButtonDisableAll, RESPONSE_ERROR);
                    } else {
                        // Reset to normal behaviour on successful change.
                        displayMessage(R.string.call_barring_deactivate_success);
                        resetCallBarringPrefState(false);
                    }
                    break;
                }
                default: {
                    if (DBG) {
                        Log.d(LOG_TAG, "Unknown message id: " + msg.what);
                    }
                    break;
                }
            }
        }
    };

    /**
     * The next two functions are for updating the message field on the dialog.
     */
    private void displayPwChangeDialog() {
        displayPwChangeDialog(0, true);
    }

    private void displayPwChangeDialog(int strId, boolean shouldDisplay) {
        int msgId = 0;
        switch (mPwChangeState) {
            case PW_CHANGE_OLD:
                msgId = R.string.call_barring_old_pwd;
                break;
            case PW_CHANGE_NEW:
                msgId = R.string.call_barring_new_pwd;
                break;
            case PW_CHANGE_REENTER:
                msgId = R.string.call_barring_confirm_pwd;
                break;
            default:
                break;
        }

        // Append the note/additional message, if needed.
        if (strId != 0) {
            mButtonChangePW.setDialogMessage(getText(msgId) + "\n" + getText(strId));
        } else {
            mButtonChangePW.setDialogMessage(msgId);
        }

        // Only display if requested.
        if (shouldDisplay) {
            mButtonChangePW.showPinDialog();
        }
        mPwChangeDialogStrId = strId;
    }

    /**
     * Reset the state of the password change dialog.
     */
    private void resetPwChangeState() {
        mPwChangeState = PW_CHANGE_OLD;
        displayPwChangeDialog(0, false);
        mOldPassword = "";
        mNewPassword = "";
    }

    /**
     * Reset the state of the all call barring setting to disable.
     */
    private void resetCallBarringPrefState(boolean enable) {
        for (CallBarringEditPreference pref : mPreferences) {
            pref.mIsActivated = enable;
            pref.updateSummaryText();
        }
    }

    /**
     * Validate the password entry.
     *
     * @param password This is the password to validate
     */
    private boolean validatePassword(String password) {
        return password != null && password.length() == PW_LENGTH;
    }

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        if (DBG) {
            Log.d(LOG_TAG, "onCreate, reading callbarring_options.xml file");
        }
        addPreferencesFromResource(R.xml.callbarring_options);

        mSubscriptionInfoHelper = new SubscriptionInfoHelper(this, getIntent());
        mPhone = mSubscriptionInfoHelper.getPhone();
        if (DBG) {
            Log.d(LOG_TAG, "onCreate, reading callbarring_options.xml file finished!");
        }

        // Get UI object references
        PreferenceScreen prefSet = getPreferenceScreen();
        mButtonBAOC = (CallBarringEditPreference) prefSet.findPreference(BUTTON_BAOC_KEY);
        mButtonBAOIC = (CallBarringEditPreference) prefSet.findPreference(BUTTON_BAOIC_KEY);
        mButtonBAOICxH = (CallBarringEditPreference) prefSet.findPreference(BUTTON_BAOICxH_KEY);
        mButtonBAIC = (CallBarringEditPreference) prefSet.findPreference(BUTTON_BAIC_KEY);
        mButtonBAICr = (CallBarringEditPreference) prefSet.findPreference(BUTTON_BAICr_KEY);
        mButtonDisableAll = (CallBarringDeselectAllPreference)
                prefSet.findPreference(BUTTON_BA_ALL_KEY);
        mButtonChangePW = (EditPinPreference) prefSet.findPreference(BUTTON_BA_CHANGE_PW_KEY);

        // Assign click listener and update state
        mButtonBAOC.setOnPinEnteredListener(this);
        mButtonBAOIC.setOnPinEnteredListener(this);
        mButtonBAOICxH.setOnPinEnteredListener(this);
        mButtonBAIC.setOnPinEnteredListener(this);
        mButtonBAICr.setOnPinEnteredListener(this);
        mButtonDisableAll.setOnPinEnteredListener(this);
        mButtonChangePW.setOnPinEnteredListener(this);

        // Store CallBarringEditPreferencence objects in array list.
        mPreferences.add(mButtonBAOC);
        mPreferences.add(mButtonBAOIC);
        mPreferences.add(mButtonBAOICxH);
        mPreferences.add(mButtonBAIC);
        mPreferences.add(mButtonBAICr);

        // Find out if password is currently used.
        boolean usePassword = true;
        boolean useDisableaAll = true;

        ImsPhone imsPhone = mPhone != null ? (ImsPhone) mPhone.getImsPhone() : null;
        if (imsPhone != null
                && ((imsPhone.getServiceState().getState() == ServiceState.STATE_IN_SERVICE)
                        || imsPhone.isUtEnabled())) {
            usePassword = false;
            useDisableaAll = false;
        }

        // Find out if the sim card is ready.
        boolean isSimReady = TelephonyManager.from(this).getSimState(
                SubscriptionManager.getSlotIndex(mPhone.getSubId()))
                        == TelephonyManager.SIM_STATE_READY;

        // Deactivate all option is unavailable when sim card is not ready or Ut is enabled.
        if (isSimReady && useDisableaAll) {
            mButtonDisableAll.setEnabled(true);
            mButtonDisableAll.init(mPhone);
        } else {
            mButtonDisableAll.setEnabled(false);
        }

        // Change password option is unavailable when sim card is not ready or when the password is
        // not used.
        if (isSimReady && usePassword) {
            mButtonChangePW.setEnabled(true);
        } else {
            mButtonChangePW.setEnabled(false);
            mButtonChangePW.setSummary(R.string.call_barring_change_pwd_description_disabled);
        }

        // Wait to do the initialization until onResume so that the TimeConsumingPreferenceActivity
        // dialog can display as it relies on onResume / onPause to maintain its foreground state.
        mFirstResume = true;
        mIcicle = icicle;

        ActionBar actionBar = getActionBar();
        if (actionBar != null) {
            // android.R.id.home will be triggered in onOptionsItemSelected()
            actionBar.setDisplayHomeAsUpEnabled(true);
        }

        if (mIcicle != null && !mIcicle.getBoolean(SAVED_BEFORE_LOAD_COMPLETED_KEY)) {
            if (DBG) {
                Log.d(LOG_TAG, "restore stored states");
            }
            mInitIndex = mPreferences.size();

            for (CallBarringEditPreference pref : mPreferences) {
                Bundle bundle = mIcicle.getParcelable(pref.getKey());
                if (bundle != null) {
                    pref.handleCallBarringResult(bundle.getBoolean(KEY_STATUS));
                    pref.init(this, true, mPhone);
                    pref.setEnabled(bundle.getBoolean(PREFERENCE_ENABLED_KEY, pref.isEnabled()));
                    pref.setInputMethodNeeded(bundle.getBoolean(PREFERENCE_SHOW_PASSWORD_KEY,
                            pref.needInputMethod()));
                }
            }
            mPwChangeState = mIcicle.getInt(PW_CHANGE_STATE_KEY);
            mOldPassword = mIcicle.getString(OLD_PW_KEY);
            mNewPassword = mIcicle.getString(NEW_PW_KEY);
            displayPwChangeDialog(mIcicle.getInt(DIALOG_MESSAGE_KEY, mPwChangeDialogStrId), false);
            mButtonChangePW.setText(mIcicle.getString(DIALOG_PW_ENTRY_KEY));
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mFirstResume) {
            if (mIcicle == null || mIcicle.getBoolean(SAVED_BEFORE_LOAD_COMPLETED_KEY)) {
                if (DBG) {
                    Log.d(LOG_TAG, "onResume: start to init ");
                }
                resetPwChangeState();
                mPreferences.get(mInitIndex).init(this, false, mPhone);

                // Request removing BUSY_SAVING_DIALOG because reading is restarted.
                // (If it doesn't exist, nothing happen.)
                removeDialog(BUSY_SAVING_DIALOG);
            }
            mFirstResume = false;
            mIcicle = null;
        }
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        for (CallBarringEditPreference pref : mPreferences) {
            Bundle bundle = new Bundle();
            bundle.putBoolean(KEY_STATUS, pref.mIsActivated);
            bundle.putBoolean(PREFERENCE_ENABLED_KEY, pref.isEnabled());
            bundle.putBoolean(PREFERENCE_SHOW_PASSWORD_KEY, pref.needInputMethod());
            outState.putParcelable(pref.getKey(), bundle);
        }
        outState.putInt(PW_CHANGE_STATE_KEY, mPwChangeState);
        outState.putString(OLD_PW_KEY, mOldPassword);
        outState.putString(NEW_PW_KEY, mNewPassword);
        outState.putInt(DIALOG_MESSAGE_KEY, mPwChangeDialogStrId);
        outState.putString(DIALOG_PW_ENTRY_KEY, mButtonChangePW.getText());

        outState.putBoolean(SAVED_BEFORE_LOAD_COMPLETED_KEY,
                mProgressDialog != null && mProgressDialog.isShowing());
    }

    /**
     * Finish initialization of this preference and start next.
     *
     * @param preference The preference.
     * @param reading If true to dismiss the busy reading dialog,
     *                false to dismiss the busy saving dialog.
     */
    public void onFinished(Preference preference, boolean reading) {
        if (mInitIndex < mPreferences.size() - 1 && !isFinishing()) {
            mInitIndex++;
            mPreferences.get(mInitIndex).init(this, false, mPhone);
        }
        super.onFinished(preference, reading);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final int itemId = item.getItemId();
        if (itemId == android.R.id.home) {
            CallFeaturesSetting.goUpToTopLevelSetting(this, mSubscriptionInfoHelper);
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onPrepareDialog(int id, Dialog dialog, Bundle args) {
        super.onPrepareDialog(id, dialog, args);
        if (id == BUSY_READING_DIALOG || id == BUSY_SAVING_DIALOG) {
            // For onSaveInstanceState, treat the SAVING dialog as the same as the READING. As
            // the result, if the activity is recreated while waiting for SAVING, it starts reading
            // all the newest data.
            mProgressDialog = dialog;
        }
    }
}
