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

package android.server.am;

import android.content.ComponentName;
import android.server.am.component.ComponentsBase;

public class Components extends ComponentsBase {
    public static final ComponentName ALT_LAUNCHING_ACTIVITY = component("AltLaunchingActivity");
    public static final ComponentName ALWAYS_FOCUSABLE_PIP_ACTIVITY =
            component("AlwaysFocusablePipActivity");
    public static final ComponentName ANIMATION_TEST_ACTIVITY = component("AnimationTestActivity");
    public static final ComponentName ASSISTANT_ACTIVITY = component("AssistantActivity");
    public static final ComponentName BOTTOM_ACTIVITY = component("BottomActivity");
    public static final ComponentName BOTTOM_LEFT_LAYOUT_ACTIVITY =
            component("BottomLeftLayoutActivity");
    public static final ComponentName BOTTOM_RIGHT_LAYOUT_ACTIVITY =
            component("BottomRightLayoutActivity");
    public static final ComponentName BROADCAST_RECEIVER_ACTIVITY =
            component("BroadcastReceiverActivity");
    public static final ComponentName DIALOG_WHEN_LARGE_ACTIVITY =
            component("DialogWhenLargeActivity");
    public static final ComponentName DISMISS_KEYGUARD_ACTIVITY =
            component("DismissKeyguardActivity");
    public static final ComponentName DISMISS_KEYGUARD_METHOD_ACTIVITY =
            component("DismissKeyguardMethodActivity");
    public static final ComponentName DOCKED_ACTIVITY = component("DockedActivity");
    /** This activity is an alias activity pointing {@link TrampolineActivity} in AndroidManifest.xml */
    public static final ComponentName ENTRY_POINT_ALIAS_ACTIVITY =
            component("EntryPointAliasActivity");
    public static final ComponentName FONT_SCALE_ACTIVITY = component("FontScaleActivity");
    public static final ComponentName FONT_SCALE_NO_RELAUNCH_ACTIVITY =
            component("FontScaleNoRelaunchActivity");
    public static final ComponentName FREEFORM_ACTIVITY = component("FreeformActivity");
    public static final ComponentName KEYGUARD_LOCK_ACTIVITY = component("KeyguardLockActivity");
    public static final ComponentName LANDSCAPE_ORIENTATION_ACTIVITY =
            component("LandscapeOrientationActivity");
    public static final ComponentName LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION =
            component("LaunchAssistantActivityFromSession");
    public static final ComponentName LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK  =
            component("LaunchAssistantActivityIntoAssistantStack");
    public static final ComponentName LAUNCH_ENTER_PIP_ACTIVITY =
            component("LaunchEnterPipActivity");
    public static final ComponentName LAUNCH_INTO_PINNED_STACK_PIP_ACTIVITY =
            component("LaunchIntoPinnedStackPipActivity");
    public static final ComponentName LAUNCH_PIP_ON_PIP_ACTIVITY =
            component("LaunchPipOnPipActivity");
    public static final ComponentName LAUNCHING_ACTIVITY = component("LaunchingActivity");
    public static final ComponentName LOG_CONFIGURATION_ACTIVITY =
            component("LogConfigurationActivity");
    public static final ComponentName MOVE_TASK_TO_BACK_ACTIVITY =
            component("MoveTaskToBackActivity");
    public static final ComponentName NIGHT_MODE_ACTIVITY = component("NightModeActivity");
    public static final ComponentName NO_HISTORY_ACTIVITY = component("NoHistoryActivity");
    public static final ComponentName NO_RELAUNCH_ACTIVITY = component("NoRelaunchActivity");
    public static final ComponentName NON_RESIZEABLE_ACTIVITY = component("NonResizeableActivity");
    public static final ComponentName PIP_ACTIVITY = component("PipActivity");
    public static final ComponentName PIP_ACTIVITY2 = component("PipActivity2");
    public static final ComponentName PIP_ACTIVITY_WITH_SAME_AFFINITY =
            component("PipActivityWithSameAffinity");
    public static final ComponentName PIP_ON_STOP_ACTIVITY = component("PipOnStopActivity");
    public static final ComponentName PORTRAIT_ORIENTATION_ACTIVITY =
            component("PortraitOrientationActivity");
    public static final ComponentName RECURSIVE_ACTIVITY = component("RecursiveActivity");
    public static final ComponentName RESIZEABLE_ACTIVITY = component("ResizeableActivity");
    public static final ComponentName RESUME_WHILE_PAUSING_ACTIVITY =
            component("ResumeWhilePausingActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_ACTIVITY =
            component("ShowWhenLockedActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_ATTR_ACTIVITY =
            component("ShowWhenLockedAttrActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_ATTR_REMOVE_ATTR_ACTIVITY =
            component("ShowWhenLockedAttrRemoveAttrActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_DIALOG_ACTIVITY =
            component("ShowWhenLockedDialogActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_TRANSLUCENT_ACTIVITY =
            component("ShowWhenLockedTranslucentActivity");
    public static final ComponentName SHOW_WHEN_LOCKED_WITH_DIALOG_ACTIVITY =
            component("ShowWhenLockedWithDialogActivity");
    public static final ComponentName SINGLE_INSTANCE_ACTIVITY =
            component("SingleInstanceActivity");
    public static final ComponentName SINGLE_TASK_ACTIVITY = component("SingleTaskActivity");
    public static final ComponentName SLOW_CREATE_ACTIVITY = component("SlowCreateActivity");
    public static final ComponentName SPLASHSCREEN_ACTIVITY = component("SplashscreenActivity");
    public static final ComponentName SWIPE_REFRESH_ACTIVITY = component("SwipeRefreshActivity");
    public static final ComponentName TEST_ACTIVITY = component("TestActivity");
    public static final ComponentName TOP_ACTIVITY = component("TopActivity");
    public static final ComponentName TEST_ACTIVITY_WITH_SAME_AFFINITY =
            component("TestActivityWithSameAffinity");
    public static final ComponentName TOP_LEFT_LAYOUT_ACTIVITY = component("TopLeftLayoutActivity");
    public static final ComponentName TOP_RIGHT_LAYOUT_ACTIVITY =
            component("TopRightLayoutActivity");
    public static final ComponentName TRANSLUCENT_ACTIVITY = component("TranslucentActivity");
    public static final ComponentName TRANSLUCENT_ASSISTANT_ACTIVITY =
            component("TranslucentAssistantActivity");
    public static final ComponentName TRANSLUCENT_TOP_ACTIVITY =
            component("TranslucentTopActivity");
    public static final ComponentName TRANSLUCENT_TEST_ACTIVITY =
            component("TranslucentTestActivity");
    public static final ComponentName TURN_SCREEN_ON_ACTIVITY = component("TurnScreenOnActivity");
    public static final ComponentName TURN_SCREEN_ON_ATTR_ACTIVITY =
            component("TurnScreenOnAttrActivity");
    public static final ComponentName TURN_SCREEN_ON_ATTR_DISMISS_KEYGUARD_ACTIVITY =
            component("TurnScreenOnAttrDismissKeyguardActivity");
    public static final ComponentName TURN_SCREEN_ON_ATTR_REMOVE_ATTR_ACTIVITY =
            component("TurnScreenOnAttrRemoveAttrActivity");
    public static final ComponentName TURN_SCREEN_ON_DISMISS_KEYGUARD_ACTIVITY =
            component("TurnScreenOnDismissKeyguardActivity");
    public static final ComponentName TURN_SCREEN_ON_SHOW_ON_LOCK_ACTIVITY =
            component("TurnScreenOnShowOnLockActivity");
    public static final ComponentName TURN_SCREEN_ON_SINGLE_TASK_ACTIVITY =
            component("TurnScreenOnSingleTaskActivity");
    public static final ComponentName TURN_SCREEN_ON_WITH_RELAYOUT_ACTIVITY =
            component("TurnScreenOnWithRelayoutActivity");
    public static final ComponentName VIRTUAL_DISPLAY_ACTIVITY =
            component("VirtualDisplayActivity");
    public static final ComponentName VR_TEST_ACTIVITY = component("VrTestActivity");
    public static final ComponentName WALLPAPAER_ACTIVITY = component("WallpaperActivity");

    public static final ComponentName ASSISTANT_VOICE_INTERACTION_SERVICE =
            component("AssistantVoiceInteractionService");

    public static final ComponentName LAUNCH_BROADCAST_RECEIVER =
            component("LaunchBroadcastReceiver");
    public static final String LAUNCH_BROADCAST_ACTION =
            getPackageName() + ".LAUNCH_BROADCAST_ACTION";

    /**
     * Action and extra key constants for {@link #TEST_ACTIVITY}.
     *
     * TODO(b/73346885): These constants should be in {@link android.server.am.TestActivity} once
     * the activity is moved to test APK.
     */
    public static class TestActivity {
        // Finishes the activity
        public static final String TEST_ACTIVITY_ACTION_FINISH_SELF =
                "android.server.am.TestActivity.finish_self";
        // Sets the fixed orientation (can be one of {@link ActivityInfo.ScreenOrientation}
        public static final String EXTRA_FIXED_ORIENTATION = "fixed_orientation";
    }

    /**
     * Extra key constants for {@link #LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK} and
     * {@link #LAUNCH_ASSISTANT_ACTIVITY_FROM_SESSION}.
     *
     * TODO(b/73346885): These constants should be in {@link android.server.am.AssistantActivity}
     * once the activity is moved to test APK.
     */
    public static class AssistantActivity {
        // Launches the given activity in onResume
        public static final String EXTRA_ASSISTANT_LAUNCH_NEW_TASK = "launch_new_task";
        // Finishes this activity in onResume, this happens after EXTRA_ASSISTANT_LAUNCH_NEW_TASK
        public static final String EXTRA_ASSISTANT_FINISH_SELF = "finish_self";
        // Attempts to enter picture-in-picture in onResume
        public static final String EXTRA_ASSISTANT_ENTER_PIP = "enter_pip";
        // Display on which Assistant runs
        public static final String EXTRA_ASSISTANT_DISPLAY_ID = "assistant_display_id";
    }

    /**
     * Extra key constants for {@link android.server.am.BottomActivity}.
     *
     * TODO(b/73346885): These constants should be in {@link android.server.am.BottomActivity}
     * once the activity is moved to test APK.
     */
    public static class BottomActivity {
        public static final String EXTRA_BOTTOM_WALLPAPER = "USE_WALLPAPER";
        public static final String EXTRA_STOP_DELAY = "STOP_DELAY";
    }

    /**
     * Extra key constants for {@link android.server.am.BroadcastReceiverActivity}.
     *
     * TODO(b/73346885): These constants should be in
     * {@link android.server.am.BroadcastReceiverActivity} once the activity is moved to test APK.
     */
    public static class BroadcastReceiverActivity {
        public static final String ACTION_TRIGGER_BROADCAST = "trigger_broadcast";
        public static final String EXTRA_DISMISS_KEYGUARD = "dismissKeyguard";
        public static final String EXTRA_DISMISS_KEYGUARD_METHOD = "dismissKeyguardMethod";
        public static final String EXTRA_FINISH_BROADCAST = "finish";
        public static final String EXTRA_MOVE_BROADCAST_TO_BACK = "moveToBack";
        public static final String EXTRA_BROADCAST_ORIENTATION = "orientation";
    }

    /**
     *  Logging constants for {@link android.server.am.KeyguardDismissLoggerCallback}.
     *
     * TODO(b/73346885): These constants should be in
     * {@link android.server.am.KeyguardDismissLoggerCallback} once the class is moved to test APK.
     */
    public static class KeyguardDismissLoggerCallback {
        public static final String KEYGUARD_DISMISS_LOG_TAG = "KeyguardDismissLoggerCallback";
        public static final String ENTRY_ON_DISMISS_CANCELLED = "onDismissCancelled";
        public static final String ENTRY_ON_DISMISS_ERROR = "onDismissError";
        public static final String ENTRY_ON_DISMISS_SUCCEEDED = "onDismissSucceeded";
    }

    /**
     * Extra key constants for {@link #LAUNCH_ASSISTANT_ACTIVITY_INTO_STACK}.
     *
     * TODO(b/73346885): These constants should be in
     * {@link android.server.am.LaunchAssistantActivityIntoAssistantStack} once the activity is
     * moved to test APK.
     */
    public static class LaunchAssistantActivityIntoAssistantStack {
        // Launches the translucent assist activity
        public static final String EXTRA_ASSISTANT_IS_TRANSLUCENT = "is_translucent";
    }

    /**
     * Extra constants for {@link android.server.am.MoveTaskToBackActivity}.
     *
     * TODO(b/73346885): These constants should be in
     * {@link android.server.am.MoveTaskToBackActivity} once the activity is moved to test APK.
     */
    public static class MoveTaskToBackActivity {
        public static final String EXTRA_FINISH_POINT = "finish_point";
        public static final String FINISH_POINT_ON_PAUSE = "on_pause";
        public static final String FINISH_POINT_ON_STOP = "on_stop";
    }

    /**
     * Action and extra key constants for {@link android.server.am.PipActivity}.
     *
     * TODO(b/73346885): These constants should be in {@link android.server.am.PipActivity}
     * once the activity is moved to test APK.
     */
    public static class PipActivity {
        // Intent action that this activity dynamically registers to enter picture-in-picture
        public static final String ACTION_ENTER_PIP = "android.server.am.PipActivity.enter_pip";
        // Intent action that this activity dynamically registers to move itself to the back
        public static final String ACTION_MOVE_TO_BACK =
                "android.server.am.PipActivity.move_to_back";
        // Intent action that this activity dynamically registers to expand itself.
        // If EXTRA_SET_ASPECT_RATIO_WITH_DELAY is set, it will also attempt to apply the aspect
        // ratio after a short delay.
        public static final String ACTION_EXPAND_PIP = "android.server.am.PipActivity.expand_pip";
        // Intent action that this activity dynamically registers to set requested orientation.
        // Will apply the oriention to the value set in the EXTRA_FIXED_ORIENTATION extra.
        public static final String ACTION_SET_REQUESTED_ORIENTATION =
                "android.server.am.PipActivity.set_requested_orientation";
        // Intent action that will finish this activity
        public static final String ACTION_FINISH = "android.server.am.PipActivity.finish";

        // Adds an assertion that we do not ever get onStop() before we enter picture in picture
        public static final String EXTRA_ASSERT_NO_ON_STOP_BEFORE_PIP =
                "assert_no_on_stop_before_pip";
        // Calls enterPictureInPicture() on creation
        public static final String EXTRA_ENTER_PIP = "enter_pip";
        // Used with EXTRA_AUTO_ENTER_PIP, value specifies the aspect ratio to enter PIP with
        public static final String EXTRA_ENTER_PIP_ASPECT_RATIO_NUMERATOR =
                "enter_pip_aspect_ratio_numerator";
        // Used with EXTRA_AUTO_ENTER_PIP, value specifies the aspect ratio to enter PIP with
        public static final String EXTRA_ENTER_PIP_ASPECT_RATIO_DENOMINATOR =
                "enter_pip_aspect_ratio_denominator";
        // Calls requestAutoEnterPictureInPicture() with the value provided
        public static final String EXTRA_ENTER_PIP_ON_PAUSE = "enter_pip_on_pause";
        // Finishes the activity at the end of onResume (after EXTRA_START_ACTIVITY is handled)
        public static final String EXTRA_FINISH_SELF_ON_RESUME = "finish_self_on_resume";
        // Sets the fixed orientation (can be one of {@link ActivityInfo.ScreenOrientation}
        public static final String EXTRA_PIP_ORIENTATION = "fixed_orientation";
        // The amount to delay to artificially introduce in onPause()
        // (before EXTRA_ENTER_PIP_ON_PAUSE is processed)
        public static final String EXTRA_ON_PAUSE_DELAY = "on_pause_delay";
        // Calls enterPictureInPicture() again after onPictureInPictureModeChanged(false) is called
        public static final String EXTRA_REENTER_PIP_ON_EXIT = "reenter_pip_on_exit";
        // Calls setPictureInPictureAspectRatio with the aspect ratio specified in the value
        public static final String EXTRA_SET_ASPECT_RATIO_DENOMINATOR =
                "set_aspect_ratio_denominator";
        // Calls setPictureInPictureAspectRatio with the aspect ratio specified in the value
        public static final String EXTRA_SET_ASPECT_RATIO_NUMERATOR = "set_aspect_ratio_numerator";
        // Calls setPictureInPictureAspectRatio with the aspect ratio specified in the value with a
        // fixed delay
        public static final String EXTRA_SET_ASPECT_RATIO_WITH_DELAY_NUMERATOR =
                "set_aspect_ratio_with_delay_numerator";
        // Calls setPictureInPictureAspectRatio with the aspect ratio specified in the value with a
        // fixed delay
        public static final String EXTRA_SET_ASPECT_RATIO_WITH_DELAY_DENOMINATOR =
                "set_aspect_ratio_with_delay_denominator";
        // Shows this activity over the keyguard
        public static final String EXTRA_SHOW_OVER_KEYGUARD = "show_over_keyguard";
        // Starts the activity (component name) provided by the value at the end of onCreate
        public static final String EXTRA_START_ACTIVITY = "start_activity";
        // Adds a click listener to finish this activity when it is clicked
        public static final String EXTRA_TAP_TO_FINISH = "tap_to_finish";
    }

    /**
     * Extra key constants for {@link android.server.am.TopActivity} and
     * {@link android.server.am.TranslucentTopActivity}.
     *
     * TODO(b/73346885): These constants should be in {@link android.server.am.TopActivity}
     * once the activity is moved to test APK.
     */
    public static class TopActivity {
        public static final String EXTRA_FINISH_DELAY = "FINISH_DELAY";
        public static final String EXTRA_TOP_WALLPAPER = "USE_WALLPAPER";
    }

    /**
     * Extra key constants for {@link android.server.am.VirtualDisplayActivity}.
     *
     * TODO(b/73346885): These constants should be in
     * {@link android.server.am.VirtualDisplayActivity} once the activity is moved to test APK.
     */
    public static class VirtualDisplayActivity {
        public static final String VIRTUAL_DISPLAY_PREFIX = "HostedVirtualDisplay";
        public static final String KEY_CAN_SHOW_WITH_INSECURE_KEYGUARD =
                "can_show_with_insecure_keyguard";
        public static final String KEY_COMMAND = "command";
        public static final String KEY_COUNT = "count";
        public static final String KEY_DENSITY_DPI = "density_dpi";
        public static final String KEY_LAUNCH_TARGET_COMPONENT = "launch_target_component";
        public static final String KEY_PUBLIC_DISPLAY = "public_display";
        public static final String KEY_RESIZE_DISPLAY = "resize_display";
        // Value constants of {@link #KEY_COMMAND}.
        public static final String COMMAND_CREATE_DISPLAY = "create_display";
        public static final String COMMAND_DESTROY_DISPLAY = "destroy_display";
        public static final String COMMAND_RESIZE_DISPLAY = "resize_display";
    }

    private static ComponentName component(String className) {
        return component(Components.class, className);
    }

    private static String getPackageName() {
        return getPackageName(Components.class);
    }
}
