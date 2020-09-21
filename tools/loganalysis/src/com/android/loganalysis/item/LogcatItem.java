/*
 * Copyright (C) 2012 The Android Open Source Project
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
import java.util.Date;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * An {@link IItem} used to store logcat info.
 */
public class LogcatItem extends GenericItem {

    /** Constant for JSON output */
    public static final String START_TIME = "START_TIME";
    /** Constant for JSON output */
    public static final String STOP_TIME = "STOP_TIME";
    /** Constant for JSON output */
    public static final String EVENTS = "EVENTS";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            START_TIME, STOP_TIME, EVENTS));

    @SuppressWarnings("serial")
    private class ItemList extends LinkedList<MiscLogcatItem> {}

    /**
     * The constructor for {@link LogcatItem}.
     */
    public LogcatItem() {
        super(ATTRIBUTES);

        setAttribute(EVENTS, new ItemList());
    }

    /**
     * Get the start time of the logcat.
     */
    public Date getStartTime() {
        return (Date) getAttribute(START_TIME);
    }

    /**
     * Set the start time of the logcat.
     */
    public void setStartTime(Date time) {
        setAttribute(START_TIME, time);
    }

    /**
     * Get the stop time of the logcat.
     */
    public Date getStopTime() {
        return (Date) getAttribute(STOP_TIME);
    }

    /**
     * Set the stop time of the logcat.
     */
    public void setStopTime(Date time) {
        setAttribute(STOP_TIME, time);
    }

    /**
     * Get the list of all {@link MiscLogcatItem} events.
     */
    public List<MiscLogcatItem> getEvents() {
        return (ItemList) getAttribute(EVENTS);
    }

    /**
     * Add an {@link MiscLogcatItem} event to the end of the list of events.
     */
    public void addEvent(MiscLogcatItem event) {
        ((ItemList) getAttribute(EVENTS)).add(event);
    }

    /**
     * Get the list of all {@link AnrItem} events.
     */
    public List<AnrItem> getAnrs() {
        List<AnrItem> anrs = new LinkedList<AnrItem>();
        for (IItem item : getEvents()) {
            if (item instanceof AnrItem) {
                anrs.add((AnrItem) item);
            }
        }
        return anrs;
    }

    /**
     * Get the list of all {@link JavaCrashItem} events.
     */
    public List<JavaCrashItem> getJavaCrashes() {
        List<JavaCrashItem> jcs = new LinkedList<JavaCrashItem>();
        for (IItem item : getEvents()) {
            if (item instanceof JavaCrashItem) {
                jcs.add((JavaCrashItem) item);
            }
        }
        return jcs;
    }

    /**
     * Get the list of all {@link NativeCrashItem} events.
     */
    public List<NativeCrashItem> getNativeCrashes() {
        List<NativeCrashItem> ncs = new LinkedList<NativeCrashItem>();
        for (IItem item : getEvents()) {
            if (item instanceof NativeCrashItem) {
                ncs.add((NativeCrashItem) item);
            }
        }
        return ncs;
    }

    /**
     * Get the list of all {@link MiscLogcatItem} events for a cateogry.
     */
    public List<MiscLogcatItem> getMiscEvents(String category) {
        List<MiscLogcatItem> items = new LinkedList<MiscLogcatItem>();
        for (MiscLogcatItem item : getEvents()) {
            if (item.getCategory().equals(category)) {
                items.add(item);
            }
        }
        return items;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LogcatItem merge(IItem other) throws ConflictingItemException {
        if (this == other) {
            return this;
        }
        if (other == null || !(other instanceof LogcatItem)) {
            throw new ConflictingItemException("Conflicting class types");
        }

        LogcatItem logcat = (LogcatItem) other;

        Date start = logcat.getStartTime().before(getStartTime()) ?
                logcat.getStartTime() : getStartTime();
        Date stop = logcat.getStopTime().after(getStopTime()) ?
                logcat.getStopTime() : getStopTime();
        Date overlapStart = logcat.getStartTime().after(getStartTime()) ?
                logcat.getStartTime() : getStartTime();
        Date overlapStop = logcat.getStopTime().before(getStopTime()) ?
                logcat.getStopTime() : getStopTime();

        // Make sure that all events in the overlapping span are
        ItemList mergedEvents = new ItemList();
        for (MiscLogcatItem event : getEvents()) {
            final Date eventTime = event.getEventTime();
            if (eventTime.after(overlapStart) && eventTime.before(overlapStop) &&
                    !logcat.getEvents().contains(event)) {
                throw new ConflictingItemException("Event in first logcat not contained in " +
                        "overlapping portion of other logcat.");
            }
            mergedEvents.add(event);
        }

        for (MiscLogcatItem event : logcat.getEvents()) {
            final Date eventTime = event.getEventTime();
            if (eventTime.after(overlapStart) && eventTime.before(overlapStop)) {
                if (!getEvents().contains(event)) {
                    throw new ConflictingItemException("Event in first logcat not contained in " +
                            "overlapping portion of other logcat.");
                }
            } else {
                mergedEvents.add(event);
            }
        }

        LogcatItem mergedLogcat = new LogcatItem();
        mergedLogcat.setStartTime(start);
        mergedLogcat.setStopTime(stop);
        mergedLogcat.setAttribute(EVENTS, mergedEvents);
        return mergedLogcat;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public JSONObject toJson() {
        JSONObject output = super.toJson();
        JSONArray events = new JSONArray();
        for (MiscLogcatItem event : getEvents()) {
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
