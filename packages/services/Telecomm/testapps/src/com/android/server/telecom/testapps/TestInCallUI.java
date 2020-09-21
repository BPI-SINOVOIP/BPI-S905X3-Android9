/*
 * Copyright (C) 2015 Android Open Source Project
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

package com.android.server.telecom.testapps;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.content.Intent;
import android.os.Bundle;
import android.telecom.Call;
import android.telecom.CallAudioState;
import android.telecom.PhoneAccount;
import android.telecom.PhoneAccountHandle;
import android.telecom.TelecomManager;
import android.telecom.VideoProfile;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Collection;
import java.util.List;
import java.util.Optional;

public class TestInCallUI extends Activity {
    private class BluetoothDeviceAdapter extends ArrayAdapter<BluetoothDevice> {
        public BluetoothDeviceAdapter() {
            super(TestInCallUI.this, android.R.layout.simple_spinner_item);
            setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            BluetoothDevice info = getItem(position);
            TextView result = new TextView(TestInCallUI.this);
            result.setText(info.getName());
            return result;
        }

        public void update(Collection<BluetoothDevice> devices) {
            clear();
            addAll(devices);
        }
    }

    public static TestInCallUI sInstance;
    private ListView mListView;
    private TestCallList mCallList;
    private Spinner mBtDeviceList;
    private BluetoothDeviceAdapter mBluetoothDeviceAdapter;
    private TextView mCurrentRouteDisplay;

    /** ${inheritDoc} */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        sInstance = this;

        setContentView(R.layout.incall_screen);

        mListView = (ListView) findViewById(R.id.callListView);
        mListView.setAdapter(new CallListAdapter(this));
        mListView.setVisibility(View.VISIBLE);

        mCallList = TestCallList.getInstance();
        mCallList.addListener(new TestCallList.Listener() {
            @Override
            public void onCallRemoved(Call call) {
                if (mCallList.size() == 0) {
                    Log.i(TestInCallUI.class.getSimpleName(), "Ending the incall UI");
                    finish();
                }
            }

            @Override
            public void onRttStarted(Call call) {
                Toast.makeText(TestInCallUI.this, "RTT now enabled", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onRttStopped(Call call) {
                Toast.makeText(TestInCallUI.this, "RTT now disabled", Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onRttInitiationFailed(Call call, int reason) {
                Toast.makeText(TestInCallUI.this, String.format("RTT failed to init: %d", reason),
                        Toast.LENGTH_SHORT).show();
            }

            @Override
            public void onRttRequest(Call call, int id) {
                Toast.makeText(TestInCallUI.this, String.format("RTT request: %d", id),
                        Toast.LENGTH_SHORT).show();
            }
        });

        View endCallButton = findViewById(R.id.end_call_button);
        View holdButton = findViewById(R.id.hold_button);
        View muteButton = findViewById(R.id.mute_button);
        View rttIfaceButton = findViewById(R.id.rtt_iface_button);
        View answerButton = findViewById(R.id.answer_button);
        View startRttButton = findViewById(R.id.start_rtt_button);
        View acceptRttButton = findViewById(R.id.accept_rtt_button);
        View handoverButton = findViewById(R.id.request_handover_button);
        View setBtDeviceButton = findViewById(R.id.set_bt_device_button);
        View earpieceButton = findViewById(R.id.earpiece_button);
        View speakerButton = findViewById(R.id.speaker_button);
        mBtDeviceList = findViewById(R.id.available_bt_devices);
        mBluetoothDeviceAdapter = new BluetoothDeviceAdapter();
        mBtDeviceList.setAdapter(mBluetoothDeviceAdapter);
        mCurrentRouteDisplay = findViewById(R.id.current_audio_route);

        endCallButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Call call = mCallList.getCall(0);
                if (call != null) {
                    call.disconnect();
                }
            }
        });
        holdButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Call call = mCallList.getCall(0);
                if (call != null) {
                    if (call.getState() == Call.STATE_HOLDING) {
                        call.unhold();
                    } else {
                        call.hold();
                    }
                }
            }
        });
        muteButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                Call call = mCallList.getCall(0);
                if (call != null) {

                }
            }
        });

        rttIfaceButton.setOnClickListener((view) -> {
            Call call = mCallList.getCall(0);
            if (call.isRttActive()) {
                Intent intent = new Intent(Intent.ACTION_MAIN);
                intent.setClass(this, TestRttActivity.class);
                startActivity(intent);
            }
        });

        answerButton.setOnClickListener(view -> {
            Call call = mCallList.getCall(0);
            if (call.getState() == Call.STATE_RINGING) {
                call.answer(VideoProfile.STATE_AUDIO_ONLY);
            }
        });

        startRttButton.setOnClickListener(view -> {
            Call call = mCallList.getCall(0);
            if (!call.isRttActive()) {
                call.sendRttRequest();
            }
        });

        earpieceButton.setOnClickListener(view -> {
            TestInCallServiceImpl.sInstance.setAudioRoute(CallAudioState.ROUTE_WIRED_OR_EARPIECE);
        });

        speakerButton.setOnClickListener(view -> {
            TestInCallServiceImpl.sInstance.setAudioRoute(CallAudioState.ROUTE_SPEAKER);
        });

        setBtDeviceButton.setOnClickListener(view -> {
            if (mBtDeviceList.getSelectedItem() != null
                    && TestInCallServiceImpl.sInstance != null) {
                TestInCallServiceImpl.sInstance.requestBluetoothAudio(
                        (BluetoothDevice) mBtDeviceList.getSelectedItem());
            }
        });

        acceptRttButton.setOnClickListener(view -> {
            Call call = mCallList.getCall(0);
            if (!call.isRttActive()) {
                call.respondToRttRequest(mCallList.getLastRttRequestId(), true);
            }
        });

        handoverButton.setOnClickListener((v) -> {
            Call call = mCallList.getCall(0);
            Bundle extras = new Bundle();
            extras.putParcelable(Call.EXTRA_HANDOVER_PHONE_ACCOUNT_HANDLE,
                    getHandoverToPhoneAccountHandle());
            extras.putInt(Call.EXTRA_HANDOVER_VIDEO_STATE, VideoProfile.STATE_BIDIRECTIONAL);
            call.sendCallEvent(Call.EVENT_REQUEST_HANDOVER, extras);
        });
    }

    public void updateCallAudioState(CallAudioState cas) {
        mBluetoothDeviceAdapter.update(cas.getSupportedBluetoothDevices());
        String routeText;
        switch (cas.getRoute()) {
            case CallAudioState.ROUTE_EARPIECE:
                routeText = "Earpiece";
                break;
            case CallAudioState.ROUTE_SPEAKER:
                routeText = "Speaker";
                break;
            case CallAudioState.ROUTE_WIRED_HEADSET:
                routeText = "Wired";
                break;
            case CallAudioState.ROUTE_BLUETOOTH:
                BluetoothDevice activeDevice = cas.getActiveBluetoothDevice();
                routeText = activeDevice == null ? "null bt" : activeDevice.getName();
                break;
            default:
                routeText = "unknown: " + cas.getRoute();
        }
        mCurrentRouteDisplay.setText(routeText);
    }

    /** ${inheritDoc} */
    @Override
    protected void onDestroy() {
        sInstance = null;
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    private PhoneAccountHandle getHandoverToPhoneAccountHandle() {
        TelecomManager tm = TelecomManager.from(this);

        List<PhoneAccountHandle> handles = tm.getAllPhoneAccountHandles();
        Optional<PhoneAccountHandle> found = handles.stream().filter(h -> {
            PhoneAccount account = tm.getPhoneAccount(h);
            Bundle extras = account.getExtras();
            return extras != null && extras.getBoolean(PhoneAccount.EXTRA_SUPPORTS_HANDOVER_TO);
        }).findFirst();
        PhoneAccountHandle foundHandle = found.orElse(null);
        Log.i(TestInCallUI.class.getSimpleName(), "getHandoverToPhoneAccountHandle() = " +
            foundHandle);
        return foundHandle;
    }
}
