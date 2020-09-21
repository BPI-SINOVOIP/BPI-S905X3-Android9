/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.tv.dvr.ui.list;

import android.app.Activity;
import android.os.Bundle;
import com.android.tv.R;
import com.android.tv.Starter;

/** Activity to show the recording history. */
public class DvrHistoryActivity extends Activity {

    @Override
    public void onCreate(final Bundle savedInstanceState) {
        Starter.start(this);
        // Pass null to prevent automatically re-creating fragments
        super.onCreate(null);
        setContentView(R.layout.activity_dvr_history);
        DvrHistoryFragment dvrHistoryFragment = new DvrHistoryFragment();
        getFragmentManager()
                .beginTransaction()
                .add(R.id.fragment_container, dvrHistoryFragment)
                .commit();
    }
}
