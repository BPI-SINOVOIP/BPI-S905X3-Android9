package android.server.am.lifecycle;

import static android.server.am.StateLogger.log;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_ACTIVITY_RESULT;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback
        .ON_MULTI_WINDOW_MODE_CHANGED;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_NEW_INTENT;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_PAUSE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_POST_CREATE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;

import android.annotation.Nullable;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.server.am.ActivityManagerTestBase;
import android.server.am.lifecycle.LifecycleLog.ActivityCallback;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.lifecycle.ActivityLifecycleMonitor;
import android.support.test.runner.lifecycle.ActivityLifecycleMonitorRegistry;
import android.util.Pair;

import org.junit.After;
import org.junit.Before;

import java.util.List;

/** Base class for device-side tests that verify correct activity lifecycle transitions. */
public class ActivityLifecycleClientTestBase extends ActivityManagerTestBase {

    static final String EXTRA_RECREATE = "recreate";
    static final String EXTRA_FINISH_IN_ON_RESUME = "finish_in_on_resume";
    static final String EXTRA_FINISH_AFTER_RESUME = "finish_after_resume";

    static final ComponentName CALLBACK_TRACKING_ACTIVITY =
            getComponentName(CallbackTrackingActivity.class);

    static final ComponentName CONFIG_CHANGE_HANDLING_ACTIVITY =
            getComponentName(ConfigChangeHandlingActivity.class);

    final ActivityTestRule mFirstActivityTestRule = new ActivityTestRule(FirstActivity.class,
            true /* initialTouchMode */, false /* launchActivity */);

    final ActivityTestRule mSecondActivityTestRule = new ActivityTestRule(SecondActivity.class,
            true /* initialTouchMode */, false /* launchActivity */);

    final ActivityTestRule mTranslucentActivityTestRule = new ActivityTestRule(
            TranslucentActivity.class, true /* initialTouchMode */, false /* launchActivity */);

    final ActivityTestRule mSecondTranslucentActivityTestRule = new ActivityTestRule(
            SecondTranslucentActivity.class, true /* initialTouchMode */,
            false /* launchActivity */);

    final ActivityTestRule mLaunchForResultActivityTestRule = new ActivityTestRule(
            LaunchForResultActivity.class, true /* initialTouchMode */, false /* launchActivity */);

    final ActivityTestRule mCallbackTrackingActivityTestRule = new ActivityTestRule(
            CallbackTrackingActivity.class, true /* initialTouchMode */,
            false /* launchActivity */);

    final ActivityTestRule mSingleTopActivityTestRule = new ActivityTestRule(
            SingleTopActivity.class, true /* initialTouchMode */, false /* launchActivity */);

    final ActivityTestRule mConfigChangeHandlingActivityTestRule = new ActivityTestRule(
            ConfigChangeHandlingActivity.class, true /* initialTouchMode */,
            false /* launchActivity */);

    private final ActivityLifecycleMonitor mLifecycleMonitor = ActivityLifecycleMonitorRegistry
            .getInstance();
    private static LifecycleLog mLifecycleLog;
    private LifecycleTracker mLifecycleTracker;

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        // Log transitions for all activities that belong to this app.
        mLifecycleLog = new LifecycleLog();
        mLifecycleMonitor.addLifecycleCallback(mLifecycleLog);

