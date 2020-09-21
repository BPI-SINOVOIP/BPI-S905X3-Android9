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

import static android.app.WindowConfiguration.ACTIVITY_TYPE_ASSISTANT;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.WINDOWING_MODE_FULLSCREEN;
import static android.app.WindowConfiguration.WINDOWING_MODE_PINNED;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_PRIMARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_SPLIT_SCREEN_SECONDARY;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.am.ActivityManagerState.STATE_RESUMED;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.ANIMATION_TEST_ACTIVITY;
import static android.server.am.Components.ASSISTANT_ACTIVITY;
import static android.server.am.Components.ASSISTANT_VOICE_INTERACTION_SERVICE;
import static android.server.am.Components.AssistantActivity.EXTRA_ASSISTANT_DISPLAY_ID;
import static android.server.am.Components.AssistantActivity.EXTRA_ASSISTANT_ENTER_PIP;
import static android.server.am.Components.AssistantActivity.EXTRA_ASSISTANT_FINISH_SELF;
import static android.server.am.Components.AssistantActivity.EXTRA_ASSISTANT_LAUNCH_NEW_TASK;
import static android.server.am.Components.DOCKED_ACTIVITY;
import static android.server.am.Components.LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION;
import static android.server.am.Components.LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK;
import static android.server.am.Components.LaunchAssistantActivityIntoAssistantStack
        .EXTRA_ASSISTANT_IS_TRANSLUCENT;
import static android.server.am.Components.PIP_ACTIVITY;
import static android.server.am.Components.PipActivity.EXTRA_ENTER_PIP;
import static android.server.am.Components.TEST_ACTIVITY;
import static android.server.am.Components.TRANSLUCENT_ASSISTANT_ACTIVITY;
import static android.server.am.Components.TestActivity.TEST_ACTIVITY_ACTION_FINISH_SELF;
import static android.server.am.UiDeviceUtils.pressBackButton;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.hamcrest.Matchers.hasSize;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assume.assumeTrue;

import android.content.ComponentName;
import android.platform.test.annotations.Presubmit;
import android.provider.Settings;
import android.server.am.settings.SettingsSession;
import android.support.test.filters.FlakyTest;

import org.junit.Ignore;
import org.junit.Test;

/**
 * Build/Install/Run:
 *     atest CtsActivityManagerDeviceTestCases:ActivityManagerAssistantStackTests
 */
//@Presubmit b/67706642
@FlakyTest(bugId = 71875631)
public class ActivityManagerAssistantStackTests extends ActivityManagerTestBase {

    private int mAssistantDisplayId = DEFAULT_DISPLAY;

