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
 * limitations under the License
 */

package android.server.am;

import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.ProtoExtractors.extract;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logE;

import android.content.ComponentName;
import android.graphics.Rect;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;

import com.android.server.am.nano.ActivityDisplayProto;
import com.android.server.am.nano.ActivityManagerServiceDumpActivitiesProto;
import com.android.server.am.nano.ActivityRecordProto;
import com.android.server.am.nano.ActivityStackProto;
import com.android.server.am.nano.ActivityStackSupervisorProto;
import com.android.server.am.nano.KeyguardControllerProto;
import com.android.server.am.nano.TaskRecordProto;
import com.android.server.wm.nano.ConfigurationContainerProto;

import com.google.protobuf.nano.InvalidProtocolBufferNanoException;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class ActivityManagerState {

    public static final int DUMP_MODE_ACTIVITIES = 0;

    public static final String STATE_PAUSED = "PAUSED";
    public static final String STATE_STOPPED = "STOPPED";

    private static final String DUMPSYS_ACTIVITY_ACTIVITIES = "dumpsys activity --proto activities";

    // Displays in z-order with the top most at the front of the list, starting with primary.
    private final List<ActivityDisplay> mDisplays = new ArrayList<>();
    // Stacks in z-order with the top most at the front of the list, starting with primary display.
    private final List<ActivityStack> mStacks = new ArrayList<>();
    private KeyguardControllerState mKeyguardControllerState;
    private int mFocusedStackId = -1;
    private String mResumedActivityRecord = null;
    private final List<String> mResumedActivities = new ArrayList<>();

    void computeState() {
        computeState(DUMP_MODE_ACTIVITIES);
    }

    void computeState(int dumpMode) {
        // It is possible the system is in the middle of transition to the right state when we get
        // the dump. We try a few times to get the information we need before giving up.
        int retriesLeft = 3;
        boolean retry = false;
        byte[] dump = null;

        log("==============================");
        log("     ActivityManagerState     ");
        log("==============================");

        do {
            if (retry) {
                log("***Incomplete AM state. Retrying...");
                // Wait half a second between retries for activity manager to finish transitioning.
                SystemClock.sleep(500);
            }

            String dumpsysCmd = "";
            switch (dumpMode) {
                case DUMP_MODE_ACTIVITIES:
                    dumpsysCmd = DUMPSYS_ACTIVITY_ACTIVITIES;
                    break;
            }

            dump = executeShellCommand(dumpsysCmd);
            try {
                parseSysDumpProto(dump);
            } catch (InvalidProtocolBufferNanoException ex) {
                throw new RuntimeException("Failed to parse dumpsys:\n"
                        + new String(dump, StandardCharsets.UTF_8), ex);
            }

            retry = mStacks.isEmpty() || mFocusedStackId == -1 || (mResumedActivityRecord == null
                    || mResumedActivities.isEmpty()) && !mKeyguardControllerState.keyguardShowing;
        } while (retry && retriesLeft-- > 0);

        if (mStacks.isEmpty()) {
            logE("No stacks found...");
        }
        if (mFocusedStackId == -1) {
            logE("No focused stack found...");
        }
        if (mResumedActivityRecord == null) {
            logE("No focused activity found...");
        }
        if (mResumedActivities.isEmpty()) {
            logE("No resumed activities found...");
        }
    }

    private byte[] executeShellCommand(String cmd) {
        try {
            ParcelFileDescriptor pfd =
                    InstrumentationRegistry.getInstrumentation().getUiAutomation()
                            .executeShellCommand(cmd);
            byte[] buf = new byte[512];
            int bytesRead;
            FileInputStream fis = new ParcelFileDescriptor.AutoCloseInputStream(pfd);
            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            while ((bytesRead = fis.read(buf)) != -1) {
                stdout.write(buf, 0, bytesRead);
            }
            fis.close();
            return stdout.toByteArray();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private void parseSysDumpProto(byte[] sysDump) throws InvalidProtocolBufferNanoException {
        reset();

        ActivityStackSupervisorProto state = ActivityManagerServiceDumpActivitiesProto.parseFrom(sysDump)
                .activityStackSupervisor;
        for (int i = 0; i < state.displays.length; i++) {
            ActivityDisplayProto activityDisplay = state.displays[i];
            mDisplays.add(new ActivityDisplay(activityDisplay, this));
        }
        mKeyguardControllerState = new KeyguardControllerState(state.keyguardController);
        mFocusedStackId = state.focusedStackId;
        if (state.resumedActivity != null) {
            mResumedActivityRecord = state.resumedActivity.title;
        }
    }

    private void reset() {
        mDisplays.clear();
        mStacks.clear();
        mFocusedStackId = -1;
        mResumedActivityRecord = null;
        mResumedActivities.clear();
        mKeyguardControllerState = null;
    }

    int getFocusedStackId() {
        return mFocusedStackId;
    }

    String getFocusedActivity() {
        return mResumedActivityRecord;
    }

    int getResumedActivitiesCount() {
        return mResumedActivities.size();
    }

    public KeyguardControllerState getKeyguardControllerState() {
        return mKeyguardControllerState;
    }

    ActivityStack getStackByActivityType(int activityType) {
        for (ActivityStack stack : mStacks) {
            if (activityType == stack.getActivityType()) {
                return stack;
            }
        }
        return null;
    }

    List<ActivityDisplay> getDisplays() {
        return new ArrayList<>(mDisplays);
    }

    List<ActivityStack> getStacks() {
        return new ArrayList<>(mStacks);
    }

    int getStackCount() {
        return mStacks.size();
    }

    boolean isActivityVisible(ComponentName activityName) {
        final String fullName = getActivityName(activityName);
        for (ActivityStack stack : mStacks) {
            for (ActivityTask task : stack.mTasks) {
                for (Activity activity : task.mActivities) {
                    if (activity.name.equals(fullName)) {
                        return activity.visible;
                    }
                }
            }
        }
        return false;
    }

    boolean isHomeActivityVisible() {
        final Activity homeActivity = getHomeActivity();
        return homeActivity != null && homeActivity.visible;
    }

    ComponentName getHomeActivityName() {
        Activity activity = getHomeActivity();
        if (activity == null) {
            return null;
        }
        return ComponentName.unflattenFromString(activity.name);
    }

    ActivityTask getHomeTask() {
        final ActivityStack homeStack = getStackByActivityType(ACTIVITY_TYPE_HOME);
        if (homeStack != null && !homeStack.mTasks.isEmpty()) {
            return homeStack.mTasks.get(0);
        }
        return null;
    }

    private Activity getHomeActivity() {
        final ActivityTask homeTask = getHomeTask();
        return homeTask != null ? homeTask.mActivities.get(homeTask.mActivities.size() - 1) : null;
    }

    static class ActivityDisplay extends ActivityContainer {

        int mId;
        ArrayList<ActivityStack> mStacks = new ArrayList<>();

        ActivityDisplay(ActivityDisplayProto proto, ActivityManagerState amState) {
            super(proto.configurationContainer);
            mId = proto.id;
            for (int i = 0; i < proto.stacks.length; i++) {
                ActivityStack activityStack = new ActivityStack(proto.stacks[i]);
                mStacks.add(activityStack);
                // Also update activity manager state
                amState.mStacks.add(activityStack);
                if (activityStack.mResumedActivity != null) {
                    amState.mResumedActivities.add(activityStack.mResumedActivity);
                }
            }
        }
    }

    static class ActivityStack extends ActivityContainer {

        int mDisplayId;
        int mStackId;
        String mResumedActivity;
        ArrayList<ActivityTask> mTasks = new ArrayList<>();

        ActivityStack(ActivityStackProto proto) {
            super(proto.configurationContainer);
            mStackId = proto.id;
            mDisplayId = proto.displayId;
            mBounds = extract(proto.bounds);
            mFullscreen = proto.fullscreen;
            for (int i = 0; i < proto.tasks.length; i++) {
                mTasks.add(new ActivityTask(proto.tasks[i]));
            }
            if (proto.resumedActivity != null) {
                mResumedActivity = proto.resumedActivity.title;
            }
        }

        List<ActivityTask> getTasks() {
            return new ArrayList<>(mTasks);
        }

        ActivityTask getTask(int taskId) {
            for (ActivityTask task : mTasks) {
                if (taskId == task.mTaskId) {
                    return task;
                }
            }
            return null;
        }
    }

    static class ActivityTask extends ActivityContainer {

        int mTaskId;
        int mStackId;
        Rect mLastNonFullscreenBounds;
        String mRealActivity;
        String mOrigActivity;
        ArrayList<Activity> mActivities = new ArrayList<>();
        int mTaskType = -1;
        private int mResizeMode;

        ActivityTask(TaskRecordProto proto) {
            super(proto.configurationContainer);
            mTaskId = proto.id;
            mStackId = proto.stackId;
            mLastNonFullscreenBounds = extract(proto.lastNonFullscreenBounds);
            mRealActivity = proto.realActivity;
            mOrigActivity = proto.origActivity;
            mTaskType = proto.activityType;
            mResizeMode = proto.resizeMode;
            mFullscreen = proto.fullscreen;
            mBounds = extract(proto.bounds);
            mMinWidth = proto.minWidth;
            mMinHeight = proto.minHeight;
            for (int i = 0;  i < proto.activities.length; i++) {
                mActivities.add(new Activity(proto.activities[i]));
            }
        }

        public int getResizeMode() {
            return mResizeMode;
        }

    }

    static class Activity extends ActivityContainer {

        String name;
        String state;
        boolean visible;
        boolean frontOfTask;
        int procId = -1;

        Activity(ActivityRecordProto proto) {
            super(proto.configurationContainer);
            name = proto.identifier.title;
            state = proto.state;
            visible = proto.visible;
            frontOfTask = proto.frontOfTask;
            if (proto.procId != 0) {
                procId = proto.procId;
            }
        }
    }

    static abstract class ActivityContainer extends WindowManagerState.ConfigurationContainer {
        protected boolean mFullscreen;
        protected Rect mBounds;
        protected int mMinWidth = -1;
        protected int mMinHeight = -1;

        ActivityContainer(ConfigurationContainerProto proto) {
            super(proto);
        }

        Rect getBounds() {
            return mBounds;
        }

        boolean isFullscreen() {
            return mFullscreen;
        }

        int getMinWidth() {
            return mMinWidth;
        }

        int getMinHeight() {
            return mMinHeight;
        }
    }

    static class KeyguardControllerState {

        boolean keyguardShowing = false;
        boolean keyguardOccluded = false;

        KeyguardControllerState(KeyguardControllerProto proto) {
            if (proto != null) {
                keyguardShowing = proto.keyguardShowing;
                keyguardOccluded = proto.keyguardOccluded;
            }
        }
    }
}
