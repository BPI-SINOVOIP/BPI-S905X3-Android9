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
 * An {@link IItem} used to store traces info.
 * <p>
 * This stores info about page allocation failures, i.e. order.
 * </p>
 */
public class PageAllocationFailureItem extends MiscKernelLogItem {

    /** Constant for JSON output */
    public static final String ORDER = "ORDER";

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(ORDER));

    /**
     * The constructor for {@link PageAllocationFailureItem}.
     */
    public PageAllocationFailureItem() {
        super(ATTRIBUTES);
    }

    /**
     * Get the order of the page allocation failure.
     */
    public int getOrder() {
        return (int) getAttribute(ORDER);
    }

    /**
     * Set the order of the page allocation failure.
     */
    public void setOrder(int order) {
        setAttribute(ORDER, order);
    }
}
