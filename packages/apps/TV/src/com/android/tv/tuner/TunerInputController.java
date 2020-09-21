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

package com.android.tv.tuner;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Log;
import android.widget.Toast;
import com.android.tv.R;
import com.android.tv.Starter;
import com.android.tv.TvApplication;
import com.android.tv.TvSingletons;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.util.SystemPropertiesProxy;


import com.android.tv.tuner.setup.BaseTunerSetupActivity;
import com.android.tv.tuner.util.TunerInputInfoUtils;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;

/**
 * Controls the package visibility of {@link BaseTunerTvInputService}.
 *
 * <p>Listens to broadcast intent for {@link Intent#ACTION_BOOT_COMPLETED}, {@code
 * UsbManager.ACTION_USB_DEVICE_ATTACHED}, and {@code UsbManager.ACTION_USB_DEVICE_ATTACHED} to
 * update the connection status of the supported USB TV tuners.
 */
public class TunerInputController {
    private static final boolean DEBUG = false;
    private static final String TAG = "TunerInputController";
    private static final String PREFERENCE_IS_NETWORK_TUNER_ATTACHED = "network_tuner";
    private static final String SECURITY_PATCH_LEVEL_KEY = "ro.build.version.security_patch";
    private static final String SECURITY_PATCH_LEVEL_FORMAT = "yyyy-MM-dd";
    private static final String PLAY_STORE_LINK_TEMPLATE = "market://details?id=%s";

    /** Action of {@link Intent} to check network connection repeatedly when it is necessary. */
    private static final String CHECKING_NETWORK_TUNER_STATUS =
            "com.android.tv.action.CHECKING_NETWORK_TUNER_STATUS";

    private static final String EXTRA_CHECKING_DURATION =
            "com.android.tv.action.extra.CHECKING_DURATION";
    private static final String EXTRA_DEVICE_IP = "com.android.tv.action.extra.DEVICE_IP";

    private static final long INITIAL_CHECKING_DURATION_MS = TimeUnit.SECONDS.toMillis(10);
    private static final long MAXIMUM_CHECKING_DURATION_MS = TimeUnit.MINUTES.toMillis(10);
    private static final String NOTIFICATION_CHANNEL_ID = "tuner_discovery_notification";

    // TODO: Load settings from XML file
    private static final TunerDevice[] TUNER_DEVICES = {
        new TunerDevice(0x2040, 0xb123, null), // WinTV-HVR-955Q
        new TunerDevice(0x07ca, 0x0837, null), // AverTV Volar Hybrid Q
        // WinTV-dualHD (bulk) will be supported after 2017 April security patch.
        new TunerDevice(0x2040, 0x826d, "2017-04-01"), // WinTV-dualHD (bulk)
        new TunerDevice(0x2040, 0x0264, null),
    };

    private static final int MSG_ENABLE_INPUT_SERVICE = 1000;
    private static final long DVB_DRIVER_CHECK_DELAY_MS = 300;

    private final ComponentName usbTunerComponent;
    private final ComponentName networkTunerComponent;
    private final ComponentName builtInTunerComponent;
    private final Map<TunerDevice, ComponentName> mTunerServiceMapping = new HashMap<>();

    private final Map<ComponentName, String> mTunerApplicationNames = new HashMap<>();
    private final Map<ComponentName, String> mNotificationMessages = new HashMap<>();
    private final Map<ComponentName, Bitmap> mNotificationLargeIcons = new HashMap<>();

    private final CheckDvbDeviceHandler mHandler = new CheckDvbDeviceHandler(this);

    public TunerInputController(ComponentName embeddedTuner) {
        usbTunerComponent = embeddedTuner;
        networkTunerComponent = usbTunerComponent;
        builtInTunerComponent = usbTunerComponent;
        for (TunerDevice device : TUNER_DEVICES) {
            mTunerServiceMapping.put(device, usbTunerComponent);
        }
    }

    /** Checks status of USB devices to see if there are available USB tuners connected. */
    public void onCheckingUsbTunerStatus(Context context, String action) {
        onCheckingUsbTunerStatus(context, action, mHandler);
    }

