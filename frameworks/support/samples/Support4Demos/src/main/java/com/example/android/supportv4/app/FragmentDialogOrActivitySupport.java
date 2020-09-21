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

package com.example.android.supportv4.app;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentTransaction;

import com.example.android.supportv4.R;

public class FragmentDialogOrActivitySupport extends FragmentActivity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.fragment_dialog_or_activity);

        if (savedInstanceState == null) {
            // First-time init; create fragment to embed in activity.
//BEGIN_INCLUDE(embed)
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            DialogFragment newFragment = MyDialogFragment.newInstance();
            ft.add(R.id.embedded, newFragment);
            ft.commit();
//END_INCLUDE(embed)
        }

        // Watch for button clicks.
        Button button = (Button)findViewById(R.id.show_dialog);
        button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                showDialog();
            }
        });
    }

//BEGIN_INCLUDE(show_dialog)
    void showDialog() {
        // Create the fragment and show it as a dialog.
        DialogFragment newFragment = MyDialogFragment.newInstance();
        newFragment.show(getSupportFragmentManager(), "dialog");
    }
//END_INCLUDE(show_dialog)

//BEGIN_INCLUDE(dialog)
    public static class MyDialogFragment extends DialogFragment {
        static MyDialogFragment newInstance() {
            return new MyDialogFragment();
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            View v = inflater.inflate(R.layout.hello_world, container, false);
            View tv = v.findViewById(R.id.text);
            ((TextView)tv).setText("This is an instance of MyDialogFragment");
            return v;
        }
    }
//END_INCLUDE(dialog)
}
