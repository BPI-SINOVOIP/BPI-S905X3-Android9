package android.server.am.lifecycle;

import static org.junit.Assert.fail;

import android.app.Activity;
import android.server.am.lifecycle.LifecycleLog.ActivityCallback;
import android.support.test.runner.lifecycle.ActivityLifecycleCallback;
import android.support.test.runner.lifecycle.Stage;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;
import java.util.function.BooleanSupplier;

/**
 * Gets notified about activity lifecycle updates and provides blocking mechanism to wait until
 * expected activity states are reached.
 */
public class LifecycleTracker implements ActivityLifecycleCallback {

    private LifecycleLog mLifecycleLog;

    LifecycleTracker(LifecycleLog lifecycleLog) {
        mLifecycleLog = lifecycleLog;
    }

    void waitAndAssertActivityStates(Pair<Activity, ActivityCallback>[] activityCallbacks) {
        final boolean waitResult = waitForConditionWithTimeout(
                () -> pendingCallbacks(activityCallbacks).isEmpty(), 5 * 1000);

        if (!waitResult) {
            fail("Expected lifecycle states not achieved: " + pendingCallbacks(activityCallbacks));
        }
    }

    /**
     * Waits for a specific sequence of events to happen.
     * When there is a possibility of some lifecycle state happening more than once in a sequence,
     * it is better to use this method instead of {@link #waitAndAssertActivityStates(Pair[])}.
     * Otherwise we might stop tracking too early.
     */
    void waitForActivityTransitions(Class<? extends Activity> activityClass,
            List<ActivityCallback> expectedTransitions) {
        waitForConditionWithTimeout(
                () -> mLifecycleLog.getActivityLog(activityClass).equals(expectedTransitions),
                5 * 1000);
    }

    @Override
    synchronized public void onActivityLifecycleChanged(Activity activity, Stage stage) {
        notify();
    }

    /** Get a list of activity states that were not reached yet. */
    private List<Pair<Activity, ActivityCallback>> pendingCallbacks(Pair<Activity,
            ActivityCallback>[] activityCallbacks) {
        final List<Pair<Activity, ActivityCallback>> notReachedActivityCallbacks = new ArrayList<>();

        for (Pair<Activity, ActivityCallback> activityCallback : activityCallbacks) {
            final Activity activity = activityCallback.first;
            final List<ActivityCallback> transitionList =
                    mLifecycleLog.getActivityLog(activity.getClass());
            if (transitionList.isEmpty()
                    || transitionList.get(transitionList.size() - 1) != activityCallback.second) {
                // The activity either hasn't got any state transitions yet or the current state is
                // not the one we expect.
                notReachedActivityCallbacks.add(activityCallback);
            }
        }
        return notReachedActivityCallbacks;
    }

    /** Blocking call to wait for a condition to become true with max timeout. */
    synchronized private boolean waitForConditionWithTimeout(BooleanSupplier waitCondition,
            long timeoutMs) {
        final long timeout = System.currentTimeMillis() + timeoutMs;
        while (!waitCondition.getAsBoolean()) {
            final long waitMs = timeout - System.currentTimeMillis();
            if (waitMs <= 0) {
                // Timeout expired.
                return false;
            }
            try {
                wait(timeoutMs);
            } catch (InterruptedException e) {
                // Weird, let's retry.
            }
        }
        return true;
    }
}
