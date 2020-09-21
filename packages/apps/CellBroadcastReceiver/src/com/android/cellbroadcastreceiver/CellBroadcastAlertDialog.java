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

package com.android.cellbroadcastreceiver;

import android.app.Activity;
import android.app.KeyguardManager;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.preference.PreferenceManager;
import android.provider.Telephony;
import android.telephony.CellBroadcastMessage;
import android.telephony.SmsCbCmasInfo;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Custom alert dialog with optional flashing warning icon.
 * Alert audio and text-to-speech handled by {@link CellBroadcastAlertAudio}.
 */
public class CellBroadcastAlertDialog extends Activity {

    private static final String TAG = "CellBroadcastAlertDialog";

    /** Intent extra for non-emergency alerts sent when user selects the notification. */
    static final String FROM_NOTIFICATION_EXTRA = "from_notification";

    // Intent extra to identify if notification was sent while trying to move away from the dialog
    //  without acknowledging the dialog
    static final String FROM_SAVE_STATE_NOTIFICATION_EXTRA = "from_save_state_notification";

    /** List of cell broadcast messages to display (oldest to newest). */
    protected ArrayList<CellBroadcastMessage> mMessageList;

    /** Whether a CMAS alert other than Presidential Alert was displayed. */
    private boolean mShowOptOutDialog;

    /** Length of time for the warning icon to be visible. */
    private static final int WARNING_ICON_ON_DURATION_MSEC = 800;

    /** Length of time for the warning icon to be off. */
    private static final int WARNING_ICON_OFF_DURATION_MSEC = 800;

    /** Length of time to keep the screen turned on. */
    private static final int KEEP_SCREEN_ON_DURATION_MSEC = 60000;

    /** Animation handler for the flashing warning icon (emergency alerts only). */
    private final AnimationHandler mAnimationHandler = new AnimationHandler();

    /** Handler to add and remove screen on flags for emergency alerts. */
    private final ScreenOffHandler mScreenOffHandler = new ScreenOffHandler();

    /**
     * Animation handler for the flashing warning icon (emergency alerts only).
     */
    private class AnimationHandler extends Handler {
        /** Latest {@code message.what} value for detecting old messages. */
        private final AtomicInteger mCount = new AtomicInteger();

        /** Warning icon state: visible == true, hidden == false. */
        private boolean mWarningIconVisible;

        /** The warning icon Drawable. */
        private Drawable mWarningIcon;

        /** The View containing the warning icon. */
        private ImageView mWarningIconView;

        /** Package local constructor (called from outer class). */
        AnimationHandler() {}

        /** Start the warning icon animation. */
        void startIconAnimation() {
            if (!initDrawableAndImageView()) {
                return;     // init failure
            }
            mWarningIconVisible = true;
            mWarningIconView.setVisibility(View.VISIBLE);
            updateIconState();
            queueAnimateMessage();
        }

        /** Stop the warning icon animation. */
        void stopIconAnimation() {
            // Increment the counter so the handler will ignore the next message.
            mCount.incrementAndGet();
            if (mWarningIconView != null) {
                mWarningIconView.setVisibility(View.GONE);
            }
        }

        /** Update the visibility of the warning icon. */
        private void updateIconState() {
            mWarningIconView.setImageAlpha(mWarningIconVisible ? 255 : 0);
            mWarningIconView.invalidateDrawable(mWarningIcon);
        }

        /** Queue a message to animate the warning icon. */
        private void queueAnimateMessage() {
            int msgWhat = mCount.incrementAndGet();
            sendEmptyMessageDelayed(msgWhat, mWarningIconVisible ? WARNING_ICON_ON_DURATION_MSEC
                    : WARNING_ICON_OFF_DURATION_MSEC);
        }

        @Override
        public void handleMessage(Message msg) {
            if (msg.what == mCount.get()) {
                mWarningIconVisible = !mWarningIconVisible;
                updateIconState();
                queueAnimateMessage();
            }
        }

