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

package com.android.services.telephony;

/** The inference used to track the hold state of a holdable object. */
public interface Holdable {

    /** Returns true if this holdable is a child node of other holdable. */
    boolean isChildHoldable();

    /**
     * Sets the holdable property for a holdable object.
     *
     * @param isHoldable true means this holdable object can be held.
     */
    void setHoldable(boolean isHoldable);
}

