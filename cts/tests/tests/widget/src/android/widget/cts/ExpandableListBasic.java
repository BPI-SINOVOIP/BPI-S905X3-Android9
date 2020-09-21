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

import android.view.Menu;
import android.view.MenuItem;
import android.widget.BaseExpandableListAdapter;
import android.widget.cts.util.ExpandableListScenario;

public class ExpandableListBasic extends ExpandableListScenario {
    private static final int[] CHILD_COUNT = {4, 3, 2, 1, 0};

    @Override
    protected void init(ExpandableParams params) {
        params.setNumChildren(CHILD_COUNT).setItemScreenSizeFactor(0.14);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add("Add item").setOnMenuItemClickListener((MenuItem item) -> {
                mGroups.add(0, new MyGroup(2));
                if (mAdapter != null) {
                    ((BaseExpandableListAdapter) mAdapter).notifyDataSetChanged();
                }
                return true;
        });

        return true;
    }
}