        /**
         * Initialize the Drawable and ImageView fields.
         * @return true if successful; false if any field failed to initialize
         */
        private boolean initDrawableAndImageView() {
            if (mWarningIcon == null) {
                try {
                    mWarningIcon = getResources().getDrawable(R.drawable.ic_warning_googred);
                } catch (Resources.NotFoundException e) {
                    Log.e(TAG, "warning icon resource not found", e);
                    return false;
                }
            }
            if (mWarningIconView == null) {
                mWarningIconView = (ImageView) findViewById(R.id.icon);
                if (mWarningIconView != null) {
                    mWarningIconView.setImageDrawable(mWarningIcon);
                } else {
                    Log.e(TAG, "failed to get ImageView for warning icon");
                    return false;
                }
            }
            return true;
        }
    }

    /**
     * Handler to add {@code FLAG_KEEP_SCREEN_ON} for emergency alerts. After a short delay,
     * remove the flag so the screen can turn off to conserve the battery.
     */
    private class ScreenOffHandler extends Handler {
        /** Latest {@code message.what} value for detecting old messages. */
        private final AtomicInteger mCount = new AtomicInteger();

        /** Package local constructor (called from outer class). */
        ScreenOffHandler() {}

        /** Add screen on window flags and queue a delayed message to remove them later. */
        void startScreenOnTimer() {
            addWindowFlags();
            int msgWhat = mCount.incrementAndGet();
            removeMessages(msgWhat - 1);    // Remove previous message, if any.
            sendEmptyMessageDelayed(msgWhat, KEEP_SCREEN_ON_DURATION_MSEC);
            Log.d(TAG, "added FLAG_KEEP_SCREEN_ON, queued screen off message id " + msgWhat);
        }

        /** Remove the screen on window flags and any queued screen off message. */
        void stopScreenOnTimer() {
            removeMessages(mCount.get());
            clearWindowFlags();
        }

        /** Set the screen on window flags. */
        private void addWindowFlags() {
            getWindow().addFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
                    | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        /** Clear the screen on window flags. */
        private void clearWindowFlags() {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON
                    | WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        }

        @Override
        public void handleMessage(Message msg) {
            int msgWhat = msg.what;
            if (msgWhat == mCount.get()) {
                clearWindowFlags();
                Log.d(TAG, "removed FLAG_KEEP_SCREEN_ON with id " + msgWhat);
            } else {
                Log.e(TAG, "discarding screen off message with id " + msgWhat);
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final Window win = getWindow();

        // We use a custom title, so remove the standard dialog title bar
        win.requestFeature(Window.FEATURE_NO_TITLE);

        // Full screen alerts display above the keyguard and when device is locked.
        win.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN
                | WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED
                | WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD);

        setFinishOnTouchOutside(false);

        // Initialize the view.
        LayoutInflater inflater = LayoutInflater.from(this);
        setContentView(inflater.inflate(R.layout.cell_broadcast_alert, null));

        findViewById(R.id.dismissButton).setOnClickListener(
                new Button.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        dismiss();
                    }
                });

        // Get message list from saved Bundle or from Intent.
        if (savedInstanceState != null) {
            Log.d(TAG, "onCreate getting message list from saved instance state");
            mMessageList = savedInstanceState.getParcelableArrayList(
                    CellBroadcastMessage.SMS_CB_MESSAGE_EXTRA);
        } else {
            Log.d(TAG, "onCreate getting message list from intent");
            Intent intent = getIntent();
            mMessageList = intent.getParcelableArrayListExtra(
                    CellBroadcastMessage.SMS_CB_MESSAGE_EXTRA);

            // If we were started from a notification, dismiss it.
            clearNotification(intent);
        }

        if (mMessageList == null || mMessageList.size() == 0) {
            Log.e(TAG, "onCreate failed as message list is null or empty");
            finish();
        } else {
            Log.d(TAG, "onCreate loaded message list of size " + mMessageList.size());
        }

        // For emergency alerts, keep screen on so the user can read it
        CellBroadcastMessage message = getLatestMessage();
        if (message != null && CellBroadcastChannelManager.isEmergencyMessage(
                this, message)) {
            Log.d(TAG, "onCreate setting screen on timer for emergency alert");
            mScreenOffHandler.startScreenOnTimer();
        }

        updateAlertText(message);
    }

