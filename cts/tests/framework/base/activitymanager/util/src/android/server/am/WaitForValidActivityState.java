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
 * limitations under the License
 */

package android.server.am;

import static android.app.ActivityManager.StackId.INVALID_STACK_ID;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.app.WindowConfiguration.WINDOWING_MODE_FREEFORM;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ComponentNameUtils.getWindowName;

import android.content.ComponentName;
import androidx.annotation.Nullable;

public class WaitForValidActivityState {
    @Nullable
    public final ComponentName activityName;
    @Nullable
    public final String windowName;
    public final int stackId;
    public final int windowingMode;
    public final int activityType;

    public static WaitForValidActivityState forWindow(final String windowName) {
        return new Builder().setWindowName(windowName).build();
    }

    public WaitForValidActivityState(final ComponentName activityName) {
        this.activityName = activityName;
        this.windowName = getWindowName(activityName);
        this.stackId = INVALID_STACK_ID;
        this.windowingMode = WINDOWING_MODE_UNDEFINED;
        this.activityType = ACTIVITY_TYPE_UNDEFINED;
    }

    private WaitForValidActivityState(final Builder builder) {
        this.activityName = builder.mActivityName;
        this.windowName = builder.mWindowName;
        this.stackId = builder.mStackId;
        this.windowingMode = builder.mWindowingMode;
        this.activityType = builder.mActivityType;
    }

    @Override
    public String toString() {
        final StringBuilder sb = new StringBuilder("wait:");
        if (activityName != null) {
            sb.append(" activity=").append(getActivityName(activityName));
        }
        if (activityType != ACTIVITY_TYPE_UNDEFINED) {
            sb.append(" type=").append(activityTypeName(activityType));
        }
        if (windowName != null) {
            sb.append(" window=").append(windowName);
        }
        if (windowingMode != WINDOWING_MODE_UNDEFINED) {
            sb.append(" mode=").append(windowingModeName(windowingMode));
        }
        if (stackId != INVALID_STACK_ID) {
            sb.append(" stack=").append(stackId);
        }
        return sb.toString();
    }

    private static String windowingModeName(int windowingMode) {
        switch (windowingMode) {
            case WINDOWING_MODE_UNDEFINED: return "UNDEFINED";
            case WINDOWING_MODE_FULLSCREEN: return "FULLSCREEN";
            case WINDOWING_MODE_PINNED: return "PINNED";
            case WINDOWING_MODE_SPLIT_SCREEN_PRIMARY: return "SPLIT_SCREEN_PRIMARY";
            case WINDOWING_MODE_SPLIT_SCREEN_SECONDARY: return "SPLIT_SCREEN_SECONDARY";
            case WINDOWING_MODE_FREEFORM: return "FREEFORM";
            default:
                throw new IllegalArgumentException("Unknown WINDOWING_MODE_: " + windowingMode);
        }
    }

    private static String activityTypeName(int activityType) {
        switch (activityType) {
            case ACTIVITY_TYPE_UNDEFINED: return "UNDEFINED";
            case ACTIVITY_TYPE_STANDARD: return "STANDARD";
            case ACTIVITY_TYPE_HOME: return "HOME";
            case ACTIVITY_TYPE_RECENTS: return "RECENTS";
            case ACTIVITY_TYPE_ASSISTANT: return "ASSISTANT";
            default:
                throw new IllegalArgumentException("Unknown ACTIVITY_TYPE_: " + activityType);
        }
    }

    public static class Builder {
        @Nullable
        private ComponentName mActivityName = null;
        @Nullable
        private String mWindowName = null;
        private int mStackId = INVALID_STACK_ID;
        private int mWindowingMode = WINDOWING_MODE_UNDEFINED;
        private int mActivityType = ACTIVITY_TYPE_UNDEFINED;

        private Builder() {}

        public Builder(final ComponentName activityName) {
            mActivityName = activityName;
            mWindowName = getWindowName(activityName);
        }

        private Builder setWindowName(String windowName) {
            mWindowName = windowName;
            return this;
        }

        public Builder setStackId(int stackId) {
            mStackId = stackId;
            return this;
        }

        public Builder setWindowingMode(int windowingMode) {
            mWindowingMode = windowingMode;
            return this;
        }

        public Builder setActivityType(int activityType) {
            mActivityType = activityType;
            return this;
        }

        public WaitForValidActivityState build() {
            return new WaitForValidActivityState(this);
        }
    }
}