    private void onCheckingUsbTunerStatus(
            Context context, String action, @NonNull CheckDvbDeviceHandler handler) {
        Set<TunerDevice> connectedUsbTuners = getConnectedUsbTuners(context);
        handler.removeMessages(MSG_ENABLE_INPUT_SERVICE);
        if (!connectedUsbTuners.isEmpty()) {
            // Need to check if DVB driver is accessible. Since the driver creation
            // could be happen after the USB event, delay the checking by
            // DVB_DRIVER_CHECK_DELAY_MS.
            handler.sendMessageDelayed(
                    handler.obtainMessage(MSG_ENABLE_INPUT_SERVICE, context),
                    DVB_DRIVER_CHECK_DELAY_MS);
        } else {
            handleTunerStatusChanged(
                    context,
                    false,
                    connectedUsbTuners,
                    TextUtils.equals(action, UsbManager.ACTION_USB_DEVICE_DETACHED)
                            ? TunerHal.TUNER_TYPE_USB
                            : null);
        }
    }

    private void onNetworkTunerChanged(Context context, boolean enabled) {
        SharedPreferences sharedPreferences =
                PreferenceManager.getDefaultSharedPreferences(context);
        if (sharedPreferences.contains(PREFERENCE_IS_NETWORK_TUNER_ATTACHED)
                && sharedPreferences.getBoolean(PREFERENCE_IS_NETWORK_TUNER_ATTACHED, false)
                        == enabled) {
            // the status is not changed
            return;
        }
        if (enabled) {
            sharedPreferences.edit().putBoolean(PREFERENCE_IS_NETWORK_TUNER_ATTACHED, true).apply();
        } else {
            sharedPreferences
                    .edit()
                    .putBoolean(PREFERENCE_IS_NETWORK_TUNER_ATTACHED, false)
                    .apply();
        }
        // Network tuner detection is initiated by UI. So the app should not
        // be killed.
        handleTunerStatusChanged(
                context, true, getConnectedUsbTuners(context), TunerHal.TUNER_TYPE_NETWORK);
    }

    /**
     * See if any USB tuner hardware is attached in the system.
     *
     * @param context {@link Context} instance
     * @return {@code true} if any tuner device we support is plugged in
     */
    private Set<TunerDevice> getConnectedUsbTuners(Context context) {
        UsbManager manager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        Map<String, UsbDevice> deviceList = manager.getDeviceList();
        String currentSecurityLevel =
                SystemPropertiesProxy.getString(SECURITY_PATCH_LEVEL_KEY, null);

        Set<TunerDevice> devices = new HashSet<>();
        for (UsbDevice device : deviceList.values()) {
            if (DEBUG) {
                Log.d(TAG, "Device: " + device);
            }
            for (TunerDevice tuner : TUNER_DEVICES) {
                if (tuner.equalsTo(device) && tuner.isSupported(currentSecurityLevel)) {
                    Log.i(TAG, "Tuner found");
                    devices.add(tuner);
                }
            }
        }
        return devices;
    }

    private void handleTunerStatusChanged(
            Context context,
            boolean forceDontKillApp,
            Set<TunerDevice> connectedUsbTuners,
            Integer triggerType) {
        Map<ComponentName, Integer> serviceToEnable = new HashMap<>();
        Set<ComponentName> serviceToDisable = new HashSet<>();
        serviceToDisable.add(builtInTunerComponent);
        serviceToDisable.add(networkTunerComponent);
        if (TunerFeatures.TUNER.isEnabled(context)) {
            // TODO: support both built-in tuner and other tuners at the same time?
            if (TunerHal.useBuiltInTuner(context)) {
                enableTunerTvInputService(
                        context, true, false, TunerHal.TUNER_TYPE_BUILT_IN, builtInTunerComponent);
                return;
            }
            SharedPreferences sharedPreferences =
                    PreferenceManager.getDefaultSharedPreferences(context);
            if (sharedPreferences.getBoolean(PREFERENCE_IS_NETWORK_TUNER_ATTACHED, false)) {
                serviceToEnable.put(networkTunerComponent, TunerHal.TUNER_TYPE_NETWORK);
            }
        }
        for (TunerDevice device : TUNER_DEVICES) {
            if (TunerFeatures.TUNER.isEnabled(context) && connectedUsbTuners.contains(device)) {
                serviceToEnable.put(mTunerServiceMapping.get(device), TunerHal.TUNER_TYPE_USB);
            } else {
                serviceToDisable.add(mTunerServiceMapping.get(device));
            }
        }
        serviceToDisable.removeAll(serviceToEnable.keySet());
        for (ComponentName serviceComponent : serviceToEnable.keySet()) {
            if (isTunerPackageInstalled(context, serviceComponent)) {
                enableTunerTvInputService(
                        context,
                        true,
                        forceDontKillApp,
                        serviceToEnable.get(serviceComponent),
                        serviceComponent);
            } else {
                sendNotificationToInstallPackage(context, serviceComponent);
            }
        }
        for (ComponentName serviceComponent : serviceToDisable) {
            if (isTunerPackageInstalled(context, serviceComponent)) {
                enableTunerTvInputService(
                        context, false, forceDontKillApp, triggerType, serviceComponent);
            } else {
                cancelNotificationToInstallPackage(context, serviceComponent);
            }
        }
    }

