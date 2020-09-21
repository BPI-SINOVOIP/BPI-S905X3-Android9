package android.server.am.lifecycle;

import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.server.am.ActivityManagerState.STATE_PAUSED;
import static android.server.am.ActivityManagerState.STATE_STOPPED;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_ACTIVITY_RESULT;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_CREATE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_DESTROY;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback
        .ON_MULTI_WINDOW_MODE_CHANGED;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_NEW_INTENT;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_PAUSE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_POST_CREATE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_RESTART;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_RESUME;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_START;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.PRE_ON_CREATE;
import static android.view.Surface.ROTATION_0;
import static android.view.Surface.ROTATION_180;
import static android.view.Surface.ROTATION_270;
import static android.view.Surface.ROTATION_90;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.app.Instrumentation.ActivityMonitor;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.platform.test.annotations.Presubmit;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.FlakyTest;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import com.android.compatibility.common.util.AmUtils;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.List;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityLifecycleTests
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
@Presubmit
@FlakyTest(bugId = 77652261)
public class ActivityLifecycleTests extends ActivityLifecycleClientTestBase {

    @Test
    public void testSingleLaunch() throws Exception {
        final Activity activity = mFirstActivityTestRule.launchActivity(new Intent());
        waitAndAssertActivityStates(state(activity, ON_RESUME));

        LifecycleVerifier.assertLaunchSequence(FirstActivity.class, getLifecycleLog(),
                false /* includeCallbacks */);
    }

    @Test
    public void testLaunchOnTop() throws Exception {
        final Activity firstActivity = mFirstActivityTestRule.launchActivity(new Intent());
        waitAndAssertActivityStates(state(firstActivity, ON_RESUME));

        getLifecycleLog().clear();
        final Activity secondActivity = mSecondActivityTestRule.launchActivity(new Intent());
        waitAndAssertActivityStates(occludedActivityState(firstActivity, secondActivity),
                state(secondActivity, ON_RESUME));

        LifecycleVerifier.assertLaunchSequence(SecondActivity.class, FirstActivity.class,
                getLifecycleLog(), isTranslucent(secondActivity));
    }

    @Test
    public void testLaunchAndDestroy() throws Exception {
        final Activity activity = mFirstActivityTestRule.launchActivity(new Intent());

        activity.finish();
        waitAndAssertActivityStates(state(activity, ON_DESTROY));

        LifecycleVerifier.assertLaunchAndDestroySequence(FirstActivity.class, getLifecycleLog());
    }

    @Test
    public void testRelaunchResumed() throws Exception {
        final Activity activity = mFirstActivityTestRule.launchActivity(new Intent());
        waitAndAssertActivityStates(state(activity, ON_RESUME));

        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(activity::recreate);
        waitAndAssertActivityStates(state(activity, ON_RESUME));

        LifecycleVerifier.assertRelaunchSequence(FirstActivity.class, getLifecycleLog(), ON_RESUME);
    }

    @Test
    public void testRelaunchPaused() throws Exception {
        final Activity pausedActivity = mFirstActivityTestRule.launchActivity(new Intent());
        final Activity topTranslucentActivity =
                mTranslucentActivityTestRule.launchActivity(new Intent());

        waitAndAssertActivityStates(state(pausedActivity, ON_PAUSE),
                state(topTranslucentActivity, ON_RESUME));

        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(pausedActivity::recreate);
        waitAndAssertActivityStates(state(pausedActivity, ON_PAUSE));

        LifecycleVerifier.assertRelaunchSequence(FirstActivity.class, getLifecycleLog(), ON_PAUSE);
    }

    @Test
    public void testRelaunchStopped() throws Exception {
        final Activity stoppedActivity = mFirstActivityTestRule.launchActivity(new Intent());
        final Activity topActivity = mSecondActivityTestRule.launchActivity(new Intent());

        waitAndAssertActivityStates(
                occludedActivityState(stoppedActivity, topActivity), state(topActivity, ON_RESUME));

        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(stoppedActivity::recreate);
        waitAndAssertActivityStates(occludedActivityState(stoppedActivity, topActivity));

        LifecycleVerifier.assertRelaunchSequence(FirstActivity.class, getLifecycleLog(),
                occludedActivityState(isTranslucent(topActivity)));
    }

