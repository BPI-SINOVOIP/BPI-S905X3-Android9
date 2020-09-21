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
 * limitations under the License.
 */

package com.android.phone.testapps.imstestapp;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.TextView;

import java.util.ArrayList;

public class ImsConfigActivity extends Activity {

    private static final String PREFIX_ITEM = "Item: ";
    private static final String PREFIX_VALUE = "Value: ";

    public static class ConfigItemAdapter extends ArrayAdapter<TestImsConfigImpl.ConfigItem> {
        public ConfigItemAdapter(Context context, ArrayList<TestImsConfigImpl.ConfigItem> configs) {
            super(context, 0, configs);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            TestImsConfigImpl.ConfigItem configItem = getItem(position);

            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.config_item,
                        parent, false);
            }

            TextView textItem = (TextView) convertView.findViewById(R.id.configItem);
            TextView textValue = (TextView) convertView.findViewById(R.id.configValue);

            textItem.setText(PREFIX_ITEM + configItem.item);
            if (configItem.valueString != null) {
                textValue.setText(PREFIX_VALUE + configItem.valueString);
            } else {
                textValue.setText(PREFIX_VALUE + configItem.value);
            }

            return convertView;
        }
    }

    private final TestImsConfigImpl.ImsConfigListener mConfigListener =
            new TestImsConfigImpl.ImsConfigListener() {
                @Override
                public void notifyConfigChanged() {
                    Log.i("ImsConfigActivity", "notifyConfigChanged");
                    mConfigItemAdapter.notifyDataSetChanged();
                }
            };

    ConfigItemAdapter mConfigItemAdapter;
    ListView mListView;

    EditText mConfigItemText;
    EditText mConfigValueText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_config);
    }

    @Override
    protected void onResume() {
        super.onResume();

        mConfigItemAdapter = new ConfigItemAdapter(this,
                TestImsConfigImpl.getInstance().getConfigList());

        mListView = (ListView) findViewById(R.id.config_list);
        mListView.setAdapter(mConfigItemAdapter);

        TestImsConfigImpl.getInstance().setConfigListener(mConfigListener);

        Button setConfigButton = findViewById(R.id.config_button);
        setConfigButton.setOnClickListener((v) -> onSetConfigClicked());

        mConfigItemText = findViewById(R.id.set_config_item);
        mConfigValueText = findViewById(R.id.set_config_value);
    }

    @Override
    protected void onPause() {
        super.onPause();

        TestImsConfigImpl.getInstance().setConfigListener(null);
    }

    private void onSetConfigClicked() {
        String configItem = mConfigItemText.getText().toString();
        String configValue = mConfigValueText.getText().toString();
        TestImsConfigImpl.getInstance().setConfigValue(Integer.parseInt(configItem),
                Integer.parseInt(configValue));
    }
}
