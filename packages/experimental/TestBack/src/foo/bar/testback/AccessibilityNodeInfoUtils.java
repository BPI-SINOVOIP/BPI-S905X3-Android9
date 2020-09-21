/*
 * Copyright 2017 The Android Open Source Project
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

package foo.bar.testback;

import android.view.accessibility.AccessibilityNodeInfo;

import java.util.function.Predicate;

/**
 * Utility class for working with AccessibilityNodeInfo
 */
public class AccessibilityNodeInfoUtils {
    public static AccessibilityNodeInfo findParent(
            AccessibilityNodeInfo start, Predicate<AccessibilityNodeInfo> condition) {
        AccessibilityNodeInfo parent = start.getParent();
        if ((parent == null) || (condition.test(parent))) {
            return parent;
        }

        return findParent(parent, condition);
    }

    public static AccessibilityNodeInfo findChildDfs(
            AccessibilityNodeInfo start, Predicate<AccessibilityNodeInfo> condition) {
        int numChildren = start.getChildCount();
        for (int i = 0; i < numChildren; i++) {
            AccessibilityNodeInfo child = start.getChild(i);
            if (child != null) {
                if (condition.test(child)) {
                    return child;
                }
                AccessibilityNodeInfo childResult = findChildDfs(child, condition);
                if (childResult != null) {
                    return childResult;
                }
            }
        }
        return null;
    }
}
