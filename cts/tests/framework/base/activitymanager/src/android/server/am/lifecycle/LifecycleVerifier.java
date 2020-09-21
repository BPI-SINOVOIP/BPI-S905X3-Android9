package android.server.am.lifecycle;

import static android.server.am.StateLogger.log;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_CREATE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_DESTROY;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_PAUSE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_POST_CREATE;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_RESTART;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_RESUME;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_START;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.ON_STOP;
import static android.server.am.lifecycle.LifecycleLog.ActivityCallback.PRE_ON_CREATE;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.app.Activity;
import android.server.am.lifecycle.LifecycleLog.ActivityCallback;
import android.util.Pair;

import java.util.Arrays;
import java.util.List;

/** Util class that verifies correct activity state transition sequences. */
class LifecycleVerifier {

    static void assertLaunchSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, boolean includeCallbacks) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch");

        final List<ActivityCallback> expectedTransitions = includeCallbacks
                ? Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_POST_CREATE, ON_RESUME)
                : Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchSequence(Class<? extends Activity> launchingActivity,
            Class<? extends Activity> existingActivity, LifecycleLog lifecycleLog,
            boolean launchingIsTranslucent) {
        final List<Pair<String, ActivityCallback>> observedTransitions = lifecycleLog.getLog();
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(launchingActivity, "launch");

        final List<Pair<String, ActivityCallback>> expectedTransitions;
        if (launchingIsTranslucent) {
            expectedTransitions = Arrays.asList(
                    transition(existingActivity, ON_PAUSE),
                    transition(launchingActivity, PRE_ON_CREATE),
                    transition(launchingActivity, ON_CREATE),
                    transition(launchingActivity, ON_START),
                    transition(launchingActivity, ON_RESUME));
        } else {
            expectedTransitions = Arrays.asList(
                    transition(existingActivity, ON_PAUSE),
                    transition(launchingActivity, PRE_ON_CREATE),
                    transition(launchingActivity, ON_CREATE),
                    transition(launchingActivity, ON_START),
                    transition(launchingActivity, ON_RESUME),
                    transition(existingActivity, ON_STOP));
        }

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndStopSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and stop");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE, ON_STOP);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndPauseSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and pause");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRestartSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "restart");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(ON_RESTART, ON_START);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRestartAndPauseSequence(Class<? extends Activity> activityClass,
                                              LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "restart and pause");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(ON_RESTART, ON_START, ON_RESUME, ON_PAUSE);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRecreateAndPauseSequence(Class<? extends Activity> activityClass,
                                              LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "recreateA  and pause");

        final List<ActivityCallback> expectedTransitions =
                Arrays.asList(ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertLaunchAndDestroySequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, "launch and destroy");

        final List<ActivityCallback> expectedTransitions = Arrays.asList(PRE_ON_CREATE, ON_CREATE,
                ON_START, ON_RESUME, ON_PAUSE, ON_STOP, ON_DESTROY);
        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    static void assertRelaunchSequence(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, ActivityCallback startState) {
        final List<ActivityCallback> expectedTransitions;
        if (startState == ON_PAUSE) {
            expectedTransitions = Arrays.asList(
                    ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE);
        } else if (startState == ON_STOP) {
            expectedTransitions = Arrays.asList(
                    ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME, ON_PAUSE, ON_STOP);
        } else {
            expectedTransitions = Arrays.asList(
                    ON_PAUSE, ON_STOP, ON_DESTROY, PRE_ON_CREATE, ON_CREATE, ON_START, ON_RESUME);
        }
        assertSequence(activityClass, lifecycleLog, expectedTransitions, "relaunch");
    }

    static void assertSequence(Class<? extends Activity> activityClass, LifecycleLog lifecycleLog,
            List<ActivityCallback> expectedTransitions, String transition) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, transition);

        assertEquals(errorMessage, expectedTransitions, observedTransitions);
    }

    /** Assert that a lifecycle sequence matches one of the possible variants. */
    static void assertSequenceMatchesOneOf(Class<? extends Activity> activityClass,
            LifecycleLog lifecycleLog, List<List<ActivityCallback>> expectedTransitions,
            String transition) {
        final List<ActivityCallback> observedTransitions =
                lifecycleLog.getActivityLog(activityClass);
        log("Observed sequence: " + observedTransitions);
        final String errorMessage = errorDuringTransition(activityClass, transition);

        boolean oneOfExpectedSequencesObserved = false;
        for (List<ActivityCallback> transitionVariant : expectedTransitions) {
            if (transitionVariant.equals(observedTransitions)) {
                oneOfExpectedSequencesObserved = true;
                break;
            }
        }
        assertTrue(errorMessage + "\nObserved transitions: " + observedTransitions
                        + "\nExpected one of: " + expectedTransitions,
                oneOfExpectedSequencesObserved);
    }

    private static Pair<String, ActivityCallback> transition(
            Class<? extends Activity> activityClass, ActivityCallback state) {
        return new Pair<>(activityClass.getCanonicalName(), state);
    }

    private static String errorDuringTransition(Class<? extends Activity> activityClass,
            String transition) {
        return "Failed verification during moving activity: " + activityClass.getCanonicalName()
                + " through transition: " + transition;
    }
}
