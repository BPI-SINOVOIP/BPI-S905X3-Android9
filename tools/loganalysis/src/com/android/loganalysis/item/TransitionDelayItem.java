/*
 * Copyright (C) 2016 The Android Open Source Project
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
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to TransitionDelayInfo.
 */
public class TransitionDelayItem extends GenericItem {

    /** Constant for JSON output */
    public static final String COMPONENT_NAME = "COMPONENT_NAME";
    /** Constant for JSON output */
    public static final String START_WINDOW_DELAY = "START_WINDOW_DELAY";
    /** Constant for JSON output */
    public static final String TRANSITION_DELAY = "TRANSITION_DELAY";
    /** Constant for JSON output */
    public static final String DATE_TIME = "DATE_TIME";
    /** Constant for JSON output */
    public static final String WINDOW_DRAWN_DELAY = "WINDOW_DRAWN_DELAY";

    private static final Set<String> ATTRIBUTES =
            new HashSet<String>(
                    Arrays.asList(
                            COMPONENT_NAME,
                            START_WINDOW_DELAY,
                            TRANSITION_DELAY,
                            DATE_TIME,
                            WINDOW_DRAWN_DELAY));

    /**
     * The constructor for {@link TransitionDelayItem}.
     */
    public TransitionDelayItem() {
        super(ATTRIBUTES);
    }

    public String getComponentName() {
        return (String) getAttribute(COMPONENT_NAME);
    }

    public void setComponentName(String componentName) {
        setAttribute(COMPONENT_NAME, componentName);
    }

    public Long getStartingWindowDelay() {
        return getAttribute(START_WINDOW_DELAY) != null ? (Long) getAttribute(START_WINDOW_DELAY)
                : null;
    }

    public void setStartingWindowDelay(long startingWindowDelay) {
        setAttribute(START_WINDOW_DELAY, startingWindowDelay);
    }

    public Long getTransitionDelay() {
        return (Long) getAttribute(TRANSITION_DELAY);
    }

    public void setTransitionDelay(long transitionDelay) {
        setAttribute(TRANSITION_DELAY, transitionDelay);
    }

    /**
     * @return date and time (MM-DD HH:MM:SS.mmm) in string format parsed from events log
     *         and used in AUPT test.
     */
    public String getDateTime() {
        return (String) getAttribute(DATE_TIME);
    }

    public void setDateTime(String dateTime) {
        setAttribute(DATE_TIME, dateTime);
    }

    public Long getWindowDrawnDelay() {
        return getAttribute(WINDOW_DRAWN_DELAY) != null
                ? (Long) getAttribute(WINDOW_DRAWN_DELAY)
                : null;
    }

    public void setWindowDrawnDelay(long windowDrawnDelay) {
        setAttribute(WINDOW_DRAWN_DELAY, windowDrawnDelay);
    }
}
