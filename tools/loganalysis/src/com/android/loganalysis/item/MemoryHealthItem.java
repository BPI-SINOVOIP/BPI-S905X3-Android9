/*
 * Copyright (C) 2015 The Android Open Source Project
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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Map;

/**
 * Contains a list of processes and which type of memory they are using, both in foreground and
 * background.
 */
public class MemoryHealthItem extends GenericItem {
    private static final String BACKGROUND = "background";
    private static final String FOREGROUND = "foreground";

    public static final String DALVIK_AVG = "dalvik_avg";
    public static final String NATIVE_AVG = "native_avg";
    public static final String  PSS_AVG = "pss_avg";
    public static final String  DALVIK_PEAK = "dalvik_peak";
    public static final String NATIVE_PEAK = "native_peak";
    public static final String PSS_PEAK = "pss_peak";

    public static final String SUMMARY_JAVA_HEAP_AVG = "summary_java_heap_avg";
    public static final String SUMMARY_NATIVE_HEAP_AVG = "summary_native_heap_avg";
    public static final String SUMMARY_CODE_AVG = "summary_code_avg";
    public static final String SUMMARY_STACK_AVG = "summary_stack_avg";
    public static final String SUMMARY_GRAPHICS_AVG = "summary_graphics_avg";
    public static final String SUMMARY_OTHER_AVG = "summary_other_avg";
    public static final String SUMMARY_SYSTEM_AVG = "summary_system_avg";
    public static final String SUMMARY_OVERALL_PSS_AVG = "summary_overall_pss_avg";

    public MemoryHealthItem(Map<String, Map<String, Long>> foreground,
            Map<String, Map<String, Long>> background) {
        super(new HashSet<String>(Arrays.asList(FOREGROUND, BACKGROUND)));
        super.setAttribute(FOREGROUND, foreground);
        super.setAttribute(BACKGROUND, background);
    }

    /**
     * Returns breakdown of memory usage of foreground processes.
     * @return Map that stores breakdown of memory usage
     */
    @SuppressWarnings("unchecked")
    public Map<String, Map<String, Long>> getForeground() {
        return (Map<String, Map<String, Long>>) super.getAttribute(FOREGROUND);
    }

    /**
     * Returns breakdown of memory usage of foreground processes.
     * @return Map that stores breakdown of memory usage
     */
    @SuppressWarnings("unchecked")
    public Map<String, Map<String, Long>> getBackground() {
        return (Map<String, Map<String, Long>>) super.getAttribute(BACKGROUND);
    }

    @Override
    public JSONObject toJson() {
        JSONObject memoryHealth = new JSONObject();
        try {
            memoryHealth.put(FOREGROUND, mapToJson(getForeground()));
            memoryHealth.put(BACKGROUND, mapToJson(getBackground()));
        } catch (JSONException e) {
            // ignore
        }
        return memoryHealth;
    }

    private JSONObject mapToJson(Map<String, Map<String, Long>> map) {
        JSONObject out = new JSONObject();
        for (Map.Entry<String, Map<String, Long>> entry : map.entrySet()) {
            try {
                out.put(entry.getKey(), processToJson(entry.getValue()));
            } catch (JSONException e) {
                // ignore
            }
        }
        return out;
    }

    private JSONObject processToJson(Map<String, Long> map) {
        JSONObject out = new JSONObject();
        for (Map.Entry<String, Long> entry : map.entrySet()) {
            try {
                out.put(entry.getKey(), entry.getValue());
            } catch (JSONException e) {
                // ignore
            }
        }
        return out;
    }
}
