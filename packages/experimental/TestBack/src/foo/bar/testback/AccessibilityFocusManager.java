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

import static foo.bar.testback.AccessibilityNodeInfoUtils.findParent;

import android.text.TextUtils;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityNodeInfo.AccessibilityAction;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.function.Predicate;

/**
 * Helper class to manage accessibility focus
 */
public class AccessibilityFocusManager {
    private static final AccessibilityAction[] IGNORED_ACTIONS_FOR_A11Y_FOCUS = {
            AccessibilityAction.ACTION_ACCESSIBILITY_FOCUS,
            AccessibilityAction.ACTION_CLEAR_ACCESSIBILITY_FOCUS,
            AccessibilityAction.ACTION_SELECT,
            AccessibilityAction.ACTION_CLEAR_SELECTION,
            AccessibilityAction.ACTION_SHOW_ON_SCREEN
    };

    public static final boolean canTakeAccessibilityFocus(AccessibilityNodeInfo nodeInfo) {
        if (nodeInfo.isFocusable() || nodeInfo.isScreenReaderFocusable()) {
            return true;
        }

        List<AccessibilityAction> actions = new ArrayList<>(nodeInfo.getActionList());
        actions.removeAll(Arrays.asList(IGNORED_ACTIONS_FOR_A11Y_FOCUS));

        // Nodes with relevant actions are always focusable
        if (!actions.isEmpty()) {
            return true;
        }

        // If a parent is specifically marked focusable, then this node should not be.
        if (findParent(nodeInfo,
                (parent) -> parent.isFocusable() || parent.isScreenReaderFocusable()) != null) {
            return false;
        }

        return !TextUtils.isEmpty(nodeInfo.getText())
                || !TextUtils.isEmpty(nodeInfo.getContentDescription());
    };
}
