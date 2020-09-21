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
package com.android.loganalysis.item;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * An {@link IItem} used to store the memory info output.
 */
@SuppressWarnings("serial")
public class MemInfoItem extends GenericMapItem<Long> {

    /** Constant for JSON output */
    public static final String LINES = "LINES";
    /** Constant for JSON output */
    public static final String TEXT = "TEXT";

    private String mText = null;

    /**
     * Get the raw text of the mem info command.
     */
    public String getText() {
        return mText;
    }

    /**
     * Set the raw text of the mem info command.
     */
    public void setText(String text) {
        mText = text;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        try {
            object.put(LINES, super.toJson());
            object.put(TEXT, getText());
        } catch (JSONException e) {
            // Ignore
        }
        return object;
    }
}
