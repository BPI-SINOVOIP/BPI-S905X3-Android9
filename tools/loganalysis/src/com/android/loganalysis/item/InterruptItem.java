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

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * An {@link IItem} used to store information related to interrupts
 */
public class InterruptItem implements IItem {
    /** Constant for JSON output */
    public static final String INTERRUPTS = "INTERRUPT_INFO";

    private Collection<InterruptInfoItem> mInterrupts = new LinkedList<InterruptInfoItem>();

    /**
     * Enum for describing the type of interrupt
     */
    public enum InterruptCategory {
        WIFI_INTERRUPT,
        MODEM_INTERRUPT,
        ALARM_INTERRUPT,
        ADSP_INTERRUPT,
        UNKNOWN_INTERRUPT,
    }

    public static class InterruptInfoItem extends GenericItem {
        /** Constant for JSON output */
        public static final String NAME = "NAME";
        /** Constant for JSON output */
        public static final String CATEGORY = "CATEGORY";
        /** Constant for JSON output */
        public static final String INTERRUPT_COUNT = "INTERRUPT_COUNT";

        private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
                NAME, INTERRUPT_COUNT, CATEGORY));

        /**
         * The constructor for {@link InterruptItem}
         *
         * @param name The name of the wake lock
         * @param interruptCount The number of times the interrupt woke up the AP
         * @param category The {@link InterruptCategory} of the interrupt
         */
        public InterruptInfoItem(String name, int interruptCount,
                InterruptCategory category) {
            super(ATTRIBUTES);

            setAttribute(NAME, name);
            setAttribute(INTERRUPT_COUNT, interruptCount);
            setAttribute(CATEGORY, category);
        }

        /**
         * Get the name of the interrupt
         */
        public String getName() {
            return (String) getAttribute(NAME);
        }

        /**
         * Get the interrupt count.
         */
        public int getInterruptCount() {
            return (Integer) getAttribute(INTERRUPT_COUNT);
        }

        /**
         * Get the {@link InterruptCategory} of the wake lock.
         */
        public InterruptCategory getCategory() {
            return (InterruptCategory) getAttribute(CATEGORY);
        }
    }

    /**
     * Add an interrupt from the battery info section.
     *
     * @param name The name of the interrupt
     * @param interruptCount Number of interrupts
     * @param category The {@link InterruptCategory} of the interrupt.
     */
    public void addInterrupt(String name, int interruptCount,
            InterruptCategory category) {
        mInterrupts.add(new InterruptInfoItem(name, interruptCount, category));
    }

    /**
     * Get a list of {@link InterruptInfoItem} objects matching a given {@link InterruptCategory}.
     */
    public List<InterruptInfoItem> getInterrupts(InterruptCategory category) {
        LinkedList<InterruptInfoItem> interrupts = new LinkedList<InterruptInfoItem>();
        if (category == null) {
            return interrupts;
        }

        for (InterruptInfoItem interrupt : mInterrupts) {
            if (category.equals(interrupt.getCategory())) {
                interrupts.add(interrupt);
            }
        }
        return interrupts;
    }

    /**
     * Get a list of {@link InterruptInfoItem} objects
     */
    public List<InterruptInfoItem> getInterrupts() {
        return (List<InterruptInfoItem>) mInterrupts;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        throw new ConflictingItemException("Wakelock items cannot be merged");
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean isConsistent(IItem other) {
        return false;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        if (mInterrupts != null) {
            try {
                JSONArray interrupts = new JSONArray();
                for (InterruptInfoItem interrupt : mInterrupts) {
                    interrupts.put(interrupt.toJson());
                }
                object.put(INTERRUPTS, interrupts);
            } catch (JSONException e) {
                // Ignore
            }
        }
        return object;
    }
}
