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
package com.android.documentsui.bots;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertTrue;

import android.app.Activity;
import android.content.Context;
import android.support.test.uiautomator.UiDevice;
import android.support.test.uiautomator.UiSelector;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.documentsui.R;
import com.android.documentsui.inspector.DetailsView;
import com.android.documentsui.inspector.KeyValueRow;
import com.android.documentsui.inspector.TableView;

import java.util.HashMap;
import java.util.Map;

public class InspectorBot extends Bots.BaseBot {

    public InspectorBot(UiDevice device, Context context, int timeout) {
        super(device, context, timeout);
    }

    public void assertTitle(String expected) throws Exception {
        UiSelector textView
                = new UiSelector().resourceId("com.android.documentsui:id/inspector_file_title");
        String text = mDevice.findObject(textView).getText();
        assertEquals(expected, text);
    }

    public void assertRowPresent(String key, Activity activity)
            throws Exception {
        assertTrue(isRowPresent(key, activity));
    }

    public void assertRowEquals(String key, String value, Activity activity)
            throws Exception {
        assertTrue(isRowEquals(key, value, activity));
    }

    private static Map<String, String> getTableRows(TableView table)
            throws Exception {
        Map<String, String> rows = new HashMap<>();
        int children = table.getChildCount();
        for (int i = 0; i < children; i++) {
            View view = table.getChildAt(i);
            if (view instanceof KeyValueRow) {
                LinearLayout row = (LinearLayout) table.getChildAt(i);
                TextView key = (TextView) row.getChildAt(0);
                TextView value = (TextView) row.getChildAt(1);
                rows.put(
                        String.valueOf(key.getText()),
                        String.valueOf(value.getText()));
            }
        }
        return rows;
    }

    private boolean isRowPresent(String key, Activity activity)
            throws Exception {
        DetailsView details = (DetailsView) activity.findViewById(R.id.inspector_details_view);
        Map<String, String> rows = getTableRows(details);
        return rows.containsKey(key);
    }

    private boolean isRowEquals(String key, String value, Activity activity)
            throws Exception {
        DetailsView details = (DetailsView) activity.findViewById(R.id.inspector_details_view);
        Map<String, String> rows = getTableRows(details);
        return rows.containsKey(key) && value.equals(rows.get(key));
    }
}