    /**
     * Start animating warning icon.
     */
    @Override
    protected void onResume() {
        super.onResume();
        CellBroadcastMessage message = getLatestMessage();
        if (message != null && CellBroadcastChannelManager.isEmergencyMessage(this, message)) {
            mAnimationHandler.startIconAnimation();
        }
    }

    /**
     * Stop animating warning icon.
     */
    @Override
    protected void onPause() {
        Log.d(TAG, "onPause called");
        mAnimationHandler.stopIconAnimation();
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
        // When the activity goes in background eg. clicking Home button, send notification.
        // Avoid doing this when activity will be recreated because of orientation change or if
        // screen goes off
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        if (!(isChangingConfigurations() || getLatestMessage() == null) && pm.isScreenOn()) {
            CellBroadcastAlertService.addToNotificationBar(getLatestMessage(), mMessageList,
                    getApplicationContext(), true);
        }
    }

    /** Returns the currently displayed message. */
    CellBroadcastMessage getLatestMessage() {
        int index = mMessageList.size() - 1;
        if (index >= 0) {
            return mMessageList.get(index);
        } else {
            Log.d(TAG, "getLatestMessage returns null");
            return null;
        }
    }

    /** Removes and returns the currently displayed message. */
    private CellBroadcastMessage removeLatestMessage() {
        int index = mMessageList.size() - 1;
        if (index >= 0) {
            return mMessageList.remove(index);
        } else {
            return null;
        }
    }

