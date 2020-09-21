/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.widget.cts;

import android.app.Activity;
import android.os.Bundle;
import android.view.ViewGroup;
import android.widget.Chronometer;
import android.widget.cts.R;

public class ChronometerCtsActivity extends Activity {

    private Chronometer chronometer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.chronometer_stub_layout);
        chronometer = (Chronometer) findViewById(R.id.test_chronometer);
    }

    public Chronometer getChronometer() {
        return chronometer;
    }

    public ViewGroup getViewGroup() {
        return (ViewGroup) findViewById(R.id.chronometer_view_group);
    }

}

