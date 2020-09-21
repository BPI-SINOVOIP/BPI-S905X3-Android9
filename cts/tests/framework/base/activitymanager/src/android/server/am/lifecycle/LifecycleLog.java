package android.server.am.lifecycle;

import static android.server.am.StateLogger.log;
import static android.support.test.runner.lifecycle.Stage.CREATED;
import static android.support.test.runner.lifecycle.Stage.DESTROYED;
import static android.support.test.runner.lifecycle.Stage.PAUSED;
import static android.support.test.runner.lifecycle.Stage.PRE_ON_CREATE;
import static android.support.test.runner.lifecycle.Stage.RESUMED;
import static android.support.test.runner.lifecycle.Stage.STARTED;
import static android.support.test.runner.lifecycle.Stage.STOPPED;

import android.app.Activity;
import android.support.test.runner.lifecycle.ActivityLifecycleCallback;
import android.support.test.runner.lifecycle.Stage;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;

/**
 * Used as a shared log storage of activity lifecycle transitions. Methods must be synchronized to
 * prevent concurrent modification of the log store.
 */
class LifecycleLog implements ActivityLifecycleCallback {

    enum ActivityCallback {
        PRE_ON_CREATE,
        ON_CREATE,
        ON_START,
        ON_RESUME,
        ON_PAUSE,
        ON_STOP,
        ON_RESTART,
        ON_DESTROY,
        ON_ACTIVITY_RESULT,
        ON_POST_CREATE,
        ON_NEW_INTENT,
        ON_MULTI_WINDOW_MODE_CHANGED
    }

    /**
     * Log for encountered activity callbacks. Note that methods accessing or modifying this
     * list should be synchronized as it can be accessed from different threads.
     */
    private List<Pair<String, ActivityCallback>> mLog = new ArrayList<>();

    /** Clear the entire transition log. */
    synchronized void clear() {
        mLog.clear();
    }

    /** Add transition of an activity to the log. */
    @Override
    synchronized public void onActivityLifecycleChanged(Activity activity, Stage stage) {
        final String activityName = activity.getClass().getCanonicalName();
        log("Activity " + activityName + " moved to stage " + stage);
        mLog.add(new Pair<>(activityName, stageToCallback(stage)));
    }

    /** Add activity callback to the log. */
    synchronized public void onActivityCallback(Activity activity, ActivityCallback callback) {
        final String activityName = activity.getClass().getCanonicalName();
        log("Activity " + activityName + " got a callback " + callback);
        mLog.add(new Pair<>(activityName, callback));
    }

    /** Get logs for all recorded transitions. */
    synchronized List<Pair<String, ActivityCallback>> getLog() {
        // Wrap in a new list to prevent concurrent modification
        return new ArrayList<>(mLog);
    }

    /** Get transition logs for the specified activity. */
    synchronized List<ActivityCallback> getActivityLog(Class<? extends Activity> activityClass) {
        final String activityName = activityClass.getCanonicalName();
        log("Looking up log for activity: " + activityName);
        final List<ActivityCallback> activityLog = new ArrayList<>();
        for (Pair<String, ActivityCallback> transition : mLog) {
            if (transition.first.equals(activityName)) {
                activityLog.add(transition.second);
            }
        }
        return activityLog;
    }

    private static ActivityCallback stageToCallback(Stage stage) {
        switch (stage) {
            case PRE_ON_CREATE:
                return ActivityCallback.PRE_ON_CREATE;
            case CREATED:
                return ActivityCallback.ON_CREATE;
            case STARTED:
                return ActivityCallback.ON_START;
            case RESUMED:
                return ActivityCallback.ON_RESUME;
            case PAUSED:
                return ActivityCallback.ON_PAUSE;
            case STOPPED:
                return ActivityCallback.ON_STOP;
            case RESTARTED:
                return ActivityCallback.ON_RESTART;
            case DESTROYED:
                return ActivityCallback.ON_DESTROY;
            default:
                throw new IllegalArgumentException("Unsupported stage: " + stage);
        }
    }
}
