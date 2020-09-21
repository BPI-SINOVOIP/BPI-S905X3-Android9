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

import android.text.TextUtils;
import android.util.Log;

/**
 * Internal helper for logging.
 */
public class Logs {
  public static final String TAG = "DroidDriver";

  public static void call(Object self, String method, Object... args) {
    call(Log.DEBUG, self, method, args);
  }

  public static void call(int priority, Object self, String method, Object... args) {
    if (Log.isLoggable(TAG, priority)) {
      Log.d(
          TAG,
          String.format("Invoking %s.%s(%s)", self.getClass().getSimpleName(), method,
              TextUtils.join(", ", args)));
    }
  }

  public static void log(int priority, String msg) {
    if (Log.isLoggable(TAG, priority)) {
      Log.println(priority, TAG, msg);
    }
  }

  public static void log(int priority, Throwable e) {
    if (Log.isLoggable(TAG, priority)) {
      Log.println(priority, TAG, Log.getStackTraceString(e));
    }
  }

  public static void log(int priority, Throwable e, String msg) {
    if (Log.isLoggable(TAG, priority)) {
      Log.println(priority, TAG, msg + '\n' + Log.getStackTraceString(e));
    }
  }

  public static void logfmt(int priority, String format, Object... args) {
    if (Log.isLoggable(TAG, priority)) {
      Log.println(priority, TAG, String.format(format, args));
    }
  }

  public static void logfmt(int priority, Throwable e, String format, Object... args) {
    if (Log.isLoggable(TAG, priority)) {
      Log.println(priority, TAG, String.format(format, args) + '\n' + Log.getStackTraceString(e));
    }
  }

  private Logs() {}
}