        // Track transitions and allow waiting for pending activity states.
        mLifecycleTracker = new LifecycleTracker(mLifecycleLog);
        mLifecycleMonitor.addLifecycleCallback(mLifecycleTracker);
    }

    @After
    @Override
    public void tearDown() throws Exception {
        mLifecycleMonitor.removeLifecycleCallback(mLifecycleLog);
        mLifecycleMonitor.removeLifecycleCallback(mLifecycleTracker);
        super.tearDown();
    }

    /** Launch an activity given a class. */
    protected Activity launchActivity(Class<? extends Activity> activityClass) {
        final Intent intent = new Intent(InstrumentationRegistry.getTargetContext(), activityClass);
        return InstrumentationRegistry.getInstrumentation().startActivitySync(intent);
    }

    /**
     * Blocking call that will wait for activities to reach expected states with timeout.
     */
    @SafeVarargs
    final void waitAndAssertActivityStates(Pair<Activity, ActivityCallback>... activityCallbacks) {
        log("Start waitAndAssertActivityCallbacks");
        mLifecycleTracker.waitAndAssertActivityStates(activityCallbacks);
    }

    /**
     * Blocking call that will wait for activities to perform the expected sequence of transitions.
     * @see LifecycleTracker#waitForActivityTransitions(Class, List)
     */
    final void waitForActivityTransitions(Class<? extends Activity> activityClass,
            List<ActivityCallback> expectedTransitions) {
        log("Start waitAndAssertActivityTransition");
        mLifecycleTracker.waitForActivityTransitions(activityClass, expectedTransitions);
    }

    LifecycleLog getLifecycleLog() {
        return mLifecycleLog;
    }

    static Pair<Activity, ActivityCallback> state(Activity activity, ActivityCallback stage) {
        return new Pair<>(activity, stage);
    }

    /**
     * Returns a pair of the activity and the state it should be in based on the configuration of
     * occludingActivity.
     */
    static Pair<Activity, ActivityCallback> occludedActivityState(
            Activity activity, Activity occludingActivity) {
        return occludedActivityState(activity, isTranslucent(occludingActivity));
    }

    /**
     * Returns a pair of the activity and the state it should be in based on
     * occludingActivityIsTranslucent.
     */
    static Pair<Activity, ActivityCallback> occludedActivityState(
            Activity activity, boolean occludingActivityIsTranslucent) {
        // Activities behind a translucent activity should be in the paused state since they are
        // still visible. Otherwise, they should be in the stopped state.
        return new Pair<>(activity, occludedActivityState(occludingActivityIsTranslucent));
    }

    static ActivityCallback occludedActivityState(boolean occludingActivityIsTranslucent) {
        return occludingActivityIsTranslucent ? ON_PAUSE : ON_STOP;
    }

    /** Returns true if the input activity is translucent. */
    static boolean isTranslucent(Activity activity) {
        return ActivityInfo.isTranslucentOrFloating(activity.getWindow().getWindowStyle());
    }

    // Test activity
    public static class FirstActivity extends Activity {
    }

    // Test activity
    public static class SecondActivity extends Activity {
    }

    // Translucent test activity
    public static class TranslucentActivity extends Activity {
    }

    // Translucent test activity
    public static class SecondTranslucentActivity extends Activity {
    }

    /**
     * Base activity that records callbacks other then main lifecycle transitions.
     */
    public static class CallbackTrackingActivity extends Activity {
        @Override
        protected void onActivityResult(int requestCode, int resultCode, Intent data) {
            super.onActivityResult(requestCode, resultCode, data);
            mLifecycleLog.onActivityCallback(this, ON_ACTIVITY_RESULT);
        }

        @Override
        protected void onPostCreate(@Nullable Bundle savedInstanceState) {
            super.onPostCreate(savedInstanceState);
            mLifecycleLog.onActivityCallback(this, ON_POST_CREATE);
        }

        @Override
        protected void onNewIntent(Intent intent) {
            super.onNewIntent(intent);
            mLifecycleLog.onActivityCallback(this, ON_NEW_INTENT);
        }

        @Override
        public void onMultiWindowModeChanged(boolean isInMultiWindowMode, Configuration newConfig) {
            mLifecycleLog.onActivityCallback(this, ON_MULTI_WINDOW_MODE_CHANGED);
        }
    }

    /**
     * Test activity that launches {@link ResultActivity} for result.
     */
    public static class LaunchForResultActivity extends CallbackTrackingActivity {

        @Override
        protected void onCreate(Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            startForResult();
        }

        private void startForResult() {
            final Intent intent = new Intent(this, ResultActivity.class);
            intent.putExtras(getIntent());
            startActivityForResult(intent, 1 /* requestCode */);
        }
    }

    /** Test activity that is started for result and finishes itself in ON_RESUME. */
    public static class ResultActivity extends Activity {
        @Override
        protected void onResume() {
            super.onResume();
            setResult(RESULT_OK);
            final Intent intent = getIntent();
            if (intent.getBooleanExtra(EXTRA_FINISH_IN_ON_RESUME, false)) {
                finish();
            } else if (intent.getBooleanExtra(EXTRA_FINISH_AFTER_RESUME, false)) {
                new Handler().postDelayed(() -> finish(), 2000);
            }
        }
    }

    /** Test activity that can call {@link Activity#recreate()} if requested in a new intent. */
    public static class SingleTopActivity extends CallbackTrackingActivity {

        @Override
        protected void onNewIntent(Intent intent) {
            super.onNewIntent(intent);
            if (intent != null && intent.getBooleanExtra(EXTRA_RECREATE, false)) {
                recreate();
            }
        }
    }

    // Config change handling activity
    public static class ConfigChangeHandlingActivity extends CallbackTrackingActivity {
    }

    static ComponentName getComponentName(Class<? extends Activity> activity) {
        return new ComponentName(InstrumentationRegistry.getContext(), activity);
    }
}
