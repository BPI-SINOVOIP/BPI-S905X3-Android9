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
package com.google.android.car.kitchensink.assistant;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.HapticFeedbackConstants;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.Toast;

import com.google.android.car.kitchensink.R;

public class CarAssistantFragment extends Fragment {

    private ImageView mMicIntent;
    private ImageView mMicService;

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                             @Nullable Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.car_assistant, container, false);
        mMicIntent = (ImageView) v.findViewById(R.id.voice_button_intent);
        mMicService = (ImageView) v.findViewById(R.id.voice_button_service);
        Context context = getContext();

        mMicIntent.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                v.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                Intent intent = new Intent();
                intent.setAction(
                        getContext().getString(R.string.assistant_activity_action));
                if (intent.resolveActivity(context.getPackageManager()) != null) {
                    startActivity(intent);
                } else {
                    Toast.makeText(context,
                            "Assistant app is not available.", Toast.LENGTH_SHORT).show();
                }
            }
        });
        mMicService.setOnClickListener(v1 -> {
            v1.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
            boolean success = getActivity().showAssist(null);
            if (!success) {
                Toast.makeText(context,
                        "Assistant app is not available.", Toast.LENGTH_SHORT).show();
            }
        });
        return v;
    }
}
