/*
 * Copyright (C) 2015 DroidDriver committers
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

import android.app.Instrumentation;
import android.content.Context;
import android.os.Bundle;
import android.os.Looper;
import android.support.test.InstrumentationRegistry;
import android.util.Log;
import io.appium.droiddriver.exceptions.DroidDriverException;
import io.appium.droiddriver.exceptions.TimeoutException;
import java.util.concurrent.Callable;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

/** Static utility methods pertaining to {@link Instrumentation}. */
public class InstrumentationUtils {
  private static final Runnable EMPTY_RUNNABLE =
      new Runnable() {
        @Override
        public void run() {}
      };
  private static final Executor RUN_ON_MAIN_SYNC_EXECUTOR = Executors.newSingleThreadExecutor();
  private static Instrumentation instrumentation;
  private static Bundle options;
  private static long runOnMainSyncTimeoutMillis;

  /**
   * Initializes this class. If you don't use android.support.test.runner.AndroidJUnitRunner or a
   * runner that supports {link InstrumentationRegistry}, you need to call this method
   * appropriately.
   */
  public static synchronized void init(Instrumentation instrumentation, Bundle arguments) {
    if (InstrumentationUtils.instrumentation != null) {
      throw new DroidDriverException("init() can only be called once");
    }
    InstrumentationUtils.instrumentation = instrumentation;
    options = arguments;

    String timeoutString = getD2Option("runOnMainSyncTimeout");
    runOnMainSyncTimeoutMillis = timeoutString == null ? 10_000L : Long.parseLong(timeoutString);
  }

  private static synchronized void checkInitialized() {
    if (instrumentation == null) {
      // Assume android.support.test.runner.InstrumentationRegistry is valid
      init(InstrumentationRegistry.getInstrumentation(), InstrumentationRegistry.getArguments());
    }
  }

  public static Instrumentation getInstrumentation() {
    checkInitialized();
    return instrumentation;
  }

  public static Context getTargetContext() {
    return getInstrumentation().getTargetContext();
  }

  /**
   * Gets the <a href=
   * "http://developer.android.com/tools/testing/testing_otheride.html#AMOptionsSyntax" >am
   * instrument options</a>.
   */
  public static Bundle getOptions() {
    checkInitialized();
    return options;
  }

  /** Gets the string value associated with the given key. */
  public static String getOption(String key) {
    return getOptions().getString(key);
  }

  /**
   * Calls {@link #getOption} with "dd." prefixed to {@code key}. This is for DroidDriver
   * implementation to use a consistent pattern for its options.
   */
  public static String getD2Option(String key) {
    return getOption("dd." + key);
  }

  /**
   * Tries to wait for an idle state on the main thread on best-effort basis up to {@code
   * timeoutMillis}. The main thread may not enter the idle state when animation is playing, for
   * example, the ProgressBar.
   */
  public static boolean tryWaitForIdleSync(long timeoutMillis) {
    checkNotMainThread();
    FutureTask<Void> emptyTask = new FutureTask<Void>(EMPTY_RUNNABLE, null);
    getInstrumentation().waitForIdle(emptyTask);

    try {
      emptyTask.get(timeoutMillis, TimeUnit.MILLISECONDS);
    } catch (java.util.concurrent.TimeoutException e) {
      Logs.log(
          Log.INFO,
          "Timed out after " + timeoutMillis + " milliseconds waiting for idle on main looper");
      return false;
    } catch (Throwable t) {
      throw DroidDriverException.propagate(t);
    }
    return true;
  }

  public static void runOnMainSyncWithTimeout(final Runnable runnable) {
    runOnMainSyncWithTimeout(
        new Callable<Void>() {
          @Override
          public Void call() throws Exception {
            runnable.run();
            return null;
          }
        });
  }

  /**
   * Runs {@code callable} on the main thread on best-effort basis up to a time limit, which
   * defaults to {@code 10000L} and can be set as an am instrument option under the key {@code
   * dd.runOnMainSyncTimeout}.
   *
   * <p>This is a safer variation of {@link Instrumentation#runOnMainSync} because the latter may
   * hang. You may turn off this behavior by setting {@code "-e dd.runOnMainSyncTimeout 0"} on the
   * am command line.The {@code callable} may never run, for example, if the main Looper has exited
   * due to uncaught exception.
   */
  public static <V> V runOnMainSyncWithTimeout(Callable<V> callable) {
    checkNotMainThread();
    final RunOnMainSyncFutureTask<V> futureTask = new RunOnMainSyncFutureTask<>(callable);

    if (runOnMainSyncTimeoutMillis <= 0L) {
      // Call runOnMainSync on current thread without time limit.
      futureTask.runOnMainSyncNoThrow();
    } else {
      RUN_ON_MAIN_SYNC_EXECUTOR.execute(
          new Runnable() {
            @Override
            public void run() {
              futureTask.runOnMainSyncNoThrow();
            }
          });
    }

    try {
      return futureTask.get(runOnMainSyncTimeoutMillis, TimeUnit.MILLISECONDS);
    } catch (java.util.concurrent.TimeoutException e) {
      throw new TimeoutException(
          "Timed out after "
              + runOnMainSyncTimeoutMillis
              + " milliseconds waiting for Instrumentation.runOnMainSync",
          e);
    } catch (Throwable t) {
      throw DroidDriverException.propagate(t);
    } finally {
      futureTask.cancel(false);
    }
  }

  public static void checkMainThread() {
    if (Looper.myLooper() != Looper.getMainLooper()) {
      throw new DroidDriverException("This method must be called on the main thread");
    }
  }

  public static void checkNotMainThread() {
    if (Looper.myLooper() == Looper.getMainLooper()) {
      throw new DroidDriverException("This method cannot be called on the main thread");
    }
  }

  private static class RunOnMainSyncFutureTask<V> extends FutureTask<V> {
    public RunOnMainSyncFutureTask(Callable<V> callable) {
      super(callable);
    }

    public void runOnMainSyncNoThrow() {
      try {
        getInstrumentation().runOnMainSync(this);
      } catch (Throwable e) {
        setException(e);
      }
    }
  }
}