    /**
     * Enable/disable the component {@link BaseTunerTvInputService}.
     *
     * @param context {@link Context} instance
     * @param enabled {@code true} to enable the service; otherwise {@code false}
     */
    private static void enableTunerTvInputService(
            Context context,
            boolean enabled,
            boolean forceDontKillApp,
            Integer tunerType,
            ComponentName serviceComponent) {
        if (DEBUG) Log.d(TAG, "enableTunerTvInputService: " + enabled);
        PackageManager pm = context.getPackageManager();
        int newState =
                enabled
                        ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                        : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        if (newState != pm.getComponentEnabledSetting(serviceComponent)) {
            int flags = forceDontKillApp ? PackageManager.DONT_KILL_APP : 0;
            if (serviceComponent.getPackageName().equals(context.getPackageName())) {
                // Don't kill APP when handling input count changing. Or the following
                // setComponentEnabledSetting() call won't work.
                ((TvApplication) context.getApplicationContext())
                        .handleInputCountChanged(true, enabled, true);
                // Bundled input. Don't kill app if LiveChannels app is active since we don't want
                // to kill the running app.
                if (TvSingletons.getSingletons(context).getMainActivityWrapper().isCreated()) {
                    flags |= PackageManager.DONT_KILL_APP;
                }
                // Send/cancel the USB tuner TV input setup notification.
                BaseTunerSetupActivity.onTvInputEnabled(context, enabled, tunerType);
                if (!enabled && tunerType != null) {
                    if (tunerType == TunerHal.TUNER_TYPE_USB) {
                        Toast.makeText(
                                        context,
                                        R.string.msg_usb_tuner_disconnected,
                                        Toast.LENGTH_SHORT)
                                .show();
                    } else if (tunerType == TunerHal.TUNER_TYPE_NETWORK) {
                        Toast.makeText(
                                        context,
                                        R.string.msg_network_tuner_disconnected,
                                        Toast.LENGTH_SHORT)
                                .show();
                    }
                }
            }
            // Enable/disable the USB tuner TV input.
            pm.setComponentEnabledSetting(serviceComponent, newState, flags);
            if (DEBUG) Log.d(TAG, "Status updated:" + enabled);
        } else if (enabled && serviceComponent.getPackageName().equals(context.getPackageName())) {
            // When # of tuners is changed or the tuner input service is switching from/to using
            // network tuners or the device just boots.
            TunerInputInfoUtils.updateTunerInputInfo(context);
        }
    }

    /**
     * Discovers a network tuner. If the network connection is down, it won't repeatedly checking.
     */
    public void executeNetworkTunerDiscoveryAsyncTask(final Context context) {
        executeNetworkTunerDiscoveryAsyncTask(context, 0, 0);
    }

