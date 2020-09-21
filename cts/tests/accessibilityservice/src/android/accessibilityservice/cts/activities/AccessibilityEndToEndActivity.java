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

package android.accessibilityservice.cts.activities;

import android.accessibilityservice.cts.R;

import android.os.Bundle;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.BaseAdapter;
import android.widget.ListAdapter;
import android.widget.ListView;
import android.widget.TextView;

/**
 * This class is an {@link android.app.Activity} used to perform end-to-end
 * testing of the accessibility feature by interaction with the
 * UI widgets.
 */
public class AccessibilityEndToEndActivity extends AccessibilityTestActivity {
    private PackageNameInjector mPackageNameInjector;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.end_to_end_test);

        ListAdapter listAdapter = new BaseAdapter() {
            public View getView(int position, View convertView, ViewGroup parent) {
                TextView textView = (TextView) View
                        .inflate(AccessibilityEndToEndActivity.this, R.layout.list_view_row, null);
                textView.setText((String) getItem(position));
                return textView;
            }

            public long getItemId(int position) {
                return position;
            }

            public Object getItem(int position) {
                if (position == 0) {
                    return AccessibilityEndToEndActivity.this.getString(R.string.first_list_item);
                } else {
                    return AccessibilityEndToEndActivity.this.getString(R.string.second_list_item);
                }
            }

            public int getCount() {
                return 2;
            }
        };

        ListView listView = (ListView) findViewById(R.id.listview);
        listView.setAdapter(listAdapter);
    }

    public void setReportedPackageName(String packageName) {
        if (packageName != null) {
            mPackageNameInjector = new PackageNameInjector(packageName);
        } else {
            mPackageNameInjector = null;
        }
        setPackageNameInjector(getWindow().getDecorView(), mPackageNameInjector);
    }

    private static void setPackageNameInjector(View node, PackageNameInjector injector) {
        if (node == null) {
            return;
        }
        node.setAccessibilityDelegate(injector);
        if (node instanceof ViewGroup) {
            final ViewGroup viewGroup = (ViewGroup) node;
            final int childCount = viewGroup.getChildCount();
            for (int i = 0; i < childCount; i++) {
                setPackageNameInjector(viewGroup.getChildAt(i), injector);
            }
        }
    }

    private static class PackageNameInjector extends View.AccessibilityDelegate {
        private final String mPackageName;

        PackageNameInjector(String packageName) {
            mPackageName = packageName;
        }

        @Override
        public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
            super.onInitializeAccessibilityEvent(host, event);
            event.setPackageName(mPackageName);
        }

        @Override
        public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
            super.onInitializeAccessibilityNodeInfo(host, info);
            info.setPackageName(mPackageName);
        }
    }
}
