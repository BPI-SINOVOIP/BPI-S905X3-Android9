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

package com.android.server.am;

import static android.app.ActivityManager.START_TASK_TO_FRONT;
import static android.app.AppOpsManager.OP_ASSIST_STRUCTURE;
import static android.app.AppOpsManager.OP_NONE;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_HOME;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_RECENTS;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.content.Intent.FLAG_ACTIVITY_NEW_TASK;
import static android.content.Intent.FLAG_ACTIVITY_NO_ANIMATION;
import static android.os.Trace.TRACE_TAG_ACTIVITY_MANAGER;
import static android.view.WindowManager.TRANSIT_NONE;
import static com.android.server.am.ActivityStackSupervisor.PRESERVE_WINDOWS;
import static com.android.server.wm.RecentsAnimationController.REORDER_KEEP_IN_PLACE;
import static com.android.server.wm.RecentsAnimationController.REORDER_MOVE_TO_ORIGINAL_POSITION;
import static com.android.server.wm.RecentsAnimationController.REORDER_MOVE_TO_TOP;

import android.app.ActivityOptions;
import android.app.AppOpsManager;
import android.app.IAssistDataReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.RemoteException;
import android.os.Trace;
import android.util.Slog;
import android.view.IRecentsAnimationRunner;
import com.android.server.wm.RecentsAnimationController;
import com.android.server.wm.RecentsAnimationController.RecentsAnimationCallbacks;
import com.android.server.wm.WindowManagerService;

/**
 * Manages the recents animation, including the reordering of the stacks for the transition and
 * cleanup. See {@link com.android.server.wm.RecentsAnimationController}.
 */
