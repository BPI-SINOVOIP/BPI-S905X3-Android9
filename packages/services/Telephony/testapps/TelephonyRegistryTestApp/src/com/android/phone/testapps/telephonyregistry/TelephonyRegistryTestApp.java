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
 * limitations under the License
 */

package com.android.phone.testapps.telephonyregistry;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Bundle;
import android.provider.Telephony;
import android.telephony.CellInfo;
import android.telephony.CellLocation;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.util.SparseArray;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.Toast;

import java.util.List;
import java.util.stream.Collectors;

public class TelephonyRegistryTestApp extends Activity {
    private TelephonyManager telephonyManager;
    private NotificationManager notificationManager;
    private int mSelectedEvents = 0;
    private static final String NOTIFICATION_CHANNEL = "registryUpdate";

    private static final SparseArray<String> EVENTS = new SparseArray<String>() {{
        put(PhoneStateListener.LISTEN_SERVICE_STATE, "SERVICE_STATE");
        put(PhoneStateListener.LISTEN_MESSAGE_WAITING_INDICATOR, "MESSAGE_WAITING_INDICATOR");
        put(PhoneStateListener.LISTEN_CALL_FORWARDING_INDICATOR, "CALL_FORWARDING_INDICATOR");
        put(PhoneStateListener.LISTEN_CELL_LOCATION, "CELL_LOCATION");
        put(PhoneStateListener.LISTEN_CALL_STATE, "CALL_STATE");
        put(PhoneStateListener.LISTEN_DATA_CONNECTION_STATE, "DATA_CONNECTION_STATE");
        put(PhoneStateListener.LISTEN_DATA_ACTIVITY, "DATA_ACTIVITY");
        put(PhoneStateListener.LISTEN_SIGNAL_STRENGTHS, "SIGNAL_STRENGTHS");
        put(PhoneStateListener.LISTEN_OTASP_CHANGED, "OTASP_CHANGED");
        put(PhoneStateListener.LISTEN_CELL_INFO, "CELL_INFO");
        put(PhoneStateListener.LISTEN_PRECISE_CALL_STATE, "PRECISE_CALL_STATE");
        put(PhoneStateListener.LISTEN_PRECISE_DATA_CONNECTION_STATE,
                "PRECISE_DATA_CONNECTION_STATE");
        put(PhoneStateListener.LISTEN_VOLTE_STATE, "VOLTE_STATE");
        put(PhoneStateListener.LISTEN_CARRIER_NETWORK_CHANGE, "CARRIER_NETWORK_CHANGE");
        put(PhoneStateListener.LISTEN_VOICE_ACTIVATION_STATE, "VOICE_ACTIVATION_STATE");
        put(PhoneStateListener.LISTEN_DATA_ACTIVATION_STATE, "DATA_ACTIVATION_STATE");
    }};

    private final PhoneStateListener phoneStateListener = new PhoneStateListener() {
        @Override
        public void onCellLocationChanged(CellLocation location) {
            notify("onCellLocationChanged", location);
        }

        @Override
        public void onCellInfoChanged(List<CellInfo> cellInfo) {
            notify("onCellInfoChanged", cellInfo);
        }

        private void notify(String method, Object data) {
            Notification.Builder builder = new Notification.Builder(TelephonyRegistryTestApp.this,
                    NOTIFICATION_CHANNEL);
            Notification notification = builder.setSmallIcon(android.R.drawable.sym_def_app_icon)
                    .setContentTitle("Registry update: " + method)
                    .setContentText(data == null ? "null" : data.toString())
                    .build();
            notificationManager.notify(0, notification);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        telephonyManager = getSystemService(TelephonyManager.class);

        LinearLayout eventContainer = (LinearLayout) findViewById(R.id.events);
        for (int i = 0; i < EVENTS.size(); i++) {
            CheckBox box = new CheckBox(this);
            box.setText(EVENTS.valueAt(i));
            final int eventCode = EVENTS.keyAt(i);
            box.setOnCheckedChangeListener((buttonView, isChecked) -> {
                if (buttonView.isChecked()) {
                    mSelectedEvents |= eventCode;
                } else {
                    mSelectedEvents &= ~eventCode;
                }
            });
            eventContainer.addView(box);
        }

        Button registerButton = (Button) findViewById(R.id.registerButton);
        registerButton.setOnClickListener(v ->
                telephonyManager.listen(phoneStateListener, mSelectedEvents));

        Button queryCellLocationButton = findViewById(R.id.queryCellLocationButton);
        queryCellLocationButton.setOnClickListener(v -> {
            List<CellInfo> cellInfos = telephonyManager.getAllCellInfo();
            String cellInfoText;
            if (cellInfos == null || cellInfos.size() == 0) {
                cellInfoText = "null";
            } else {
                cellInfoText = cellInfos.stream().map(CellInfo::toString)
                        .collect(Collectors.joining(","));
            }
            Toast.makeText(TelephonyRegistryTestApp.this, "queryCellInfo: " + cellInfoText,
                    Toast.LENGTH_SHORT).show();
        });

        notificationManager = getSystemService(NotificationManager.class);
        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL,
                "Telephony Registry updates", NotificationManager.IMPORTANCE_HIGH);
        notificationManager.createNotificationChannel(channel);
    }
}
