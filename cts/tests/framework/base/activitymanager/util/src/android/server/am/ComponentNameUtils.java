/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.server.am;

import android.content.ComponentName;

public class ComponentNameUtils {

    /**
     * Get component's activity name.
     *
     * @return the activity name of {@code componentName}, such as "contextPackage/.SimpleClassName"
     *     or "contextPackage/FullyClassName".
     */
    public static String getActivityName(ComponentName componentName) {
        return componentName.flattenToShortString();
    }

    /**
     * Get component's window mane.
     *
     * @return the window name of {@code componentName}, such that "contextPackage/FullyClassName".
     */
    public static String getWindowName(ComponentName componentName) {
        return componentName.flattenToString();
    }

    /**
     * Get component's simple class name suitable for logging tag.
     *
     * @return the simple class name of {@code componentName} that has no '.'.
     */
    public static String getLogTag(ComponentName componentName) {
        final String className = componentName.getClassName();
        final int pos = className.lastIndexOf('.');
        return pos >= 0 ? className.substring(pos + 1) : className;
    }
}