class RecentsAnimation implements RecentsAnimationCallbacks,
        ActivityDisplay.OnStackOrderChangedListener {
    private static final String TAG = RecentsAnimation.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final ActivityManagerService mService;
    private final ActivityStackSupervisor mStackSupervisor;
    private final ActivityStartController mActivityStartController;
    private final WindowManagerService mWindowManager;
    private final UserController mUserController;
    private final ActivityDisplay mDefaultDisplay;
    private final int mCallingPid;

    private int mTargetActivityType;
    private AssistDataRequester mAssistDataRequester;

    // The stack to restore the target stack behind when the animation is finished
    private ActivityStack mRestoreTargetBehindStack;

    RecentsAnimation(ActivityManagerService am, ActivityStackSupervisor stackSupervisor,
            ActivityStartController activityStartController, WindowManagerService wm,
            UserController userController, int callingPid) {
        mService = am;
        mStackSupervisor = stackSupervisor;
        mDefaultDisplay = stackSupervisor.getDefaultDisplay();
        mActivityStartController = activityStartController;
        mWindowManager = wm;
        mUserController = userController;
        mCallingPid = callingPid;
    }

    void startRecentsActivity(Intent intent, IRecentsAnimationRunner recentsAnimationRunner,
            ComponentName recentsComponent, int recentsUid,
            IAssistDataReceiver assistDataReceiver) {
        if (DEBUG) Slog.d(TAG, "startRecentsActivity(): intent=" + intent
                + " assistDataReceiver=" + assistDataReceiver);
        Trace.traceBegin(TRACE_TAG_ACTIVITY_MANAGER, "RecentsAnimation#startRecentsActivity");

        if (!mWindowManager.canStartRecentsAnimation()) {
            notifyAnimationCancelBeforeStart(recentsAnimationRunner);
            if (DEBUG) Slog.d(TAG, "Can't start recents animation, nextAppTransition="
                        + mWindowManager.getPendingAppTransition());
            return;
        }

        // If the activity is associated with the recents stack, then try and get that first
        mTargetActivityType = intent.getComponent() != null
                && recentsComponent.equals(intent.getComponent())
                        ? ACTIVITY_TYPE_RECENTS
                        : ACTIVITY_TYPE_HOME;
        final ActivityStack targetStack = mDefaultDisplay.getStack(WINDOWING_MODE_UNDEFINED,
                mTargetActivityType);
        ActivityRecord targetActivity = getTargetActivity(targetStack, intent.getComponent());
        final boolean hasExistingActivity = targetActivity != null;
        if (hasExistingActivity) {
            final ActivityDisplay display = targetActivity.getDisplay();
            mRestoreTargetBehindStack = display.getStackAbove(targetStack);
            if (mRestoreTargetBehindStack == null) {
                notifyAnimationCancelBeforeStart(recentsAnimationRunner);
                if (DEBUG) Slog.d(TAG, "No stack above target stack=" + targetStack);
                return;
            }
        }

        // Send launch hint if we are actually launching the target. If it's already visible
        // (shouldn't happen in general) we don't need to send it.
        if (targetActivity == null || !targetActivity.visible) {
            mStackSupervisor.sendPowerHintForLaunchStartIfNeeded(true /* forceSend */,
                    targetActivity);
        }

        mStackSupervisor.getActivityMetricsLogger().notifyActivityLaunching();

        mService.setRunningRemoteAnimation(mCallingPid, true);

        mWindowManager.deferSurfaceLayout();
        try {
            // Kick off the assist data request in the background before showing the target activity
            if (assistDataReceiver != null) {
                final AppOpsManager appOpsManager = (AppOpsManager)
                        mService.mContext.getSystemService(Context.APP_OPS_SERVICE);
                final AssistDataReceiverProxy proxy = new AssistDataReceiverProxy(
                        assistDataReceiver, recentsComponent.getPackageName());
                mAssistDataRequester = new AssistDataRequester(mService.mContext, mService,
                        mWindowManager, appOpsManager, proxy, this, OP_ASSIST_STRUCTURE, OP_NONE);
                mAssistDataRequester.requestAssistData(mStackSupervisor.getTopVisibleActivities(),
                        true /* fetchData */, false /* fetchScreenshots */,
                        true /* allowFetchData */, false /* allowFetchScreenshots */,
                        recentsUid, recentsComponent.getPackageName());
            }

            if (hasExistingActivity) {
                // Move the recents activity into place for the animation if it is not top most
                mDefaultDisplay.moveStackBehindBottomMostVisibleStack(targetStack);
                if (DEBUG) Slog.d(TAG, "Moved stack=" + targetStack + " behind stack="
                            + mDefaultDisplay.getStackAbove(targetStack));

                // If there are multiple tasks in the target stack (ie. the home stack, with 3p
                // and default launchers coexisting), then move the task to the top as a part of
                // moving the stack to the front
                if (targetStack.topTask() != targetActivity.getTask()) {
                    targetStack.addTask(targetActivity.getTask(), true /* toTop */,
                            "startRecentsActivity");
                }
            } else {
                // No recents activity
                ActivityOptions options = ActivityOptions.makeBasic();
                options.setLaunchActivityType(mTargetActivityType);
                options.setAvoidMoveToFront();
                intent.addFlags(FLAG_ACTIVITY_NEW_TASK | FLAG_ACTIVITY_NO_ANIMATION);

                mActivityStartController
                        .obtainStarter(intent, "startRecentsActivity_noTargetActivity")
                        .setCallingUid(recentsUid)
                        .setCallingPackage(recentsComponent.getPackageName())
                        .setActivityOptions(SafeActivityOptions.fromBundle(options.toBundle()))
                        .setMayWait(mUserController.getCurrentUserId())
                        .execute();
                mWindowManager.prepareAppTransition(TRANSIT_NONE, false);
                mWindowManager.executeAppTransition();

                targetActivity = mDefaultDisplay.getStack(WINDOWING_MODE_UNDEFINED,
                        mTargetActivityType).getTopActivity();

                // TODO: Maybe wait for app to draw in this particular case?

                if (DEBUG) Slog.d(TAG, "Started intent=" + intent);
            }

            // Mark the target activity as launch-behind to bump its visibility for the
            // duration of the gesture that is driven by the recents component
            targetActivity.mLaunchTaskBehind = true;

            // Fetch all the surface controls and pass them to the client to get the animation
            // started. Cancel any existing recents animation running synchronously (do not hold the
            // WM lock)
            mWindowManager.cancelRecentsAnimationSynchronously(REORDER_MOVE_TO_ORIGINAL_POSITION,
                    "startRecentsActivity");
            mWindowManager.initializeRecentsAnimation(mTargetActivityType, recentsAnimationRunner,
                    this, mDefaultDisplay.mDisplayId,
                    mStackSupervisor.mRecentTasks.getRecentTaskIds());

            // If we updated the launch-behind state, update the visibility of the activities after
            // we fetch the visible tasks to be controlled by the animation
            mStackSupervisor.ensureActivitiesVisibleLocked(null, 0, PRESERVE_WINDOWS);

            mStackSupervisor.getActivityMetricsLogger().notifyActivityLaunched(START_TASK_TO_FRONT,
                    targetActivity);

            // Register for stack order changes
            mDefaultDisplay.registerStackOrderChangedListener(this);
        } catch (Exception e) {
            Slog.e(TAG, "Failed to start recents activity", e);
            throw e;
        } finally {
            mWindowManager.continueSurfaceLayout();
            Trace.traceEnd(TRACE_TAG_ACTIVITY_MANAGER);
        }
    }

    private void finishAnimation(@RecentsAnimationController.ReorderMode int reorderMode) {
        synchronized (mService) {
            if (DEBUG) Slog.d(TAG, "onAnimationFinished(): controller="
                    + mWindowManager.getRecentsAnimationController()
                    + " reorderMode=" + reorderMode);

            // Cancel the associated assistant data request
            if (mAssistDataRequester != null) {
                mAssistDataRequester.cancel();
                mAssistDataRequester = null;
            }

            // Unregister for stack order changes
            mDefaultDisplay.unregisterStackOrderChangedListener(this);

            if (mWindowManager.getRecentsAnimationController() == null) return;

            // Just to be sure end the launch hint in case the target activity was never launched.
            // However, if we're keeping the activity and making it visible, we can leave it on.
            if (reorderMode != REORDER_KEEP_IN_PLACE) {
                mStackSupervisor.sendPowerHintForLaunchEndIfNeeded();
            }

            mService.setRunningRemoteAnimation(mCallingPid, false);

            mWindowManager.inSurfaceTransaction(() -> {
                Trace.traceBegin(TRACE_TAG_ACTIVITY_MANAGER,
                        "RecentsAnimation#onAnimationFinished_inSurfaceTransaction");
                mWindowManager.deferSurfaceLayout();
                try {
                    mWindowManager.cleanupRecentsAnimation(reorderMode);

                    final ActivityStack targetStack = mDefaultDisplay.getStack(
                            WINDOWING_MODE_UNDEFINED, mTargetActivityType);
                    final ActivityRecord targetActivity = targetStack != null
                            ? targetStack.getTopActivity()
                            : null;
                    if (DEBUG) Slog.d(TAG, "onAnimationFinished(): targetStack=" + targetStack
                            + " targetActivity=" + targetActivity
                            + " mRestoreTargetBehindStack=" + mRestoreTargetBehindStack);
                    if (targetActivity == null) {
                        return;
                    }

                    // Restore the launched-behind state
                    targetActivity.mLaunchTaskBehind = false;

                    if (reorderMode == REORDER_MOVE_TO_TOP) {
                        // Bring the target stack to the front
                        mStackSupervisor.mNoAnimActivities.add(targetActivity);
                        targetStack.moveToFront("RecentsAnimation.onAnimationFinished()");
                        if (DEBUG) {
                            final ActivityStack topStack = getTopNonAlwaysOnTopStack();
                            if (topStack != targetStack) {
                                Slog.w(TAG, "Expected target stack=" + targetStack
                                        + " to be top most but found stack=" + topStack);
                            }
                        }
                    } else if (reorderMode == REORDER_MOVE_TO_ORIGINAL_POSITION){
                        // Restore the target stack to its previous position
                        final ActivityDisplay display = targetActivity.getDisplay();
                        display.moveStackBehindStack(targetStack, mRestoreTargetBehindStack);
                        if (DEBUG) {
                            final ActivityStack aboveTargetStack =
                                    mDefaultDisplay.getStackAbove(targetStack);
                            if (mRestoreTargetBehindStack != null
                                    && aboveTargetStack != mRestoreTargetBehindStack) {
                                Slog.w(TAG, "Expected target stack=" + targetStack
                                        + " to restored behind stack=" + mRestoreTargetBehindStack
                                        + " but it is behind stack=" + aboveTargetStack);
                            }
                        }
                    } else {
                        // Keep target stack in place, nothing changes, so ignore the transition
                        // logic below
                        return;
                    }

                    mWindowManager.prepareAppTransition(TRANSIT_NONE, false);
                    mStackSupervisor.ensureActivitiesVisibleLocked(null, 0, false);
                    mStackSupervisor.resumeFocusedStackTopActivityLocked();

                    // No reason to wait for the pausing activity in this case, as the hiding of
                    // surfaces needs to be done immediately.
                    mWindowManager.executeAppTransition();

                    // After reordering the stacks, reset the minimized state. At this point, either
                    // the target activity is now top-most and we will stay minimized (if in
                    // split-screen), or we will have returned to the app, and the minimized state
                    // should be reset
                    mWindowManager.checkSplitScreenMinimizedChanged(true /* animate */);
                } catch (Exception e) {
                    Slog.e(TAG, "Failed to clean up recents activity", e);
                    throw e;
                } finally {
                    mWindowManager.continueSurfaceLayout();
                    Trace.traceEnd(TRACE_TAG_ACTIVITY_MANAGER);
                }
            });
        }
    }

    @Override
    public void onAnimationFinished(@RecentsAnimationController.ReorderMode int reorderMode,
            boolean runSychronously) {
        if (runSychronously) {
            finishAnimation(reorderMode);
        } else {
            mService.mHandler.post(() -> finishAnimation(reorderMode));
        }
    }

    @Override
    public void onStackOrderChanged() {
        // If the activity display stack order changes, cancel any running recents animation in
        // place
        mWindowManager.cancelRecentsAnimationSynchronously(REORDER_KEEP_IN_PLACE,
                "stackOrderChanged");
    }

    /**
     * Called only when the animation should be canceled prior to starting.
     */
    private void notifyAnimationCancelBeforeStart(IRecentsAnimationRunner recentsAnimationRunner) {
        try {
            recentsAnimationRunner.onAnimationCanceled();
        } catch (RemoteException e) {
            Slog.e(TAG, "Failed to cancel recents animation before start", e);
        }
    }

    /**
     * @return The top stack that is not always-on-top.
     */
    private ActivityStack getTopNonAlwaysOnTopStack() {
        for (int i = mDefaultDisplay.getChildCount() - 1; i >= 0; i--) {
            final ActivityStack s = mDefaultDisplay.getChildAt(i);
            if (s.getWindowConfiguration().isAlwaysOnTop()) {
                continue;
            }
            return s;
        }
        return null;
    }

    /**
     * @return the top activity in the {@param targetStack} matching the {@param component}, or just
     * the top activity of the top task if no task matches the component.
     */
    private ActivityRecord getTargetActivity(ActivityStack targetStack, ComponentName component) {
        if (targetStack == null) {
            return null;
        }

        for (int i = targetStack.getChildCount() - 1; i >= 0; i--) {
            final TaskRecord task = (TaskRecord) targetStack.getChildAt(i);
            if (task.getBaseIntent().getComponent().equals(component)) {
                return task.getTopActivity();
            }
        }
        return targetStack.getTopActivity();
    }
}
