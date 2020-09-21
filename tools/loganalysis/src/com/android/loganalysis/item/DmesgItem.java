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

package com.android.loganalysis.item;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * An {@link IItem} used to store different infos logged in dmesg.
 */
public class DmesgItem extends GenericItem {

    private Map<String, DmesgServiceInfoItem> mServiceInfoItems = new HashMap<>();

    private List<DmesgStageInfoItem> mStageInfoItems = new ArrayList<>();

    private List<DmesgActionInfoItem> mActionInfoItems = new ArrayList<>();

    public DmesgItem() {
        super(Collections.emptySet());
    }

    /**
     * @return the serviceInfoItems
     */
    public Map<String, DmesgServiceInfoItem> getServiceInfoItems() {
        return mServiceInfoItems;
    }

    /**
     * @param key to identify service info item
     * @param serviceInfoItem to add
     */
    public void addServiceInfoItem(String key, DmesgServiceInfoItem serviceInfoItem) {
        mServiceInfoItems.put(key, serviceInfoItem);
    }

    /**
     * @return stageInfoItems
     */
    public List<DmesgStageInfoItem> getStageInfoItems() {
        return mStageInfoItems;
    }

    /**
     * @param stageInfoItem to be added to the list
     */
    public void addStageInfoItem(DmesgStageInfoItem stageInfoItem) {
        mStageInfoItems.add(stageInfoItem);
    }

    /**
     * @return actionInfoItems
     */
    public List<DmesgActionInfoItem> getActionInfoItems() {
        return mActionInfoItems;
    }

    /**
     * @param actionInfoItem to be added to the list
     */
    public void addActionInfoItem(DmesgActionInfoItem actionInfoItem) {
        mActionInfoItems.add(actionInfoItem);
    }

}
