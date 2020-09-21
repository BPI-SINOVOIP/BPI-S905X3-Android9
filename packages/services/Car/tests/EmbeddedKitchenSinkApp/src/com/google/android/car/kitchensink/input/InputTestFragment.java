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
package com.google.android.car.kitchensink.input;

import static android.hardware.automotive.vehicle.V2_0.SubscribeFlags.EVENTS_FROM_ANDROID;
import static android.hardware.automotive.vehicle.V2_0.SubscribeFlags.EVENTS_FROM_CAR;
import static android.hardware.automotive.vehicle.V2_0.VehicleDisplay.INSTRUMENT_CLUSTER;
import static android.hardware.automotive.vehicle.V2_0.VehicleHwKeyInputAction.ACTION_DOWN;
import static android.hardware.automotive.vehicle.V2_0.VehicleProperty.HW_KEY_INPUT;

import android.annotation.Nullable;
import android.annotation.StringRes;
import android.hardware.automotive.vehicle.V2_0.IVehicle;
import android.hardware.automotive.vehicle.V2_0.IVehicleCallback;
import android.hardware.automotive.vehicle.V2_0.IVehicleCallback.Stub;
import android.hardware.automotive.vehicle.V2_0.SubscribeOptions;
import android.hardware.automotive.vehicle.V2_0.VehicleArea;
import android.hardware.automotive.vehicle.V2_0.VehicleDisplay;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyGroup;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyStatus;
import android.hardware.automotive.vehicle.V2_0.VehiclePropertyType;
import android.os.Bundle;
import android.os.RemoteException;
import android.support.v4.app.Fragment;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.android.car.keventreader.EventReaderService;
import com.android.car.keventreader.IEventCallback;
import com.android.car.keventreader.KeypressEvent;

import com.google.android.car.kitchensink.R;
import com.google.android.collect.Lists;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.NoSuchElementException;

/**
 * Test input event handling to system.
 * vehicle hal should have VEHICLE_PROPERTY_HW_KEY_INPUT support for this to work.
 */
public class InputTestFragment extends Fragment {

    private static final String TAG = "CAR.INPUT.KS";

    private static final Button BREAK_LINE = null;

    private final List<View> mButtons = new ArrayList<>();

    // This is fake data generation property available only in emulated VHAL implementation.
    private static final int sGenerateFakeDataControllingProperty =
            0x0666 | VehiclePropertyGroup.VENDOR | VehicleArea.GLOBAL | VehiclePropertyType.MIXED;
    // The key press command is sent with the fake data generation property. It's matching the
    // command defined in the emulated VHAL implementation.
    private static final int sKeyPressCommand = 100;

    private IVehicle mVehicle;

    private EventReaderService mEventReaderService;

    private final IEventCallback.Stub mKeypressEventHandler = new IEventCallback.Stub() {
        private String prettyPrint(KeypressEvent event) {
            return String.format("Event{source = %s, keycode = %s, key%s}\n",
                event.source,
                event.keycodeToString(),
                event.isKeydown ? "down" : "up");
        }

        @Override
        public void onEvent(KeypressEvent keypressEvent) throws RemoteException {
            Log.d(TAG, "received event " + keypressEvent);
            synchronized (mInputEventsList) {
                mInputEventsList.append(prettyPrint(keypressEvent));
            }
        }
    };

    private final IVehicleCallback.Stub mHalKeyEventHandler = new Stub() {
        private String prettyPrint(VehiclePropValue event) {
            if (event.prop != HW_KEY_INPUT) return "";
            if (event.value == null ||
                event.value.int32Values == null ||
                event.value.int32Values.size() < 2) return "";
            return String.format("Event{source = HAL, keycode = %s, key%s}\n",
                event.value.int32Values.get(1),
                event.value.int32Values.get(0) == ACTION_DOWN ? "down" : "up");
        }

        @Override
        public void onPropertyEvent(ArrayList<VehiclePropValue> propValues) throws RemoteException {
            synchronized (mInputEventsList) {
                propValues.forEach(vpv -> mInputEventsList.append(prettyPrint(vpv)));
            }
        }

        @Override
        public void onPropertySet(VehiclePropValue propValue) {}

        @Override
        public void onPropertySetError(int errorCode, int propId, int areaId) {}
    };

    private TextView mInputEventsList;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        try {
            mVehicle = IVehicle.getService();
        } catch (NoSuchElementException ex) {
            throw new RuntimeException("Couldn't connect to " + IVehicle.kInterfaceName, ex);
        } catch (RemoteException e) {
            throw new RuntimeException("Failed to connect to IVehicle");
        }
        Log.d(TAG, "Connected to IVehicle service: " + mVehicle);

        mEventReaderService = EventReaderService.tryGet();
        Log.d(TAG, "Key Event Reader service: " + mEventReaderService);
        if (mEventReaderService != null) {
            mEventReaderService.registerCallback(mKeypressEventHandler);
        }

