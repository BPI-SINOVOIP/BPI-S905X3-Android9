/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cts.verifier;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;

public class ReportViewerActivity extends Activity {

    public static final String EXTRA_REPORT_CONTENTS = "reportContents";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.report_viewer);

        Intent intent = getIntent();
        if (intent != null) {
            String reportContents = intent.getStringExtra(EXTRA_REPORT_CONTENTS);
            if (reportContents != null) {
                TextView reportText = (TextView) findViewById(R.id.report_text);
                reportText.setText(reportContents);
            }
        }
    }
}
