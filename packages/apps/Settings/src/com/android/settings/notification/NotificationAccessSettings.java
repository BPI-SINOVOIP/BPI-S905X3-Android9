/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.settings.notification;

import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Fragment;
import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.Settings;
import android.service.notification.NotificationListenerService;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.settings.R;
import com.android.settings.core.instrumentation.InstrumentedDialogFragment;
import com.android.settings.overlay.FeatureFactory;
import com.android.settings.utils.ManagedServiceSettings;

/**
 * Settings screen for managing notification listener permissions
 */
public class NotificationAccessSettings extends ManagedServiceSettings {
    private static final String TAG = NotificationAccessSettings.class.getSimpleName();
    private static final Config CONFIG =  new Config.Builder()
            .setTag(TAG)
            .setSetting(Settings.Secure.ENABLED_NOTIFICATION_LISTENERS)
            .setIntentAction(NotificationListenerService.SERVICE_INTERFACE)
            .setPermission(android.Manifest.permission.BIND_NOTIFICATION_LISTENER_SERVICE)
            .setNoun("notification listener")
            .setWarningDialogTitle(R.string.notification_listener_security_warning_title)
            .setWarningDialogSummary(R.string.notification_listener_security_warning_summary)
            .setEmptyText(R.string.no_notification_listeners)
            .build();

    private NotificationManager mNm;

    @Override
    public int getMetricsCategory() {
        return MetricsEvent.NOTIFICATION_ACCESS;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mNm = context.getSystemService(NotificationManager.class);
    }

    @Override
    protected Config getConfig() {
        return CONFIG;
    }

    @Override
    protected boolean setEnabled(ComponentName service, String title, boolean enable) {
        logSpecialPermissionChange(enable, service.getPackageName());
        if (!enable) {
            if (!isServiceEnabled(service)) {
                return true; // already disabled
            }
            // show a friendly dialog
            new FriendlyWarningDialogFragment()
                    .setServiceInfo(service, title, this)
                    .show(getFragmentManager(), "friendlydialog");
            return false;
        } else {
            if (isServiceEnabled(service)) {
                return true; // already enabled
            }
            // show a scary dialog
            new ScaryWarningDialogFragment()
                    .setServiceInfo(service, title, this)
                    .show(getFragmentManager(), "dialog");
            return false;
        }
    }

    @Override
    protected boolean isServiceEnabled(ComponentName cn) {
        return mNm.isNotificationListenerAccessGranted(cn);
    }

    @Override
    protected void enable(ComponentName service) {
        mNm.setNotificationListenerAccessGranted(service, true);
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.notification_access_settings;
    }

    @VisibleForTesting
    void logSpecialPermissionChange(boolean enable, String packageName) {
        int logCategory = enable ? MetricsEvent.APP_SPECIAL_PERMISSION_NOTIVIEW_ALLOW
                : MetricsEvent.APP_SPECIAL_PERMISSION_NOTIVIEW_DENY;
        FeatureFactory.getFactory(getContext()).getMetricsFeatureProvider().action(getContext(),
                logCategory, packageName);
    }

    private static void disable(final NotificationAccessSettings parent, final ComponentName cn) {
        parent.mNm.setNotificationListenerAccessGranted(cn, false);
        AsyncTask.execute(() -> {
            if (!parent.mNm.isNotificationPolicyAccessGrantedForPackage(
                    cn.getPackageName())) {
                parent.mNm.removeAutomaticZenRules(cn.getPackageName());
            }
        });
    }

    public static class FriendlyWarningDialogFragment extends InstrumentedDialogFragment {
        static final String KEY_COMPONENT = "c";
        static final String KEY_LABEL = "l";

        public FriendlyWarningDialogFragment setServiceInfo(ComponentName cn, String label,
                Fragment target) {
            Bundle args = new Bundle();
            args.putString(KEY_COMPONENT, cn.flattenToString());
            args.putString(KEY_LABEL, label);
            setArguments(args);
            setTargetFragment(target, 0);
            return this;
        }

        @Override
        public int getMetricsCategory() {
            return MetricsEvent.DIALOG_DISABLE_NOTIFICATION_ACCESS;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            final Bundle args = getArguments();
            final String label = args.getString(KEY_LABEL);
            final ComponentName cn = ComponentName.unflattenFromString(args
                    .getString(KEY_COMPONENT));
            NotificationAccessSettings parent = (NotificationAccessSettings) getTargetFragment();

            final String summary = getResources().getString(
                    R.string.notification_listener_disable_warning_summary, label);
            return new AlertDialog.Builder(getContext())
                    .setMessage(summary)
                    .setCancelable(true)
                    .setPositiveButton(R.string.notification_listener_disable_warning_confirm,
                            (dialog, id) -> disable(parent, cn))
                    .setNegativeButton(R.string.notification_listener_disable_warning_cancel,
                            (dialog, id) -> {
                                // pass
                            })
                    .create();
        }
    }
}
