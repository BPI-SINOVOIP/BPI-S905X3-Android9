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

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to store the output of the top command.
 */
public class TopItem extends GenericItem {

    /** Constant for JSON output */
    public static final String USER = "USER";
    /** Constant for JSON output */
    public static final String NICE = "NICE";
    /** Constant for JSON output */
    public static final String SYSTEM = "SYSTEM";
    /** Constant for JSON output */
    public static final String IDLE = "IDLE";
    /** Constant for JSON output */
    public static final String IOW = "IOW";
    /** Constant for JSON output */
    public static final String IRQ = "IRQ";
    /** Constant for JSON output */
    public static final String SIRQ = "SIRQ";
    /** Constant for JSON output */
    public static final String TOTAL = "TOTAL";
    /** Constant for JSON output */
    public static final String TEXT = "TEXT";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            USER, NICE, SYSTEM, IDLE, IOW, IRQ, SIRQ, TOTAL, TEXT));

    /**
     * The constructor for {@link TopItem}.
     */
    public TopItem() {
        super(ATTRIBUTES);

        for (String attribute : ATTRIBUTES) {
            setAttribute(attribute, 0);
        }
    }

    /**
     * Get the number of user ticks.
     */
    public int getUser() {
        return (Integer) getAttribute(USER);
    }

    /**
     * Set the number of user ticks.
     */
    public void setUser(int user) {
        setAttribute(USER, user);
    }

    /**
     * Get the number of nice ticks.
     */
    public int getNice() {
        return (Integer) getAttribute(NICE);
    }

    /**
     * Set the number of nice ticks.
     */
    public void setNice(int nice) {
        setAttribute(NICE, nice);
    }

    /**
     * Get the number of system ticks.
     */
    public int getSystem() {
        return (Integer) getAttribute(SYSTEM);
    }

    /**
     * Set the number of system ticks.
     */
    public void setSystem(int system) {
        setAttribute(SYSTEM, system);
    }

    /**
     * Get the number of idle ticks.
     */
    public int getIdle() {
        return (Integer) getAttribute(IDLE);
    }

    /**
     * Set the number of idle ticks.
     */
    public void setIdle(int idle) {
        setAttribute(IDLE, idle);
    }

    /**
     * Get the number of IOW ticks.
     */
    public int getIow() {
        return (Integer) getAttribute(IOW);
    }

    /**
     * Set the number of IOW ticks.
     */
    public void setIow(int iow) {
        setAttribute(IOW, iow);
    }

    /**
     * Get the number of IRQ ticks.
     */
    public int getIrq() {
        return (Integer) getAttribute(IRQ);
    }

    /**
     * Set the number of IRQ ticks.
     */
    public void setIrq(int irq) {
        setAttribute(IRQ, irq);
    }

    /**
     * Get the number of SIRQ ticks.
     */
    public int getSirq() {
        return (Integer) getAttribute(SIRQ);
    }

    /**
     * Set the number of SIRQ ticks.
     */
    public void setSirq(int sirq) {
        setAttribute(SIRQ, sirq);
    }

    /**
     * Get the number of total ticks.
     */
    public int getTotal() {
        return (Integer) getAttribute(TOTAL);
    }

    /**
     * Set the number of total ticks.
     */
    public void setTotal(int total) {
        setAttribute(TOTAL, total);
    }

    /**
     * Get the raw text of the top command.
     */
    public String getText() {
        return (String) getAttribute(TEXT);
    }

    /**
     * Set the raw text of the top command.
     */
    public void setText(String text) {
        setAttribute(TEXT, text);
    }
}
