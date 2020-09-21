/**
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 * in compliance with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.accessibilityservice.cts.utils;

import static org.hamcrest.CoreMatchers.both;

import android.app.UiAutomation.AccessibilityEventFilter;
import android.view.accessibility.AccessibilityEvent;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.hamcrest.TypeSafeMatcher;

/**
 * Utility class for creating AccessibilityEventFilters
 */
public class AccessibilityEventFilterUtils {
    public static AccessibilityEventFilter filterForEventType(int eventType) {
        return (new AccessibilityEventTypeMatcher(eventType))::matches;
    }

    public static AccessibilityEventFilter filterWindowsChangedWithChangeTypes(int changes) {
        return (both(new AccessibilityEventTypeMatcher(AccessibilityEvent.TYPE_WINDOWS_CHANGED))
                        .and(new WindowChangesMatcher(changes)))::matches;
    }
    public static class AccessibilityEventTypeMatcher extends TypeSafeMatcher<AccessibilityEvent> {
        private int mType;

        public AccessibilityEventTypeMatcher(int type) {
            super();
            mType = type;
        }

        @Override
        protected boolean matchesSafely(AccessibilityEvent event) {
            return event.getEventType() == mType;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("Matching to type " + mType);
        }
    }

    public static class WindowChangesMatcher extends TypeSafeMatcher<AccessibilityEvent> {
        private int mWindowChanges;

        public WindowChangesMatcher(int windowChanges) {
            super();
            mWindowChanges = windowChanges;
        }

        @Override
        protected boolean matchesSafely(AccessibilityEvent event) {
            return (event.getWindowChanges() & mWindowChanges) == mWindowChanges;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("With window change type " + mWindowChanges);
        }
    }

    public static class ContentChangesMatcher extends TypeSafeMatcher<AccessibilityEvent> {
        private int mContentChanges;

        public ContentChangesMatcher(int contentChanges) {
            super();
            mContentChanges = contentChanges;
        }

        @Override
        protected boolean matchesSafely(AccessibilityEvent event) {
            return (event.getContentChangeTypes() & mContentChanges) == mContentChanges;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("With window change type " + mContentChanges);
        }
    }
}
