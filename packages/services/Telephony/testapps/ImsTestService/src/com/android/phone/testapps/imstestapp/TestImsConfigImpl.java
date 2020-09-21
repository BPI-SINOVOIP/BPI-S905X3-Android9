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

package com.android.phone.testapps.imstestapp;

import android.telephony.ims.stub.ImsConfigImplBase;

import com.android.ims.ImsConfig;

import java.util.ArrayList;

public class TestImsConfigImpl extends ImsConfigImplBase {

    public static class ConfigItem {
        public int item;
        public int value;
        public String valueString;

        public ConfigItem(int item, int value) {
            this.item = item;
            this.value = value;
        }

        public ConfigItem(int item, String value) {
            this.item = item;
            valueString = value;
        }
    }

    public interface ImsConfigListener {
        void notifyConfigChanged();
    }

    private static TestImsConfigImpl sTestImsConfigImpl;
    private ImsConfigListener mListener;
    private ArrayList<ConfigItem> mArrayOfConfigs = new ArrayList<>();

    public static TestImsConfigImpl getInstance() {
        if (sTestImsConfigImpl == null) {
            sTestImsConfigImpl = new TestImsConfigImpl();
        }
        return sTestImsConfigImpl;
    }

    private TestImsConfigImpl() {
        super();
    }

    public void setConfigListener(ImsConfigListener listener) {
        mListener = listener;
    }

    public ArrayList<ConfigItem> getConfigList() {
        return mArrayOfConfigs;
    }

    @Override
    public int setConfig(int item, int value) {
        replaceConfig(new ConfigItem(item, value));
        return ImsConfig.OperationStatusConstants.SUCCESS;
    }

    @Override
    public int setConfig(int item, String value) {
        replaceConfig(new ConfigItem(item, value));
        return ImsConfig.OperationStatusConstants.SUCCESS;
    }

    @Override
    public int getConfigInt(int item) {
        replaceConfig(new ConfigItem(item, ImsConfig.FeatureValueConstants.ON));
        return ImsConfig.FeatureValueConstants.ON;
    }

    @Override
    public String getConfigString(int item) {
        return null;
    }

    public void setConfigValue(int item, int value) {
        replaceConfig(new ConfigItem(item, value));
        notifyProvisionedValueChanged(item, value);
    }

    public void replaceConfig(ConfigItem configItem) {
        ConfigItem config = mArrayOfConfigs.stream()
                .filter(configElem -> configElem.item == configItem.item)
                .findFirst().orElse(null);
        if (config != null) {
            mArrayOfConfigs.remove(config);
        }
        mArrayOfConfigs.add(configItem);
        if (mListener != null) {
            mListener.notifyConfigChanged();
        }
    }
}
