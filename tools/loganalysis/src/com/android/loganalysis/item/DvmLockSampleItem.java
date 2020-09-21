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


import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * An {@link IItem} used to store DVM Lock allocation Info
 */
public class DvmLockSampleItem extends GenericItem {

    public static final String PROCESS_NAME = "PROCESS_NAME";
    public static final String SENSITIVITY_FLAG = "SENSITIVITY_FLAG";
    public static final String WAITING_THREAD_NAME = "WAITING_THREAD_NAME";
    public static final String WAIT_TIME = "WAIT_TIME";
    public static final String WAITING_SOURCE_FILE = "WAITING_SOURCE_FILE";
    public static final String WAITING_SOURCE_LINE = "WAITING_SOURCE_LINE";
    public static final String OWNER_FILE_NAME = "OWNER_FILE_NAME";
    public static final String OWNER_ACQUIRE_SOURCE_LINE = "OWNER_ACQUIRE_SOURCE_LINE";
    public static final String SAMPLE_PERCENTAGE = "SAMPLE_PERCENTAGE";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            PROCESS_NAME, SENSITIVITY_FLAG, WAITING_THREAD_NAME, WAIT_TIME,
            WAITING_SOURCE_FILE, WAITING_SOURCE_LINE, OWNER_FILE_NAME,
            OWNER_ACQUIRE_SOURCE_LINE, SAMPLE_PERCENTAGE));

    @SuppressWarnings("serial")
    private static final Map<String, Class<?>> TYPES = new HashMap<String, Class<?>>() {{
        put(PROCESS_NAME, String.class);
        put(SENSITIVITY_FLAG, Boolean.class);
        put(WAITING_THREAD_NAME, String.class);
        put(WAIT_TIME, Integer.class);
        put(WAITING_SOURCE_FILE, String.class);
        put(WAITING_SOURCE_LINE, Integer.class);
        put(OWNER_FILE_NAME, String.class);
        put(OWNER_ACQUIRE_SOURCE_LINE, Integer.class);
        put(SAMPLE_PERCENTAGE, Integer.class);
    }};

    public DvmLockSampleItem() {
        super(ATTRIBUTES);
    }

    /** {@inheritDoc} */
    @Override
    public void setAttribute(String attribute, Object value) throws IllegalArgumentException {
        if(ATTRIBUTES.contains(attribute)) {
            if (TYPES.get(attribute).isAssignableFrom(value.getClass())) {
                super.setAttribute(attribute, value);
            } else {
                throw new IllegalArgumentException(
                        "Invalid attribute type for " + attribute +
                                ": found " + value.getClass().getCanonicalName() +
                                " expected " + TYPES.get(attribute).getCanonicalName());
            }
        } else {
            throw new IllegalArgumentException("Invalid attribute key: " + attribute);
        }
    }

    /** {@inheritDoc} */
    @Override
    public Object getAttribute(String attribute) throws IllegalArgumentException {
        return super.getAttribute(attribute);
    }
}