    public void setUp() throws Exception {
        super.setUp();
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            ActivityManagerState.ActivityStack assistantStack =
                    mAmWmState.getAmState().getStackByActivityType(ACTIVITY_TYPE_ASSISTANT);
            mAssistantDisplayId = assistantStack.mDisplayId;
        }
    }

    @Test
    @Presubmit
    public void testLaunchingAssistantActivityIntoAssistantStack() throws Exception {
        // Enable the assistant and launch an assistant activity
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);

            // Ensure that the activity launched in the fullscreen assistant stack
            assertAssistantStackExists();
            assertTrue("Expected assistant stack to be fullscreen",
                    mAmWmState.getAmState().getStackByActivityType(
                            ACTIVITY_TYPE_ASSISTANT).isFullscreen());
        }
    }

    @FlakyTest(bugId = 69573940)
    @Presubmit
    @Test
    public void testAssistantStackZOrder() throws Exception {
        assumeTrue(assistantRunsOnPrimaryDisplay());
        assumeTrue(supportsPip());
        assumeTrue(supportsSplitScreenMultiWindow());

        // Launch a pinned stack task
        launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
        waitForValidStateWithActivityTypeAndWindowingMode(
                PIP_ACTIVITY, ACTIVITY_TYPE_STANDARD, WINDOWING_MODE_PINNED);
        mAmWmState.assertContainsStack("Must contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);

        // Dock a task
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);

        // Enable the assistant and launch an assistant activity, ensure it is on top
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();

            mAmWmState.assertFrontStack("Pinned stack should be on top.",
                    WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
            mAmWmState.assertFocusedStack("Assistant stack should be focused.",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
        }
    }

    @Test
    @Presubmit
    public void testAssistantStackLaunchNewTask() throws Exception {
        assertAssistantStackCanLaunchAndReturnFromNewTask(WINDOWING_MODE_FULLSCREEN);
    }

    @Test
    @Presubmit
    @Ignore("b/77272253#comment10")
    public void testAssistantStackLaunchNewTaskWithDockedStack() throws Exception {
        assumeTrue(assistantRunsOnPrimaryDisplay());
        assumeTrue(supportsSplitScreenMultiWindow());

        // Dock a task
        launchActivitiesInSplitScreen(
                getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
        mAmWmState.assertContainsStack("Must contain fullscreen stack.",
                WINDOWING_MODE_SPLIT_SCREEN_SECONDARY, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertContainsStack("Must contain docked stack.",
                WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);

        assertAssistantStackCanLaunchAndReturnFromNewTask(WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
    }

    private void assertAssistantStackCanLaunchAndReturnFromNewTask(int expectedWindowingMode)
            throws Exception {
        // Enable the assistant and launch an assistant activity which will launch a new task
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK, mAssistantDisplayId,
                    EXTRA_ASSISTANT_LAUNCH_NEW_TASK, getActivityName(TEST_ACTIVITY),
                    EXTRA_ASSISTANT_DISPLAY_ID, Integer.toString(mAssistantDisplayId));
            // Ensure that the fullscreen stack is on top and the test activity is now visible
            waitForValidStateWithActivityTypeAndWindowingMode(
                    TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, expectedWindowingMode);
        }

        mAmWmState.assertFocusedActivity("TestActivity should be resumed", TEST_ACTIVITY);
        mAmWmState.assertFrontStack("Fullscreen stack should be on top.",
                expectedWindowingMode, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertFocusedStack("Fullscreen stack should be focused.",
                expectedWindowingMode, ACTIVITY_TYPE_STANDARD);

        // Now, tell it to finish itself and ensure that the assistant stack is brought back forward
        executeShellCommand("am broadcast -a " + TEST_ACTIVITY_ACTION_FINISH_SELF);
        mAmWmState.waitForFocusedStack(WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
        mAmWmState.assertFrontStackActivityType(
                "Assistant stack should be on top.", ACTIVITY_TYPE_ASSISTANT);
        mAmWmState.assertFocusedStack("Assistant stack should be focused.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 71875631)
    public void testAssistantStackFinishToPreviousApp() throws Exception {
        // Launch an assistant activity on top of an existing fullscreen activity, and ensure that
        // the fullscreen activity is still visible and on top after the assistant activity finishes
        launchActivityOnDisplay(TEST_ACTIVITY, mAssistantDisplayId);
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_FINISH_SELF, "true");
            mAmWmState.waitFor((amState, wmState) -> !amState.containsActivity(ASSISTANT_ACTIVITY),
                    "Waiting for " + getActivityName(ASSISTANT_ACTIVITY) + " finished");
        }
        waitForValidStateWithActivityTypeAndWindowingMode(
                TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, WINDOWING_MODE_FULLSCREEN);
        mAmWmState.waitForActivityState(TEST_ACTIVITY, STATE_RESUMED);
        mAmWmState.assertFocusedActivity("TestActivity should be resumed", TEST_ACTIVITY);
        mAmWmState.assertFrontStack("Fullscreen stack should be on top.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
        mAmWmState.assertFocusedStack("Fullscreen stack should be focused.",
                WINDOWING_MODE_FULLSCREEN, ACTIVITY_TYPE_STANDARD);
    }

    @Test
    @Presubmit
    @FlakyTest(bugId = 71875631)
    public void testDisallowEnterPiPFromAssistantStack() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_ENTER_PIP, "true");
        }
        waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
        mAmWmState.assertDoesNotContainStack("Must not contain pinned stack.",
                WINDOWING_MODE_PINNED, ACTIVITY_TYPE_STANDARD);
    }

    @FlakyTest(bugId = 69573940)
    @Presubmit
    @Test
    public void testTranslucentAssistantActivityStackVisibility() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Go home, launch the assistant and check to see that home is visible
            removeStacksInWindowingModes(WINDOWING_MODE_FULLSCREEN,
                    WINDOWING_MODE_SPLIT_SCREEN_SECONDARY);
            launchHomeActivity();
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_IS_TRANSLUCENT, "true");
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.waitForHomeActivityVisible();
            if (hasHomeScreen()) {
                mAmWmState.assertHomeActivityVisible(true);
            }

            // Launch a fullscreen app and then launch the assistant and check to see that it is
            // also visible
            removeStacksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
            launchActivityOnDisplay(TEST_ACTIVITY, mAssistantDisplayId);
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_IS_TRANSLUCENT, "true");
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.assertVisibility(TEST_ACTIVITY, true);

            // Go home, launch assistant, launch app into fullscreen with activity present, and go
            // back.Ensure home is visible.
            removeStacksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
            launchHomeActivity();
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_IS_TRANSLUCENT, "true",
                    EXTRA_ASSISTANT_LAUNCH_NEW_TASK, getActivityName(TEST_ACTIVITY));
            waitForValidStateWithActivityTypeAndWindowingMode(
                    TEST_ACTIVITY, ACTIVITY_TYPE_STANDARD, WINDOWING_MODE_FULLSCREEN);
            boolean isTranslucent = mAmWmState.getAmState().isActivityTranslucent(TEST_ACTIVITY);
            // Home should be visible if the occluding activity is translucent, else home shouldn't
            // be visible.
            mAmWmState.assertHomeActivityVisible(isTranslucent);
            pressBackButton();
            mAmWmState.waitForFocusedStack(WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.waitForHomeActivityVisible();
            if (hasHomeScreen()) {
                mAmWmState.assertHomeActivityVisible(true);
            }

            // Launch a fullscreen and docked app and then launch the assistant and check to see
            // that it
            // is also visible
            if (supportsSplitScreenMultiWindow() &&  assistantRunsOnPrimaryDisplay()) {
                removeStacksWithActivityTypes(ACTIVITY_TYPE_ASSISTANT);
                launchActivitiesInSplitScreen(
                        getLaunchActivityBuilder().setTargetActivity(DOCKED_ACTIVITY),
                        getLaunchActivityBuilder().setTargetActivity(TEST_ACTIVITY));
                mAmWmState.assertContainsStack("Must contain docked stack.",
                        WINDOWING_MODE_SPLIT_SCREEN_PRIMARY, ACTIVITY_TYPE_STANDARD);
                launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                        EXTRA_ASSISTANT_IS_TRANSLUCENT, "true");
                waitForValidStateWithActivityType(
                        TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
                assertAssistantStackExists();
                mAmWmState.assertVisibility(DOCKED_ACTIVITY, true);
                mAmWmState.assertVisibility(TEST_ACTIVITY, true);
            }
        }
    }

    @FlakyTest(bugId = 69229402)
    @Test
    @Presubmit
    public void testLaunchIntoSameTask() throws Exception {
        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Launch the assistant
            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION,
                    mAssistantDisplayId);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.assertVisibility(ASSISTANT_ACTIVITY, true);
            mAmWmState.assertFocusedStack("Expected assistant stack focused",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            final ActivityManagerState amState = mAmWmState.getAmState();
            assertThat(amState.getStackByActivityType(ACTIVITY_TYPE_ASSISTANT).getTasks(),
                    hasSize(1));
            final int taskId = mAmWmState.getAmState().getTaskByActivity(ASSISTANT_ACTIVITY)
                    .mTaskId;

            // Launch a new fullscreen activity
            // Using Animation Test Activity because it is opaque on all devices.
            launchActivityOnDisplay(ANIMATION_TEST_ACTIVITY, mAssistantDisplayId);
            // Wait for animation finished.
            mAmWmState.waitForActivityState(ANIMATION_TEST_ACTIVITY, STATE_RESUMED);
            mAmWmState.assertVisibility(ASSISTANT_ACTIVITY, false);

            // Launch the assistant again and ensure that it goes into the same task
            launchActivityOnDisplayNoWait(LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION,
                    mAssistantDisplayId);
            waitForValidStateWithActivityType(ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.assertVisibility(ASSISTANT_ACTIVITY, true);
            mAmWmState.assertFocusedStack("Expected assistant stack focused",
                    WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
            assertThat(amState.getStackByActivityType(ACTIVITY_TYPE_ASSISTANT).getTasks(),
                    hasSize(1));
            assertEquals(taskId,
                    mAmWmState.getAmState().getTaskByActivity(ASSISTANT_ACTIVITY).mTaskId);

        }
    }

    @Test
    public void testPinnedStackWithAssistant() throws Exception {
        assumeTrue(supportsPip());
        assumeTrue(supportsSplitScreenMultiWindow());

        try (final AssistantSession assistantSession = new AssistantSession()) {
            assistantSession.setVoiceInteractionService(ASSISTANT_VOICE_INTERACTION_SERVICE);

            // Launch a fullscreen activity and a PIP activity, then launch the assistant, and
            // ensure that the test activity is still visible
            launchActivity(TEST_ACTIVITY);
            launchActivity(PIP_ACTIVITY, EXTRA_ENTER_PIP, "true");
            launchActivityNoWait(LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK,
                    EXTRA_ASSISTANT_IS_TRANSLUCENT, String.valueOf(true));
            waitForValidStateWithActivityType(
                    TRANSLUCENT_ASSISTANT_ACTIVITY, ACTIVITY_TYPE_ASSISTANT);
            assertAssistantStackExists();
            mAmWmState.assertVisibility(TRANSLUCENT_ASSISTANT_ACTIVITY, true);
            mAmWmState.assertVisibility(PIP_ACTIVITY, true);
            mAmWmState.assertVisibility(TEST_ACTIVITY, true);

        }
    }

    private void waitForValidStateWithActivityType(ComponentName activityName, int activityType)
            throws Exception {
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(activityType)
                .build());
    }

    private void waitForValidStateWithActivityTypeAndWindowingMode(ComponentName activityName,
            int activityType, int windowingMode) throws Exception {
        mAmWmState.waitForValidState(new WaitForValidActivityState.Builder(activityName)
                .setActivityType(activityType)
                .setWindowingMode(windowingMode)
                .build());
    }

    /**
     * Asserts that the assistant stack exists.
     */
    private void assertAssistantStackExists() throws Exception {
        mAmWmState.assertContainsStack("Must contain assistant stack.",
                WINDOWING_MODE_UNDEFINED, ACTIVITY_TYPE_ASSISTANT);
    }

    // Any 2D Activity in VR mode is run on a special VR virtual display, so check if the Assistant
    // is going to run on the same display as other tasks.
    protected boolean assistantRunsOnPrimaryDisplay() {
        return mAssistantDisplayId == DEFAULT_DISPLAY;
    }

    /** Helper class to save, set, and restore
     * {@link Settings.Secure#VOICE_INTERACTION_SERVICE} system preference.
     */
    private static class AssistantSession extends SettingsSession<String> {
        AssistantSession() {
            super(Settings.Secure.getUriFor(Settings.Secure.VOICE_INTERACTION_SERVICE),
                    Settings.Secure::getString, Settings.Secure::putString);
        }

        void setVoiceInteractionService(ComponentName assistantName) throws Exception {
            super.set(getActivityName(assistantName));
        }
    }
}
