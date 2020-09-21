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

package com.android.cts.verifier.tv;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import android.annotation.SuppressLint;
import android.content.Context;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.util.Log;
import android.view.InputDevice;
import android.view.View;
import android.widget.Toast;

import com.android.cts.verifier.R;

/**
 * Tests for verifying that all input devices report correct hasMicrophone() states.
 */
@SuppressLint("NewApi")
public class MicrophoneDeviceTestActivity extends TvAppVerifierActivity
        implements View.OnClickListener {
    private static final String TAG = "MicrophoneDeviceTestActivity";
    private static final boolean PASS = true;

    private InputManager mInputManager;
    private HashMap<View, List<Object>> mInputDeviceItems;
    private View mPreparationYesItem;
    private View mPreparationNoItem;

    @Override
    public void onClick(View v) {
        final View postTarget = getPostTarget();

        if (containsButton(mPreparationYesItem, v)) {
            setPassState(mPreparationYesItem, true);
            setButtonEnabled(mPreparationNoItem, false);
            createInputDeviceItems();
            return;
        } else if (containsButton(mPreparationNoItem, v)) {
            setPassState(mPreparationYesItem, false);
            setButtonEnabled(mPreparationNoItem, false);
            getPassButton().setEnabled(false);
            return;
        } else if (mInputDeviceItems == null) {
            return;
        }

        for (View item : mInputDeviceItems.keySet()) {
            if (containsButton(item, v)) {
                final List<Object> triple = mInputDeviceItems.get(item);
                final boolean userAnswer = (boolean) triple.get(0);
                final boolean actualAnswer = (boolean) triple.get(1);
                final View pairedItem = (View) triple.get(2);

                if (userAnswer == actualAnswer) {
                    setPassState(userAnswer ? item : pairedItem, true);
                    setButtonEnabled(userAnswer ? pairedItem : item, false);
                    item.setTag(PASS); pairedItem.setTag(PASS);
                    if (checkAllPassed()) {
                        getPassButton().setEnabled(true);
                    }
                    return;
                }

                final int messageId =
                    actualAnswer ? R.string.tv_microphone_device_test_negative_mismatch :
                    R.string.tv_microphone_device_test_positive_mismatch;
                Toast.makeText(this, messageId, Toast.LENGTH_LONG).show();
                setPassState(userAnswer ? item : pairedItem, false);
                getPassButton().setEnabled(false);
                return;
            }
        }
    }

    @Override
    protected void createTestItems() {
        mPreparationYesItem = createUserItem(
                R.string.tv_microphone_device_test_prep_question,
                R.string.tv_yes, this);
        setButtonEnabled(mPreparationYesItem, true);
        mPreparationNoItem = createButtonItem(R.string.tv_no, this);
        setButtonEnabled(mPreparationNoItem, true);
    }

    private void createInputDeviceItems() {
        final Context context = MicrophoneDeviceTestActivity.this;
        mInputManager = (InputManager) context.getSystemService(Context.INPUT_SERVICE);

        final int[] inputDeviceIds = mInputManager.getInputDeviceIds();
        mInputDeviceItems = new HashMap<View, List<Object>>();

        for (int inputDeviceId : inputDeviceIds) {
            final InputDevice inputDevice = mInputManager.getInputDevice(inputDeviceId);
            final boolean hasMicrophone = inputDevice.hasMicrophone();
            Log.w(TAG, "name: " + inputDevice.getName() + ", mic: " + hasMicrophone + ", virtual: "
                  + inputDevice.isVirtual() + ", descriptor: " + inputDevice.getDescriptor());

            // Skip virtual input devices such as virtual keyboards.  This does
            // not, e.g., include com.google.android.tv.remote bluetooth connections.
            if (inputDevice.isVirtual()) {
                continue;
            }

            final CharSequence micQuestion =
                getString(R.string.tv_microphone_device_test_mic_question, inputDevice.getName());

            final View inputDeviceYesItem = createUserItem(micQuestion, R.string.tv_yes, this);
            setButtonEnabled(inputDeviceYesItem, true);
            final View inputDeviceNoItem = createButtonItem(R.string.tv_no, this);
            setButtonEnabled(inputDeviceNoItem, true);
            mInputDeviceItems.put(
                inputDeviceYesItem, Arrays.asList(true, hasMicrophone, inputDeviceNoItem));
            mInputDeviceItems.put(
                inputDeviceNoItem, Arrays.asList(false, hasMicrophone, inputDeviceYesItem));
        }

        if (mInputDeviceItems.size() == 0) {
            Toast.makeText(this, R.string.tv_microphone_device_test_no_input_devices,
                           Toast.LENGTH_LONG).show();
            getPassButton().setEnabled(true);
        }
    }

    @Override
    protected void setInfoResources() {
        setInfoResources(R.string.tv_microphone_device_test,
                R.string.tv_microphone_device_test_info, -1);
    }

    private boolean hasPassed(View item) {
        return (item.getTag() != null) && ((Boolean) item.getTag()) == PASS;
    }

    private boolean checkAllPassed() {
        if (mInputDeviceItems != null && mInputDeviceItems.size() > 0) {
            for (View item : mInputDeviceItems.keySet()) {
                if (!hasPassed(item)) {
                    return false;
                }
            }
        }
        return true;
    }
}