    @Test
    public void testRelaunchConfigurationChangedWhileBecomingVisible() throws Exception {
        if (!supportsRotation()) {
            // Skip rotation test if device doesn't support it.
            return;
        }

        final Activity becomingVisibleActivity =
                mFirstActivityTestRule.launchActivity(new Intent());
        final Activity translucentActivity =
                mTranslucentActivityTestRule.launchActivity(new Intent());
        final Activity topOpaqueActivity = mSecondActivityTestRule.launchActivity(new Intent());

        waitAndAssertActivityStates(
                occludedActivityState(becomingVisibleActivity, topOpaqueActivity),
                occludedActivityState(translucentActivity, topOpaqueActivity),
                state(topOpaqueActivity, ON_RESUME));

        getLifecycleLog().clear();
        try (final RotationSession rotationSession = new RotationSession()) {
            final int current = rotationSession.get();
            // Set new rotation to cause a configuration change.
            switch (current) {
                case ROTATION_0:
                case ROTATION_180:
                    rotationSession.set(ROTATION_90);
                    break;
                case ROTATION_90:
                case ROTATION_270:
                    rotationSession.set(ROTATION_0);
                    break;
                default:
                    fail("Unknown rotation:" + current);
            }

            // Assert that the top activity was relaunched.
            waitAndAssertActivityStates(state(topOpaqueActivity, ON_RESUME));
            LifecycleVerifier.assertRelaunchSequence(
                    SecondActivity.class, getLifecycleLog(), ON_RESUME);

            // Finish the top activity
            getLifecycleLog().clear();
            mSecondActivityTestRule.finishActivity();

            // Assert that the translucent activity and the activity visible behind it were
            // relaunched.
            waitAndAssertActivityStates(state(becomingVisibleActivity, ON_PAUSE),
                    state(translucentActivity, ON_RESUME));

            LifecycleVerifier.assertSequence(FirstActivity.class, getLifecycleLog(),
                    Arrays.asList(ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME,
                            ON_PAUSE), "becomingVisiblePaused");
            // TODO(b/77974794): New intent handling sequence should always be the same.
            // It is possible to get an extra pause and resume now.
            final List<LifecycleLog.ActivityCallback> expectedSequence =
                    Arrays.asList(ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME);
            final List<LifecycleLog.ActivityCallback> extraCycleSequence =
                    Arrays.asList(ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME,
                            ON_PAUSE, ON_RESUME);
            LifecycleVerifier.assertSequenceMatchesOneOf(TranslucentActivity.class,
                    getLifecycleLog(),
                    Arrays.asList(expectedSequence, extraCycleSequence),
                    "becomingVisibleResumed");
        }
    }

    @Test
    public void testPausedWithTranslucentOnTop() throws Exception {
        // Launch fullscreen activity
        final Activity firstActivity =
                mFirstActivityTestRule.launchActivity(new Intent());

        // Launch translucent activity on top
        mTranslucentActivityTestRule.launchActivity(new Intent());

        // Launch another translucent activity on top to make sure the fullscreen activity
        // transitions to final state
        final Activity secondTranslucentActivity =
                mSecondTranslucentActivityTestRule.launchActivity(new Intent());

        // Wait for the second translucent activity to become resumed.
        waitAndAssertActivityStates(state(secondTranslucentActivity, ON_RESUME),
                state(firstActivity, ON_PAUSE));

        // Assert that the fullscreen activity was not stopped and is in the paused state.
        LifecycleVerifier.assertLaunchAndPauseSequence(FirstActivity.class, getLifecycleLog());
    }

