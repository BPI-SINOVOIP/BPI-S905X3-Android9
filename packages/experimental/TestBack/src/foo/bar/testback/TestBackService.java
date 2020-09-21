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

import static foo.bar.testback.AccessibilityNodeInfoUtils.findChildDfs;
import static foo.bar.testback.AccessibilityNodeInfoUtils.findParent;

import static foo.bar.testback.AccessibilityFocusManager.CAN_TAKE_A11Y_FOCUS;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.Context;
import android.graphics.Rect;
import android.util.ArraySet;
import android.util.Log;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.view.accessibility.AccessibilityWindowInfo;
import android.widget.Button;

import java.util.List;
import java.util.Set;
import java.util.function.Predicate;

public class TestBackService extends AccessibilityService {

    private static final String LOG_TAG = TestBackService.class.getSimpleName();

    private Button mButton;

    int mRowIndexOfA11yFocus = -1;
    int mColIndexOfA11yFocus = -1;
    AccessibilityNodeInfo mCollectionWithAccessibiltyFocus;
    AccessibilityNodeInfo mCurrentA11yFocus;

    @Override
    public void onCreate() {
        super.onCreate();
        mButton = new Button(this);
        mButton.setText("Button 1");
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent event) {
        int eventType = event.getEventType();
        AccessibilityNodeInfo source = event.getSource();
        if (eventType == AccessibilityEvent.TYPE_VIEW_HOVER_ENTER) {
            if (source != null) {
                AccessibilityNodeInfo focusNode =
                        (CAN_TAKE_A11Y_FOCUS.test(source)) ? source : findParent(
                                source, AccessibilityFocusManager::canTakeAccessibilityFocus);
                if (focusNode != null) {
                    focusNode.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
                }
            }
            return;
        }
        if (eventType == AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUSED) {
            mCurrentA11yFocus = source;
            AccessibilityNodeInfo.CollectionItemInfo itemInfo = source.getCollectionItemInfo();
            if (itemInfo == null) {
                mRowIndexOfA11yFocus = -1;
                mColIndexOfA11yFocus = -1;
                mCollectionWithAccessibiltyFocus = null;
            } else {
                mRowIndexOfA11yFocus = itemInfo.getRowIndex();
                mColIndexOfA11yFocus = itemInfo.getColumnIndex();
                mCollectionWithAccessibiltyFocus = findParent(source,
                        (nodeInfo) -> nodeInfo.getCollectionInfo() != null);
            }
            Rect bounds = new Rect();
            source.getBoundsInScreen(bounds);
        }

        if (eventType == AccessibilityEvent.TYPE_VIEW_ACCESSIBILITY_FOCUS_CLEARED) {
            mCurrentA11yFocus = null;
            return;
        }

        if (eventType == AccessibilityEvent.TYPE_WINDOWS_CHANGED) {
            mRowIndexOfA11yFocus = -1;
            mColIndexOfA11yFocus = -1;
            mCollectionWithAccessibiltyFocus = null;
        }

        if ((mCurrentA11yFocus == null) && (mCollectionWithAccessibiltyFocus != null)) {
            // Look for a node to re-focus
            AccessibilityNodeInfo focusRestoreTarget = findChildDfs(
                    mCollectionWithAccessibiltyFocus, (nodeInfo) -> {
                        AccessibilityNodeInfo.CollectionItemInfo collectionItemInfo =
                                nodeInfo.getCollectionItemInfo();
                        return (collectionItemInfo != null)
                                && (collectionItemInfo.getRowIndex() == mRowIndexOfA11yFocus)
                                && (collectionItemInfo.getColumnIndex() == mColIndexOfA11yFocus);
                    });
            if (focusRestoreTarget != null) {
                focusRestoreTarget.performAction(AccessibilityNodeInfo.ACTION_ACCESSIBILITY_FOCUS);
            }
        }
    }

    private void dumpWindows() {
        List<AccessibilityWindowInfo> windows = getWindows();
        final int windowCount = windows.size();
        for (int i = 0; i < windowCount; i++) {
            AccessibilityWindowInfo window = windows.get(i);
            Log.i(LOG_TAG, "=============================");
            Log.i(LOG_TAG, window.toString());

        }
    }

    private void dumpWindow(AccessibilityNodeInfo source) {
        AccessibilityNodeInfo root = source;
        while (true) {
            AccessibilityNodeInfo parent = root.getParent();
            if (parent == null) {
                break;
            } else if (parent.equals(root)) {
                Log.i(LOG_TAG, "Node is own parent:" + root);
            }
            root = parent;
        }
        dumpTree(root, new ArraySet<AccessibilityNodeInfo>());
    }

    private void dumpTree(AccessibilityNodeInfo root, Set<AccessibilityNodeInfo> visited) {
        if (root == null) {
            return;
        }

        if (!visited.add(root)) {
            Log.i(LOG_TAG, "Cycle detected to node:" + root);
        }

        final int childCount = root.getChildCount();
        for (int i = 0; i < childCount; i++) {
            AccessibilityNodeInfo child = root.getChild(i);
            if (child != null) {
                AccessibilityNodeInfo parent = child.getParent();
                if (parent == null) {
                    Log.e(LOG_TAG, "Child of a node has no parent");
                } else if (!parent.equals(root)) {
                    Log.e(LOG_TAG, "Child of a node has wrong parent");
                }
                Log.i(LOG_TAG, child.toString());
            }
        }

        for (int i = 0; i < childCount; i++) {
            AccessibilityNodeInfo child = root.getChild(i);
            dumpTree(child, visited);
        }
    }

    @Override
    public void onInterrupt() {
        /* ignore */
    }

    @Override
    public void onServiceConnected() {
        AccessibilityServiceInfo info = getServiceInfo();
        info.flags |= AccessibilityServiceInfo.FLAG_REPORT_VIEW_IDS;
        info.flags |= AccessibilityServiceInfo.FLAG_REQUEST_TOUCH_EXPLORATION_MODE;
        info.flags |= AccessibilityServiceInfo.FLAG_RETRIEVE_INTERACTIVE_WINDOWS;
        setServiceInfo(info);
    }
}
