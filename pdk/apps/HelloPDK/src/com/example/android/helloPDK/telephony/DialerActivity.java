/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.example.android.helloPDK.telephony;

import com.example.android.helloPDK.R;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;

public class DialerActivity extends Activity {
    private EditText mText;
    private Button mButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setContentView(R.layout.dialer);
        mText = (EditText)findViewById(R.id.editText_dialer);
        mButton = (Button)findViewById(R.id.button_dialer);
        mButton.setOnClickListener(new View.OnClickListener() {

            @Override
            public void onClick(View arg0) {
                Uri uri = Uri.parse("tel:" + mText.getText().toString());
                Intent intent = new Intent(Intent.ACTION_CALL, uri);
                startActivity(intent);

            }
        });
        super.onCreate(savedInstanceState);
    }

}
