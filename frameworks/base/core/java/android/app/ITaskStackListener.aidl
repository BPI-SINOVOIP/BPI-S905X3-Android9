/**
 * Copyright (c) 2014, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.app;

import android.app.ActivityManager;
import android.content.ComponentName;

/** @hide */
oneway interface ITaskStackListener {
    /** Activity was resized to be displayed in split-screen. */
    const int FORCED_RESIZEABLE_REASON_SPLIT_SCREEN = 1;
    /** Activity was resized to be displayed on a secondary display. */
    const int FORCED_RESIZEABLE_REASON_SECONDARY_DISPLAY = 2;

    /** Called whenever there are changes to the state of tasks in a stack. */
    void onTaskStackChanged();

    /** Called whenever an Activity is moved to the pinned stack from another stack. */
    void onActivityPinned(String packageName, int userId, int taskId, int stackId);

    /** Called whenever an Activity is moved from the pinned stack to another stack. */
    void onActivityUnpinned();

    /**
     * Called whenever IActivityManager.startActivity is called on an activity that is already
     * running in the pinned stack and the activity is not actually started, but the task is either
     * brought to the front or a new Intent is delivered to it.
     *
     * @param clearedTask whether or not the launch activity also cleared the task as a part of
     * starting
     */
    void onPinnedActivityRestartAttempt(boolean clearedTask);

    /**
     * Called whenever the pinned stack is starting animating a resize.
     */
    void onPinnedStackAnimationStarted();

    /**
     * Called whenever the pinned stack is done animating a resize.
     */
    void onPinnedStackAnimationEnded();

    /**
     * Called when we launched an activity that we forced to be resizable.
     *
     * @param packageName Package name of the top activity in the task.
     * @param taskId Id of the task.
     * @param reason {@link #FORCED_RESIZEABLE_REASON_SPLIT_SCREEN} or
      *              {@link #FORCED_RESIZEABLE_REASON_SECONDARY_DISPLAY}.
     */
    void onActivityForcedResizable(String packageName, int taskId, int reason);

    /**
     * Called when we launched an activity that dismissed the docked stack.
     */
    void onActivityDismissingDockedStack();

    /**
     * Called when an activity was requested to be launched on a secondary display but was not
     * allowed there.
     */
    void onActivityLaunchOnSecondaryDisplayFailed();

    /**
     * Called when a task is added.
     *
     * @param taskId id of the task.
     * @param componentName of the activity that the task is being started with.
    */
    void onTaskCreated(int taskId, in ComponentName componentName);

    /**
     * Called when a task is removed.
     *
     * @param taskId id of the task.
    */
    void onTaskRemoved(int taskId);

    /**
     * Called when a task is moved to the front of its stack.
     *
     * @param taskId id of the task.
    */
    void onTaskMovedToFront(int taskId);

    /**
     * Called when a task’s description is changed due to an activity calling
     * ActivityManagerService.setTaskDescription
     *
     * @param taskId id of the task.
     * @param td the new TaskDescription.
    */
    void onTaskDescriptionChanged(int taskId, in ActivityManager.TaskDescription td);

    /**
     * Called when a activity’s orientation is changed due to it calling
     * ActivityManagerService.setRequestedOrientation
     *
     * @param taskId id of the task that the activity is in.
     * @param requestedOrientation the new requested orientation.
    */
    void onActivityRequestedOrientationChanged(int taskId, int requestedOrientation);

    /**
     * Called when the task is about to be finished but before its surfaces are
     * removed from the window manager. This allows interested parties to
     * perform relevant animations before the window disappears.
     */
    void onTaskRemovalStarted(int taskId);

    /**
     * Called when the task has been put in a locked state because one or more of the
     * activities inside it belong to a managed profile user, and that user has just
     * been locked.
     */
    void onTaskProfileLocked(int taskId, int userId);

    /**
     * Called when a task snapshot got updated.
     */
    void onTaskSnapshotChanged(int taskId, in ActivityManager.TaskSnapshot snapshot);
}