    /**
     * Discovers a network tuner.
     *
     * @param context {@link Context}
     * @param repeatedDurationMs The time length to wait to repeatedly check network status to start
     *     finding network tuner when the network connection is not available. {@code 0} to disable
     *     repeatedly checking.
     * @param deviceIp The previous discovered device IP, 0 if none.
     */
    private void executeNetworkTunerDiscoveryAsyncTask(
            final Context context, final long repeatedDurationMs, final int deviceIp) {
        if (!TunerFeatures.NETWORK_TUNER.isEnabled(context)) {
            return;
        }
        final Intent networkCheckingIntent = new Intent(context, IntentReceiver.class);
        networkCheckingIntent.setAction(CHECKING_NETWORK_TUNER_STATUS);
        if (!isNetworkConnected(context) && repeatedDurationMs > 0) {
            sendCheckingAlarm(context, networkCheckingIntent, repeatedDurationMs);
        } else {
            new AsyncTask<Void, Void, Boolean>() {
                @Override
                protected Boolean doInBackground(Void... params) {
                    Boolean result = null;
                    // Implement and execute network tuner discovery AsyncTask here.
                    return result;
                }

                @Override
                protected void onPostExecute(Boolean foundNetworkTuner) {
                    if (foundNetworkTuner == null) {
                        return;
                    }
                    sendCheckingAlarm(
                            context,
                            networkCheckingIntent,
                            foundNetworkTuner ? INITIAL_CHECKING_DURATION_MS : repeatedDurationMs);
                    onNetworkTunerChanged(context, foundNetworkTuner);
                }
            }.execute();
        }
    }

    private static boolean isNetworkConnected(Context context) {
        ConnectivityManager cm =
                (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo networkInfo = cm.getActiveNetworkInfo();
        return networkInfo != null && networkInfo.isConnected();
    }

    private static void sendCheckingAlarm(Context context, Intent intent, long delayMs) {
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        intent.putExtra(EXTRA_CHECKING_DURATION, delayMs);
        PendingIntent alarmIntent =
                PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        alarmManager.set(
                AlarmManager.ELAPSED_REALTIME,
                SystemClock.elapsedRealtime() + delayMs,
                alarmIntent);
    }

    private static boolean isTunerPackageInstalled(
            Context context, ComponentName serviceComponent) {
        try {
            context.getPackageManager().getPackageInfo(serviceComponent.getPackageName(), 0);
            return true;
        } catch (NameNotFoundException e) {
            return false;
        }
    }

    private void sendNotificationToInstallPackage(Context context, ComponentName serviceComponent) {
        if (!BuildConfig.ENG) {
            return;
        }
        String applicationName = mTunerApplicationNames.get(serviceComponent);
        if (applicationName == null) {
            applicationName = context.getString(R.string.tuner_install_default_application_name);
        }
        String contentTitle =
                context.getString(
                        R.string.tuner_install_notification_content_title, applicationName);
        String contentText = mNotificationMessages.get(serviceComponent);
        if (contentText == null) {
            contentText = context.getString(R.string.tuner_install_notification_content_text);
        }
        Bitmap largeIcon = mNotificationLargeIcons.get(serviceComponent);
        if (largeIcon == null) {
            // TODO: Make a better default image.
            largeIcon = BitmapFactory.decodeResource(context.getResources(), R.drawable.ic_store);
        }
        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        if (notificationManager.getNotificationChannel(NOTIFICATION_CHANNEL_ID) == null) {
            createNotificationChannel(context, notificationManager);
        }
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setData(
                Uri.parse(
                        String.format(
                                PLAY_STORE_LINK_TEMPLATE, serviceComponent.getPackageName())));
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        Notification.Builder builder = new Notification.Builder(context, NOTIFICATION_CHANNEL_ID);
        builder.setAutoCancel(true)
                .setSmallIcon(R.drawable.ic_launcher_s)
                .setLargeIcon(largeIcon)
                .setContentTitle(contentTitle)
                .setContentText(contentText)
                .setCategory(Notification.CATEGORY_RECOMMENDATION)
                .setContentIntent(PendingIntent.getActivity(context, 0, intent, 0));
        notificationManager.notify(serviceComponent.getPackageName(), 0, builder.build());
    }

    private static void cancelNotificationToInstallPackage(
            Context context, ComponentName serviceComponent) {
        NotificationManager notificationManager =
                (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(serviceComponent.getPackageName(), 0);
    }

    private static void createNotificationChannel(
            Context context, NotificationManager notificationManager) {
        notificationManager.createNotificationChannel(
                new NotificationChannel(
                        NOTIFICATION_CHANNEL_ID,
                        context.getResources()
                                .getString(R.string.ut_setup_notification_channel_name),
                        NotificationManager.IMPORTANCE_HIGH));
    }

    public static class IntentReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (DEBUG) Log.d(TAG, "Broadcast intent received:" + intent);
            Starter.start(context);
            TunerInputController tunerInputController =
                    TvSingletons.getSingletons(context).getTunerInputController();
            if (!TunerFeatures.TUNER.isEnabled(context)) {
                tunerInputController.handleTunerStatusChanged(
                        context, false, Collections.emptySet(), null);
                return;
            }
            switch (intent.getAction()) {
                case Intent.ACTION_BOOT_COMPLETED:
                    tunerInputController.executeNetworkTunerDiscoveryAsyncTask(
                            context, INITIAL_CHECKING_DURATION_MS, 0);
                    // fall through
                case TvApplication.ACTION_APPLICATION_FIRST_LAUNCHED:
                case UsbManager.ACTION_USB_DEVICE_ATTACHED:
                case UsbManager.ACTION_USB_DEVICE_DETACHED:
                    tunerInputController.onCheckingUsbTunerStatus(context, intent.getAction());
                    break;
                case CHECKING_NETWORK_TUNER_STATUS:
                    long repeatedDurationMs =
                            intent.getLongExtra(
                                    EXTRA_CHECKING_DURATION, INITIAL_CHECKING_DURATION_MS);
                    tunerInputController.executeNetworkTunerDiscoveryAsyncTask(
                            context,
                            Math.min(repeatedDurationMs * 2, MAXIMUM_CHECKING_DURATION_MS),
                            intent.getIntExtra(EXTRA_DEVICE_IP, 0));
                    break;
                default: // fall out
            }
        }
    }

