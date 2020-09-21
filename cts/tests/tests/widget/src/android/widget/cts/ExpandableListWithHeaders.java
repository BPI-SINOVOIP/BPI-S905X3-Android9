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

import android.os.Bundle;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.cts.util.ExpandableListScenario;

public class ExpandableListWithHeaders extends ExpandableListScenario {
    private static final int[] CHILD_COUNT = {1, 4, 3, 2, 6};
    private static final int HEADER_FOOTER_COUNT = 12;

    @Override
    protected void init(ExpandableParams params) {
        params.setStackFromBottom(false)
                .setStartingSelectionPosition(-1)
                .setNumChildren(CHILD_COUNT)
                .setItemScreenSizeFactor(0.14)
                .setConnectAdapter(false);
    }

    @Override
    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);

        final ExpandableListView expandableListView = getExpandableListView();
        expandableListView.setItemsCanFocus(true);

        for (int i = 0; i < HEADER_FOOTER_COUNT; i++) {
            Button header = new Button(this);
            header.setText("Header View");
            expandableListView.addHeaderView(header);
        }

        for (int i = 0; i < HEADER_FOOTER_COUNT; i++) {
            Button footer = new Button(this);
            footer.setText("Footer View");
            expandableListView.addFooterView(footer);
        }

        // Set adapter here AFTER we set header and footer views
        setAdapter(expandableListView);
    }

    /**
     * @return The number of headers (and the same number of footers)
     */
    public int getNumOfHeadersAndFooters() {
        return HEADER_FOOTER_COUNT;
    }
}