    @Test
    public void testPausedWhenReturningWithTranslucentOnTop() throws Exception {
        // Launch fullscreen activity
        final Activity firstActivity =
                mFirstActivityTestRule.launchActivity(new Intent());

        // Launch translucent activity
        final Activity translucentActivity =
                mTranslucentActivityTestRule.launchActivity(new Intent());

        // Launch another fullscreen activity
        final Activity secondActivity =
                mSecondActivityTestRule.launchActivity(new Intent());

        // Wait for top activity to resume
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME),
                occludedActivityState(translucentActivity, secondActivity),
                occludedActivityState(firstActivity, secondActivity));

        getLifecycleLog().clear();

        final boolean secondActivityIsTranslucent = ActivityInfo.isTranslucentOrFloating(
                secondActivity.getWindow().getWindowStyle());

        // Finish top activity
        mSecondActivityTestRule.finishActivity();

        if (secondActivityIsTranslucent) {
            // In this case we don't expect the state of the firstActivity to change since it is
            // already in the visible paused state. So, we just verify that translucentActivity
            // transitions to resumed state.
            waitAndAssertActivityStates(state(translucentActivity, ON_RESUME));
        } else {
            // Wait for translucent activity to resume
            waitAndAssertActivityStates(state(translucentActivity, ON_RESUME),
                    state(firstActivity, ON_START));

            // Verify that the first activity was restarted
            LifecycleVerifier.assertRestartSequence(FirstActivity.class, getLifecycleLog());
        }
    }

    @Test
    public void testPausedWhenRecreatedFromInNonFocusedStack() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no split multi-window support
            return;
        }

        // Launch first activity
        final Activity firstActivity =
                mFirstActivityTestRule.launchActivity(new Intent());

        // Launch second activity to stop first
        final Activity secondActivity =
                mSecondActivityTestRule.launchActivity(new Intent());

        // Wait for second activity to resume. We must also wait for the first activity to stop
        // so that this event is not included in the logs.
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME),
                state(firstActivity, ON_STOP));

        // Enter split screen
        moveTaskToPrimarySplitScreen(secondActivity.getTaskId());

        // CLear logs so we can capture just the destroy sequence
        getLifecycleLog().clear();

        // Start an activity in separate task (will be placed in secondary stack)
        getLaunchActivityBuilder().execute();

        // Finish top activity
        secondActivity.finish();

        waitAndAssertActivityStates(state(firstActivity, ON_PAUSE));

        // Verify that the first activity was recreated to pause as it was created before
        // windowing mode was switched
        LifecycleVerifier.assertRecreateAndPauseSequence(FirstActivity.class, getLifecycleLog());
    }

    @Test
    public void testResultInNonFocusedStack() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no split multi-window support
            return;
        }

        // Launch first activity
        final Activity callbackTrackingActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        // Wait for first activity to resume
        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_RESUME));

        // Enter split screen
        moveTaskToPrimarySplitScreen(callbackTrackingActivity.getTaskId(),
                true /* launchSideActivityIfNeeded */);

        // Launch second activity to pause first
        // Create an ActivityMonitor that catch ChildActivity and return mock ActivityResult:
        ActivityMonitor activityMonitor = InstrumentationRegistry.getInstrumentation()
                .addMonitor(SecondActivity.class.getName(), null /* activityResult */,
                        false /* block */);

        callbackTrackingActivity.startActivityForResult(
                new Intent(callbackTrackingActivity, SecondActivity.class), 1 /* requestCode */);

        // Wait for the ActivityMonitor to be hit
        final Activity secondActivity = InstrumentationRegistry.getInstrumentation()
                .waitForMonitorWithTimeout(activityMonitor, 5 * 1000);

        // Wait for second activity to resume
        assertNotNull("Second activity should be started", secondActivity);
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME));

        // Start an activity in separate task (will be placed in secondary stack)
        getLaunchActivityBuilder().execute();

        waitAndAssertActivityStates(state(secondActivity, ON_PAUSE));

        // Finish top activity and verify that activity below became focused.
        getLifecycleLog().clear();
        secondActivity.setResult(Activity.RESULT_OK);
        secondActivity.finish();

        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_START));
        LifecycleVerifier.assertRestartSequence(CallbackTrackingActivity.class, getLifecycleLog());

        // Bring the first activity to front to verify that it receives the result.
        getLifecycleLog().clear();
        final Intent singleTopIntent = new Intent(InstrumentationRegistry.getTargetContext(),
                CallbackTrackingActivity.class);
        singleTopIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
        InstrumentationRegistry.getTargetContext().startActivity(singleTopIntent);

        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_RESUME));
        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class, getLifecycleLog(),
                Arrays.asList(ON_ACTIVITY_RESULT, ON_NEW_INTENT, ON_RESUME), "bring to front");
    }

    @Test
    public void testPausedWhenRestartedFromInNonFocusedStack() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no split multi-window support
            return;
        }

        // Launch first activity
        final Activity firstActivity =
                mFirstActivityTestRule.launchActivity(new Intent());

        // Wait for first activity to resume
        waitAndAssertActivityStates(state(firstActivity, ON_RESUME));

        // Enter split screen
        moveTaskToPrimarySplitScreen(firstActivity.getTaskId(),
                true /* launchSideActivityIfNeeded */);

        // Launch second activity to pause first
        final Activity secondActivity =
                mSecondActivityTestRule.launchActivity(new Intent());

        // Wait for second activity to resume
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME));

        // Start an activity in separate task (will be placed in secondary stack)
        getLaunchActivityBuilder().execute();

        waitAndAssertActivityStates(state(secondActivity, ON_PAUSE));

        getLifecycleLog().clear();

        // Finish top activity
        secondActivity.finish();

        waitAndAssertActivityStates(state(firstActivity, ON_PAUSE));

        // Verify that the first activity was restarted to pause as it was brought back after
        // windowing mode was switched
        LifecycleVerifier.assertRestartAndPauseSequence(FirstActivity.class, getLifecycleLog());
    }

    @Test
    public void testOnActivityResult() throws Exception {
        final Intent intent = new Intent();
        intent.putExtra(EXTRA_FINISH_IN_ON_RESUME, true);
        mLaunchForResultActivityTestRule.launchActivity(intent);

        final List<LifecycleLog.ActivityCallback> expectedSequence =
                Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME,
                        ON_PAUSE, ON_ACTIVITY_RESULT, ON_RESUME);
        waitForActivityTransitions(LaunchForResultActivity.class, expectedSequence);

        // TODO(b/79218023): First activity might also be stopped before getting result.
        final List<LifecycleLog.ActivityCallback> sequenceWithStop =
                Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME,
                        ON_PAUSE, ON_STOP, ON_ACTIVITY_RESULT, ON_RESTART, ON_START, ON_RESUME);
        LifecycleVerifier.assertSequenceMatchesOneOf(LaunchForResultActivity.class,
                getLifecycleLog(), Arrays.asList(expectedSequence, sequenceWithStop),
                "activityResult");
    }

    @Test
    public void testOnActivityResultAfterStop() throws Exception {
        final Intent intent = new Intent();
        intent.putExtra(EXTRA_FINISH_AFTER_RESUME, true);
        mLaunchForResultActivityTestRule.launchActivity(intent);
        final boolean isTranslucent = isTranslucent(mLaunchForResultActivityTestRule.getActivity());

        final List<LifecycleLog.ActivityCallback> expectedSequence;
        if (isTranslucent) {
            expectedSequence = Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE,
                    ON_RESUME, ON_PAUSE, ON_ACTIVITY_RESULT, ON_RESUME);
        } else {
            expectedSequence = Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE,
                    ON_RESUME, ON_PAUSE, ON_STOP, ON_ACTIVITY_RESULT, ON_RESTART, ON_START,
                    ON_RESUME);
        }
        waitForActivityTransitions(LaunchForResultActivity.class, expectedSequence);

        LifecycleVerifier.assertSequence(LaunchForResultActivity.class,
                getLifecycleLog(), expectedSequence, "activityResult");
    }

    @Test
    public void testOnPostCreateAfterCreate() throws Exception {
        final Activity callbackTrackingActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        waitAndAssertActivityStates(state(callbackTrackingActivity, ON_RESUME));

        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class,
                getLifecycleLog(),
                Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME),
                "create");
    }

    @Test
    public void testOnPostCreateAfterRecreateInOnResume() throws Exception {
        // Launch activity
        final Activity trackingActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        // Wait for activity to resume
        waitAndAssertActivityStates(state(trackingActivity, ON_RESUME));

        // Call "recreate" and assert sequence
        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(trackingActivity::recreate);
        waitAndAssertActivityStates(state(trackingActivity, ON_RESUME));

        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class,
                getLifecycleLog(),
                Arrays.asList(ON_PAUSE, ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                        ON_POST_CREATE, ON_RESUME),
                "recreate");
    }

    @Test
    public void testOnPostCreateAfterRecreateInOnPause() throws Exception {
        // Launch activity
        final Activity trackingActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        // Wait for activity to resume
        waitAndAssertActivityStates(state(trackingActivity, ON_RESUME));

        // Launch translucent activity, which will make the first one paused.
        mTranslucentActivityTestRule.launchActivity(new Intent());

        // Wait for first activity to become paused
        waitAndAssertActivityStates(state(trackingActivity, ON_PAUSE));

        // Call "recreate" and assert sequence
        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(trackingActivity::recreate);
        waitAndAssertActivityStates(state(trackingActivity, ON_PAUSE));

        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class,
                getLifecycleLog(),
                Arrays.asList(ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                        ON_POST_CREATE, ON_RESUME, ON_PAUSE),
                "recreate");
    }

    @Test
    public void testOnPostCreateAfterRecreateInOnStop() throws Exception {
        // Launch first activity
        final Activity trackingActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        // Wait for activity to resume
        waitAndAssertActivityStates(state(trackingActivity, ON_RESUME));

        // Launch second activity to cover and stop first
        final Activity secondActivity =
                mSecondActivityTestRule.launchActivity(new Intent());

        // Wait for second activity to become resumed
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME));

        // Wait for first activity to become stopped
        waitAndAssertActivityStates(occludedActivityState(trackingActivity, secondActivity));

        // Call "recreate" and assert sequence
        getLifecycleLog().clear();
        InstrumentationRegistry.getInstrumentation().runOnMainSync(trackingActivity::recreate);
        waitAndAssertActivityStates(occludedActivityState(trackingActivity, secondActivity));

        final List<LifecycleLog.ActivityCallback> callbacks;
        if (isTranslucent(secondActivity)) {
            callbacks = Arrays.asList(ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                    ON_POST_CREATE, ON_RESUME, ON_PAUSE);
        } else {
            callbacks = Arrays.asList(ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                    ON_POST_CREATE, ON_RESUME, ON_PAUSE, ON_STOP);
        }

        LifecycleVerifier.assertSequence(
                CallbackTrackingActivity.class, getLifecycleLog(), callbacks, "recreate");
    }

    /**
     * The following test ensures an activity is brought back if its process is ended in the
     * background.
     */
    @Test
    public void testRestoreFromKill() throws Exception {
        final LaunchActivityBuilder builder = getLaunchActivityBuilder();
        final ComponentName targetActivity = builder.getTargetActivity();

        // Launch activity whose process will be killed
        builder.execute();

        // Start activity in another process to put original activity in background.
        mFirstActivityTestRule.launchActivity(new Intent());
        final boolean isTranslucent = isTranslucent(mFirstActivityTestRule.getActivity());
        mAmWmState.waitForActivityState(
                targetActivity, isTranslucent ? STATE_PAUSED : STATE_STOPPED);

        // Only try to kill targetActivity if the top activity isn't translucent. If the top
        // activity is translucent then targetActivity will be visible, so the process will be
        // started again really quickly.
        if (!isTranslucent) {
            // Kill first activity
            AmUtils.runKill(targetActivity.getPackageName(), true /* wait */);
        }

        // Return back to first activity
        pressBackButton();

        // Verify activity is resumed
        mAmWmState.waitForValidState(targetActivity);
        mAmWmState.assertResumedActivity("Originally launched activity should be resumed",
                targetActivity);
    }

    /**
     * The that recreate request from an activity is executed immediately.
     */
    @Test
    public void testLocalRecreate() throws Exception {
        // Launch the activity that will recreate itself
        Activity recreatingActivity = mSingleTopActivityTestRule.launchActivity(new Intent());

        // Launch second activity to cover and stop first
        final LaunchActivityBuilder launchActivityBuilder = getLaunchActivityBuilder();
        launchActivityBuilder.setNewTask(true).setMultipleTask(true).execute();

        // Wait for first activity to become stopped
        waitAndAssertActivityStates(occludedActivityState(recreatingActivity,
                launchActivityBuilder.isTargetActivityTranslucent()));

        // Launch the activity again to recreate
        getLifecycleLog().clear();
        final Intent intent = new Intent(InstrumentationRegistry.getContext(),
                SingleTopActivity.class);
        intent.putExtra(EXTRA_RECREATE, true);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        InstrumentationRegistry.getTargetContext().startActivity(intent);

        // Wait for activity to relaunch and resume
        final List<LifecycleLog.ActivityCallback> expectedRelaunchSequence;
        if (launchActivityBuilder.isTargetActivityTranslucent()) {
            expectedRelaunchSequence = Arrays.asList(ON_NEW_INTENT, ON_RESUME, ON_PAUSE, ON_STOP,
                    ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME,
                    ON_PAUSE, ON_RESUME);
        } else {
            expectedRelaunchSequence = Arrays.asList(ON_NEW_INTENT, ON_RESTART, ON_START, ON_RESUME,
                    ON_PAUSE, ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                    ON_POST_CREATE, ON_RESUME);
        }

        waitForActivityTransitions(SingleTopActivity.class, expectedRelaunchSequence);
        LifecycleVerifier.assertSequence(SingleTopActivity.class, getLifecycleLog(),
                expectedRelaunchSequence, "recreate");
    }

    @Test
    public void testOnNewIntent() throws Exception {
        // Launch a singleTop activity
        final Activity singleTopActivity =
                mSingleTopActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(singleTopActivity, ON_RESUME));
        LifecycleVerifier.assertLaunchSequence(SingleTopActivity.class, getLifecycleLog(),
                true /* includeCallbacks */);

        // Try to launch again
        getLifecycleLog().clear();
        final Intent intent = new Intent(InstrumentationRegistry.getContext(),
                SingleTopActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        InstrumentationRegistry.getTargetContext().startActivity(intent);

        // Wait for the activity to resume again
        waitAndAssertActivityStates(state(singleTopActivity, ON_RESUME));

        // Verify that the first activity was paused, new intent was delivered and resumed again
        LifecycleVerifier.assertSequence(SingleTopActivity.class, getLifecycleLog(),
                Arrays.asList(ON_PAUSE, ON_NEW_INTENT, ON_RESUME), "newIntent");
    }

    @Test
    public void testOnNewIntentFromHidden() throws Exception {
        // Launch a singleTop activity
        final Activity singleTopActivity =
                mSingleTopActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(singleTopActivity, ON_RESUME));
        LifecycleVerifier.assertLaunchSequence(SingleTopActivity.class, getLifecycleLog(),
                true /* includeCallbacks */);

        // Launch something on top
        final Intent newTaskIntent = new Intent();
        newTaskIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_MULTIPLE_TASK);
        final Activity secondActivity = mSecondActivityTestRule.launchActivity(newTaskIntent);

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(secondActivity, ON_RESUME));
        waitAndAssertActivityStates(occludedActivityState(singleTopActivity, secondActivity));

        // Try to launch again
        getLifecycleLog().clear();
        final Intent intent = new Intent(InstrumentationRegistry.getContext(),
                SingleTopActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        InstrumentationRegistry.getTargetContext().startActivity(intent);

        // Wait for the activity to resume again
        waitAndAssertActivityStates(state(singleTopActivity, ON_RESUME));

        // Verify that the first activity was restarted, new intent was delivered and resumed again
        final List<LifecycleLog.ActivityCallback> expectedSequence;
        final List<LifecycleLog.ActivityCallback> otherSequence;
        if (isTranslucent(singleTopActivity)) {
            expectedSequence = Arrays.asList(ON_NEW_INTENT, ON_RESUME, ON_PAUSE, ON_RESUME);
            otherSequence = Arrays.asList(ON_RESTART, ON_START, ON_NEW_INTENT, ON_RESUME);
        } else {
            expectedSequence = Arrays.asList(ON_NEW_INTENT, ON_RESTART, ON_START, ON_RESUME);
            otherSequence = Arrays.asList(ON_RESTART, ON_START, ON_NEW_INTENT, ON_RESUME);
        }
        // TODO(b/65236456): This should always be ON_RESTART, ON_START, ON_NEW_INTENT, ON_RESUME
        LifecycleVerifier.assertSequenceMatchesOneOf(SingleTopActivity.class, getLifecycleLog(),
                Arrays.asList(expectedSequence, otherSequence), "newIntent");
    }

    @Test
    public void testOnNewIntentFromPaused() throws Exception {
        // Launch a singleTop activity
        final Activity singleTopActivity =
                mSingleTopActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(singleTopActivity, ON_RESUME));
        LifecycleVerifier.assertLaunchSequence(SingleTopActivity.class, getLifecycleLog(),
                true /* includeCallbacks */);

        // Launch translucent activity, which will make the first one paused.
        mTranslucentActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to pause
        waitAndAssertActivityStates(state(singleTopActivity, ON_PAUSE));

        // Try to launch again
        getLifecycleLog().clear();
        final Intent intent = new Intent(InstrumentationRegistry.getContext(),
                SingleTopActivity.class);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        InstrumentationRegistry.getTargetContext().startActivity(intent);

        // Wait for the activity to resume again
        final List<LifecycleLog.ActivityCallback> expectedSequence =
                Arrays.asList(ON_NEW_INTENT, ON_RESUME);
        waitForActivityTransitions(SingleTopActivity.class, expectedSequence);

        // Verify that the new intent was delivered and resumed again
        // TODO(b/77974794): New intent handling sequence should always be the same.
        // It is possible to get an extra pause and resume now.
        final List<LifecycleLog.ActivityCallback> extraPauseSequence =
                Arrays.asList(ON_NEW_INTENT, ON_RESUME, ON_PAUSE, ON_RESUME);
        LifecycleVerifier.assertSequenceMatchesOneOf(SingleTopActivity.class, getLifecycleLog(),
                Arrays.asList(expectedSequence, extraPauseSequence), "newIntent");
    }

    @Test
    public void testLifecycleOnMoveToFromSplitScreenRelaunch() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no split multi-window support
            return;
        }

        // Launch a singleTop activity
        final Activity testActivity =
                mCallbackTrackingActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(testActivity, ON_RESUME));
        LifecycleVerifier.assertLaunchSequence(CallbackTrackingActivity.class, getLifecycleLog(),
                true /* includeCallbacks */);

        // Enter split screen
        getLifecycleLog().clear();
        setActivityTaskWindowingMode(CALLBACK_TRACKING_ACTIVITY,
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);

        // Wait for the activity to pause
        final List<LifecycleLog.ActivityCallback> expectedEnterSequence =
                Arrays.asList(ON_PAUSE, ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                        ON_POST_CREATE, ON_RESUME, ON_MULTI_WINDOW_MODE_CHANGED, ON_PAUSE);
        waitForActivityTransitions(CallbackTrackingActivity.class, expectedEnterSequence);

        // Verify that the activity was relaunched and received multi-window mode change
        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class, getLifecycleLog(),
                expectedEnterSequence, "moveToSplitScreen");

        // Exit split-screen
        getLifecycleLog().clear();
        setActivityTaskWindowingMode(CALLBACK_TRACKING_ACTIVITY, WINDOWING_MODE_FULLSCREEN);

        // Wait for the activity to resume
        final List<LifecycleLog.ActivityCallback> expectedExitSequence =
                Arrays.asList(ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START,
                        ON_POST_CREATE, ON_RESUME, ON_PAUSE, ON_MULTI_WINDOW_MODE_CHANGED,
                        ON_RESUME);
        waitForActivityTransitions(CallbackTrackingActivity.class, expectedExitSequence);

        // Verify that the activity was relaunched and received multi-window mode change
        LifecycleVerifier.assertSequence(CallbackTrackingActivity.class, getLifecycleLog(),
                expectedExitSequence, "moveFromSplitScreen");
    }

    @Test
    public void testLifecycleOnMoveToFromSplitScreenNoRelaunch() throws Exception {
        if (!supportsSplitScreenMultiWindow()) {
            // Skipping test: no split multi-window support
            return;
        }

        // Launch a singleTop activity
        final Activity testActivity =
                mConfigChangeHandlingActivityTestRule.launchActivity(new Intent());

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(testActivity, ON_RESUME));
        LifecycleVerifier.assertLaunchSequence(ConfigChangeHandlingActivity.class,
                getLifecycleLog(), true /* includeCallbacks */);

        // Enter split screen
        getLifecycleLog().clear();
        setActivityTaskWindowingMode(CONFIG_CHANGE_HANDLING_ACTIVITY,
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY);

        // Wait for the activity to pause
        waitAndAssertActivityStates(state(testActivity, ON_PAUSE));

        // Verify that the activity was relaunched and received multi-window mode change
        LifecycleVerifier.assertSequence(ConfigChangeHandlingActivity.class, getLifecycleLog(),
                Arrays.asList(ON_MULTI_WINDOW_MODE_CHANGED, ON_PAUSE), "moveToSplitScreen");

        // Exit split-screen
        getLifecycleLog().clear();
        setActivityTaskWindowingMode(CONFIG_CHANGE_HANDLING_ACTIVITY, WINDOWING_MODE_FULLSCREEN);

        // Wait for the activity to resume
        waitAndAssertActivityStates(state(testActivity, ON_RESUME));

        // Verify that the activity was relaunched and received multi-window mode change
        LifecycleVerifier.assertSequence(ConfigChangeHandlingActivity.class, getLifecycleLog(),
                Arrays.asList(ON_MULTI_WINDOW_MODE_CHANGED, ON_RESUME), "moveFromSplitScreen");
    }
}