        SubscribeOptions subscribeOption = new SubscribeOptions();
        subscribeOption.propId = HW_KEY_INPUT;
        subscribeOption.flags = EVENTS_FROM_CAR | EVENTS_FROM_ANDROID;
        ArrayList<SubscribeOptions> subscribeOptions = new ArrayList<>();
        subscribeOptions.add(subscribeOption);
        try {
            mVehicle.subscribe(mHalKeyEventHandler, subscribeOptions);
        } catch (RemoteException e) {
            Log.e(TAG, "failed to connect to VHAL for key events", e);
        }
    }

    @Nullable
    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.input_test, container, false);

        mInputEventsList = view.findViewById(R.id.events_list);
        mInputEventsList.setMovementMethod(new ScrollingMovementMethod());

        TextView steeringWheelLabel = new TextView(getActivity() /*context*/);
        steeringWheelLabel.setText(R.string.steering_wheel);
        steeringWheelLabel.setTextSize(getResources().getDimension(R.dimen.car_title2_size));

        Collections.addAll(mButtons,
                BREAK_LINE,
                createButton(R.string.home, KeyEvent.KEYCODE_HOME),
                createButton(R.string.volume_up, KeyEvent.KEYCODE_VOLUME_UP),
                createButton(R.string.volume_down, KeyEvent.KEYCODE_VOLUME_DOWN),
                createButton(R.string.volume_mute, KeyEvent.KEYCODE_VOLUME_MUTE),
                createButton(R.string.voice, KeyEvent.KEYCODE_VOICE_ASSIST),
                BREAK_LINE,
                createButton(R.string.music, KeyEvent.KEYCODE_MUSIC),
                createButton(R.string.music_play, KeyEvent.KEYCODE_MEDIA_PLAY),
                createButton(R.string.music_stop, KeyEvent.KEYCODE_MEDIA_STOP),
                createButton(R.string.next_song, KeyEvent.KEYCODE_MEDIA_NEXT),
                createButton(R.string.prev_song, KeyEvent.KEYCODE_MEDIA_PREVIOUS),
                createButton(R.string.tune_right, KeyEvent.KEYCODE_CHANNEL_UP),
                createButton(R.string.tune_left, KeyEvent.KEYCODE_CHANNEL_DOWN),
                BREAK_LINE,
                createButton(R.string.call_send, KeyEvent.KEYCODE_CALL),
                createButton(R.string.call_end, KeyEvent.KEYCODE_ENDCALL),
                BREAK_LINE,
                steeringWheelLabel,
                BREAK_LINE,
                createButton(R.string.sw_left, KeyEvent.KEYCODE_DPAD_LEFT, INSTRUMENT_CLUSTER),
                createButton(R.string.sw_right, KeyEvent.KEYCODE_DPAD_RIGHT,
                        INSTRUMENT_CLUSTER),
                createButton(R.string.sw_up, KeyEvent.KEYCODE_DPAD_UP, INSTRUMENT_CLUSTER),
                createButton(R.string.sw_down, KeyEvent.KEYCODE_DPAD_DOWN, INSTRUMENT_CLUSTER),
                createButton(R.string.sw_center, KeyEvent.KEYCODE_DPAD_CENTER,
                        INSTRUMENT_CLUSTER),
                createButton(R.string.sw_back, KeyEvent.KEYCODE_BACK, INSTRUMENT_CLUSTER)
        );

        addButtonsToPanel(view.findViewById(R.id.input_buttons), mButtons);

        return view;
    }

    private Button createButton(@StringRes int textResId, int keyCode) {
        return createButton(textResId, keyCode, VehicleDisplay.MAIN);
    }

    private Button createButton(@StringRes int textResId, int keyCode, int targetDisplay) {
        Button button = new Button(getContext());
        button.setText(getContext().getString(textResId));
        button.setTextSize(getResources().getDimension(R.dimen.car_button_text_size));
        button.setOnClickListener(v -> onButtonClick(keyCode, targetDisplay));
        return button;
    }

    private void onButtonClick(int keyCode, int targetDisplay) {
        VehiclePropValue prop = new VehiclePropValue();
        prop.prop = sGenerateFakeDataControllingProperty;
        prop.value.int32Values.addAll(Lists.newArrayList(
                sKeyPressCommand, HW_KEY_INPUT, keyCode, targetDisplay));
        int status;
        try {
            status = mVehicle.set(prop);
        } catch (RemoteException e) {
            throw new RuntimeException("Failed to inject key press");
        }

        if (VehiclePropertyStatus.AVAILABLE != status) {
            Toast.makeText(getContext(), "Failed to inject key event, status:" + status,
                    Toast.LENGTH_LONG).show();
        }
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mButtons.clear();
        if (mEventReaderService != null) {
            mEventReaderService.unregisterCallback(mKeypressEventHandler);
        }
        try {
            mVehicle.unsubscribe(mHalKeyEventHandler, HW_KEY_INPUT);
        } catch (RemoteException e) {
            Log.e(TAG, "failed to remove HAL registration for keypress events", e);
        }
    }

    private void addButtonsToPanel(LinearLayout root, List<View> buttons) {
        LinearLayout panel = null;
        for (View button : buttons) {
            if (button == BREAK_LINE || panel == null) {
                panel = new LinearLayout(getContext());
                panel.setOrientation(LinearLayout.HORIZONTAL);
                root.addView(panel);
            } else {
                panel.addView(button);
                panel.setPadding(0, 10, 10, 0);
            }
        }
    }
}