    /**
     * Save the list of messages so the state can be restored later.
     * @param outState Bundle in which to place the saved state.
     */
    @Override
    protected void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);
        outState.putParcelableArrayList(CellBroadcastMessage.SMS_CB_MESSAGE_EXTRA, mMessageList);
    }

    /**
     * Update alert text when a new emergency alert arrives.
     * @param message CB message which is used to update alert text.
     */
    private void updateAlertText(CellBroadcastMessage message) {
        int titleId = CellBroadcastResources.getDialogTitleResource(
                getApplicationContext(), message);

        String title = getText(titleId).toString();
        TextView titleTextView = findViewById(R.id.alertTitle);

        if (getApplicationContext().getResources().getBoolean(R.bool.show_date_time_title)) {
            titleTextView.setSingleLine(false);
            title += "\n" + message.getDateString(getApplicationContext());
        }

        setTitle(title);
        titleTextView.setText(title);

        ((TextView) findViewById(R.id.message)).setText(message.getMessageBody());

        String dismissButtonText = getString(R.string.button_dismiss);

        if (mMessageList.size() > 1) {
            dismissButtonText += "  (1/" + mMessageList.size() + ")";
        }

        ((TextView) findViewById(R.id.dismissButton)).setText(dismissButtonText);
    }

    /**
     * Called by {@link CellBroadcastAlertService} to add a new alert to the stack.
     * @param intent The new intent containing one or more {@link CellBroadcastMessage}s.
     */
    @Override
    protected void onNewIntent(Intent intent) {
        ArrayList<CellBroadcastMessage> newMessageList = intent.getParcelableArrayListExtra(
                CellBroadcastMessage.SMS_CB_MESSAGE_EXTRA);
        if (newMessageList != null) {
            if (intent.getBooleanExtra(FROM_SAVE_STATE_NOTIFICATION_EXTRA, false)) {
                mMessageList = newMessageList;
            } else {
                mMessageList.addAll(newMessageList);
            }
            Log.d(TAG, "onNewIntent called with message list of size " + newMessageList.size());
            updateAlertText(getLatestMessage());
            // If the new intent was sent from a notification, dismiss it.
            clearNotification(intent);
        } else {
            Log.e(TAG, "onNewIntent called without SMS_CB_MESSAGE_EXTRA, ignoring");
        }
    }

    /**
     * Try to cancel any notification that may have started this activity.
     * @param intent Intent containing extras used to identify if notification needs to be cleared
     */
    private void clearNotification(Intent intent) {
        if (intent.getBooleanExtra(FROM_NOTIFICATION_EXTRA, false)) {
            NotificationManager notificationManager =
                    (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.cancel(CellBroadcastAlertService.NOTIFICATION_ID);
            CellBroadcastReceiverApp.clearNewMessageList();
        }
    }

    /**
     * Stop animating warning icon and stop the {@link CellBroadcastAlertAudio}
     * service if necessary.
     */
    void dismiss() {
        Log.d(TAG, "dismiss");
        // Stop playing alert sound/vibration/speech (if started)
        stopService(new Intent(this, CellBroadcastAlertAudio.class));

        // Cancel any pending alert reminder
        CellBroadcastAlertReminder.cancelAlertReminder();

        // Remove the current alert message from the list.
        CellBroadcastMessage lastMessage = removeLatestMessage();
        if (lastMessage == null) {
            Log.e(TAG, "dismiss() called with empty message list!");
            finish();
            return;
        }

        // Mark the alert as read.
        final long deliveryTime = lastMessage.getDeliveryTime();

        // Mark broadcast as read on a background thread.
        new CellBroadcastContentProvider.AsyncCellBroadcastTask(getContentResolver())
                .execute(new CellBroadcastContentProvider.CellBroadcastOperation() {
                    @Override
                    public boolean execute(CellBroadcastContentProvider provider) {
                        return provider.markBroadcastRead(
                                Telephony.CellBroadcasts.DELIVERY_TIME, deliveryTime);
                    }
                });

        // Set the opt-out dialog flag if this is a CMAS alert (other than Presidential Alert).
        if (lastMessage.isCmasMessage() && lastMessage.getCmasMessageClass() !=
                SmsCbCmasInfo.CMAS_CLASS_PRESIDENTIAL_LEVEL_ALERT) {
            mShowOptOutDialog = true;
        }

        // If there are older emergency alerts to display, update the alert text and return.
        CellBroadcastMessage nextMessage = getLatestMessage();
        if (nextMessage != null) {
            updateAlertText(nextMessage);
            if (CellBroadcastChannelManager.isEmergencyMessage(
                    this, nextMessage)) {
                mAnimationHandler.startIconAnimation();
            } else {
                mAnimationHandler.stopIconAnimation();
            }
            return;
        }

        // Remove pending screen-off messages (animation messages are removed in onPause()).
        mScreenOffHandler.stopScreenOnTimer();

        // Show opt-in/opt-out dialog when the first CMAS alert is received.
        if (mShowOptOutDialog) {
            SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
            if (prefs.getBoolean(CellBroadcastSettings.KEY_SHOW_CMAS_OPT_OUT_DIALOG, true)) {
                // Clear the flag so the user will only see the opt-out dialog once.
                prefs.edit().putBoolean(CellBroadcastSettings.KEY_SHOW_CMAS_OPT_OUT_DIALOG, false)
                        .apply();

                KeyguardManager km = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
                if (km.inKeyguardRestrictedInputMode()) {
                    Log.d(TAG, "Showing opt-out dialog in new activity (secure keyguard)");
                    Intent intent = new Intent(this, CellBroadcastOptOutActivity.class);
                    startActivity(intent);
                } else {
                    Log.d(TAG, "Showing opt-out dialog in current activity");
                    CellBroadcastOptOutActivity.showOptOutDialog(this);
                    return; // don't call finish() until user dismisses the dialog
                }
            }
        }
        NotificationManager notificationManager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(CellBroadcastAlertService.NOTIFICATION_ID);
        finish();
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        CellBroadcastMessage message = getLatestMessage();
        if (message != null && !message.isEtwsMessage()) {
            switch (event.getKeyCode()) {
                // Volume keys and camera keys mute the alert sound/vibration (except ETWS).
                case KeyEvent.KEYCODE_VOLUME_UP:
                case KeyEvent.KEYCODE_VOLUME_DOWN:
                case KeyEvent.KEYCODE_VOLUME_MUTE:
                case KeyEvent.KEYCODE_CAMERA:
                case KeyEvent.KEYCODE_FOCUS:
                    // Stop playing alert sound/vibration/speech (if started)
                    stopService(new Intent(this, CellBroadcastAlertAudio.class));
                    return true;

                default:
                    break;
            }
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    public void onBackPressed() {
        // Disable back key
    }
}