    /**
     * Simple data holder for a USB device. Used to represent a tuner model, and compare against
     * {@link UsbDevice}.
     */
    private static class TunerDevice {
        private final int vendorId;
        private final int productId;

        // security patch level from which the specific tuner type is supported.
        private final String minSecurityLevel;

        private TunerDevice(int vendorId, int productId, String minSecurityLevel) {
            this.vendorId = vendorId;
            this.productId = productId;
            this.minSecurityLevel = minSecurityLevel;
        }

        private boolean equalsTo(UsbDevice device) {
            return device.getVendorId() == vendorId && device.getProductId() == productId;
        }

        private boolean isSupported(String currentSecurityLevel) {
            if (minSecurityLevel == null) {
                return true;
            }

            long supportSecurityLevelTimeStamp = 0;
            long currentSecurityLevelTimestamp = 0;
            try {
                SimpleDateFormat format = new SimpleDateFormat(SECURITY_PATCH_LEVEL_FORMAT);
                supportSecurityLevelTimeStamp = format.parse(minSecurityLevel).getTime();
                currentSecurityLevelTimestamp = format.parse(currentSecurityLevel).getTime();
            } catch (ParseException e) {
            }
            return supportSecurityLevelTimeStamp != 0
                    && supportSecurityLevelTimeStamp <= currentSecurityLevelTimestamp;
        }
    }

    private static class CheckDvbDeviceHandler extends Handler {

        private final TunerInputController mTunerInputController;
        private DvbDeviceAccessor mDvbDeviceAccessor;

        CheckDvbDeviceHandler(TunerInputController tunerInputController) {
            super(Looper.getMainLooper());
            this.mTunerInputController = tunerInputController;
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_ENABLE_INPUT_SERVICE:
                    Context context = (Context) msg.obj;
                    if (mDvbDeviceAccessor == null) {
                        mDvbDeviceAccessor = new DvbDeviceAccessor(context);
                    }
                    boolean enabled = mDvbDeviceAccessor.isDvbDeviceAvailable();
                    mTunerInputController.handleTunerStatusChanged(
                            context,
                            false,
                            enabled
                                    ? mTunerInputController.getConnectedUsbTuners(context)
                                    : Collections.emptySet(),
                            TunerHal.TUNER_TYPE_USB);
                    break;
                default: // fall out
            }
        }
    }
}
