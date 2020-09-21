/*
 * Copyright (C) 2013 DroidDriver committers
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

package io.appium.droiddriver.util;

import android.app.Activity;
import android.os.Looper;
import android.support.test.runner.lifecycle.ActivityLifecycleMonitorRegistry;
import android.support.test.runner.lifecycle.Stage;
import android.util.Log;
import java.util.Iterator;
import java.util.concurrent.Callable;

/** Static helper methods for retrieving activities. */
public class ActivityUtils {
  private static final Callable<Activity> GET_RUNNING_ACTIVITY =
      new Callable<Activity>() {
        @Override
        public Activity call() {
          Iterator<Activity> activityIterator =
              ActivityLifecycleMonitorRegistry.getInstance()
                  .getActivitiesInStage(Stage.RESUMED)
                  .iterator();
          return activityIterator.hasNext() ? activityIterator.next() : null;
        }
      };
  private static Supplier<Activity> runningActivitySupplier =
      new Supplier<Activity>() {
        @Override
        public Activity get() {
          try {
            // If this is called on main (UI) thread, don't call runOnMainSync
            if (Looper.myLooper() == Looper.getMainLooper()) {
              return GET_RUNNING_ACTIVITY.call();
            }

            return InstrumentationUtils.runOnMainSyncWithTimeout(GET_RUNNING_ACTIVITY);
          } catch (Exception e) {
            Logs.log(Log.WARN, e);
            return null;
          }
        }
      };

  /**
   * Sets the Supplier for the running (a.k.a. resumed or foreground) activity. If a custom runner
   * is used, this method must be called appropriately, otherwise {@link #getRunningActivity} won't
   * work.
   */
  public static synchronized void setRunningActivitySupplier(Supplier<Activity> activitySupplier) {
    runningActivitySupplier = Preconditions.checkNotNull(activitySupplier);
  }

  /** Shorthand to {@link #getRunningActivity(long)} with {@code timeoutMillis=30_000}. */
  public static Activity getRunningActivity() {
    return getRunningActivity(30_000L);
  }

  /**
   * Waits for idle on main looper, then gets the running (a.k.a. resumed or foreground) activity.
   *
   * @return the currently running activity, or null if no activity has focus.
   */
  public static Activity getRunningActivity(long timeoutMillis) {
    // It's safe to check running activity only when the main looper is idle.
    // If the AUT is in background, its main looper should be idle already.
    // If the AUT is in foreground, its main looper should be idle eventually.
    if (InstrumentationUtils.tryWaitForIdleSync(timeoutMillis)) {
      return getRunningActivityNoWait();
    }
    return null;
  }

  /**
   * Gets the running (a.k.a. resumed or foreground) activity without waiting for idle on main
   * looper.
   *
   * @return the currently running activity, or null if no activity has focus.
   */
  public static synchronized Activity getRunningActivityNoWait() {
    return runningActivitySupplier.get();
  }

  public interface Supplier<T> {
    /**
     * Retrieves an instance of the appropriate type. The returned object may or may not be a new
     * instance, depending on the implementation.
     *
     * @return an instance of the appropriate type
     */
    T get();
  }
}
