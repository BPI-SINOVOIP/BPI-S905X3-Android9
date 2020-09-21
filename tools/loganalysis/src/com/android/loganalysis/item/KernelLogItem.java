/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.loganalysis.parser.KernelLogParser;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * A {@link IItem} used to store kernel log info.
 */
public class KernelLogItem extends GenericItem {

    /** Constant for JSON output */
    public static final String START_TIME = "START_TIME";
    /** Constant for JSON output */
    public static final String STOP_TIME = "STOP_TIME";
    /** Constant for JSON output */
    public static final String EVENTS = "EVENTS";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            START_TIME, STOP_TIME, EVENTS));

    @SuppressWarnings("serial")
    private class ItemList extends LinkedList<MiscKernelLogItem> {}

    /**
     * The constructor for {@link KernelLogItem}.
     */
    public KernelLogItem() {
        super(ATTRIBUTES);

        setAttribute(START_TIME, Double.valueOf(0.0));
        setAttribute(STOP_TIME, Double.valueOf(0.0));
        setAttribute(EVENTS, new ItemList());
    }

    /**
     * Get the start time of the kernel log.
     */
    public Double getStartTime() {
        return (Double) getAttribute(START_TIME);
    }

    /**
     * Set the start time of the kernel log.
     */
    public void setStartTime(Double time) {
        setAttribute(START_TIME, time);
    }

    /**
     * Get the stop time of the kernel log.
     */
    public Double getStopTime() {
        return (Double) getAttribute(STOP_TIME);
    }

    /**
     * Set the stop time of the kernel log.
     */
    public void setStopTime(Double time) {
        setAttribute(STOP_TIME, time);
    }

    /**
     * Get the list of all {@link MiscKernelLogItem} events.
     */
    public List<MiscKernelLogItem> getEvents() {
        return (ItemList) getAttribute(EVENTS);
    }

    /**
     * Add an {@link MiscKernelLogItem} event to the end of the list of events.
     */
    public void addEvent(MiscKernelLogItem event) {
        // Only take the first kernel reset
        if (KernelLogParser.KERNEL_RESET.equals(event.getCategory()) &&
                !getMiscEvents(KernelLogParser.KERNEL_RESET).isEmpty()) {
            return;
        }
        ((ItemList) getAttribute(EVENTS)).add(event);
    }

    /**
     * Get the list of all {@link MiscKernelLogItem} events for a category.
     */
    public List<MiscKernelLogItem> getMiscEvents(String category) {
        List<MiscKernelLogItem> items = new LinkedList<MiscKernelLogItem>();
        for (MiscKernelLogItem item : getEvents()) {
            if (item.getCategory().equals(category)) {
                items.add(item);
            }
        }
        return items;
    }

    /**
     * Get the list of all {@link SELinuxItem} events.
     */
    public List<SELinuxItem> getSELinuxEvents() {
        List<SELinuxItem> items = new LinkedList<SELinuxItem>();
        for (MiscKernelLogItem item : getEvents()) {
            if (item instanceof SELinuxItem) {
                items.add((SELinuxItem)item);
            }
        }
        return items;
    }

    /**
     * Get the list of all {@link LowMemoryKillerItem} events.
     */
    public List<LowMemoryKillerItem> getLowMemoryKillerEvents() {
        List<LowMemoryKillerItem> items = new LinkedList<LowMemoryKillerItem>();
        for (MiscKernelLogItem item : getEvents()) {
            if (item instanceof LowMemoryKillerItem) {
                items.add((LowMemoryKillerItem)item);
            }
        }
        return items;
    }

    /**
     * Get the list of all {@link PageAllocationFailureItem} events.
     */
    public List<PageAllocationFailureItem> getPageAllocationFailureEvents() {
        List<PageAllocationFailureItem> items = new LinkedList<PageAllocationFailureItem>();
        for (MiscKernelLogItem item : getEvents()) {
            if (item instanceof PageAllocationFailureItem) {
                items.add((PageAllocationFailureItem)item);
            }
        }
        return items;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject output = super.toJson();
        JSONArray events = new JSONArray();
        for (MiscKernelLogItem event : getEvents()) {
            events.put(event.toJson());
        }

        try {
            output.put(EVENTS, events);
        } catch (JSONException e) {
            // Ignore
        }
        return output;
    }
}
