/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.tradefed.log;

import com.android.ddmlib.Log;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IGlobalConfiguration;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * A logging utility class.  Useful for code that needs to override static methods from {@link Log}
 */
public class LogUtil {

    /**
     * Make uninstantiable
     */
    private LogUtil() {}

    /**
     * Sent when a log message needs to be printed.  This implementation prints the message to
     * stdout in all cases.
     *
     * @param logLevel The {@link LogLevel} enum representing the priority of the message.
     * @param tag The tag associated with the message.
     * @param message The message to display.
     */
    public static void printLog(LogLevel logLevel, String tag, String message) {
        System.out.print(LogUtil.getLogFormatString(logLevel, tag, message));
    }

    /**
     * Creates a format string that is similar to the "threadtime" log format on the device.  This
     * is specifically useful because it includes the day and month (to differentiate times for
     * long-running TF instances), and also uses 24-hour time to disambiguate morning from evening.
     * <p/>
     * @see Log#getLogFormatString(LogLevel, String, String)
     */
    public static String getLogFormatString(LogLevel logLevel, String tag, String message) {
        SimpleDateFormat formatter = new SimpleDateFormat("MM-dd HH:mm:ss");
        return String.format("%s %c/%s: %s\n", formatter.format(new Date()),
                logLevel.getPriorityLetter(), tag, message);
    }

    /**
     * A shim class for {@link Log} that automatically uses the simple classname of the caller as
     * the log tag
     */
    public static class CLog {

        protected static final String CLASS_NAME = CLog.class.getName();
        private static IGlobalConfiguration sGlobalConfig = null;

        /**
         * The shim version of {@link Log#v(String, String)}.
         *
         * @param message The {@code String} to log
         */
        public static void v(String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.v(getClassName(2), message);
        }

        /**
         * The shim version of {@link Log#v(String, String)}.  Also calls String.format for
         * convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void v(String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.v(getClassName(2), String.format(format, args));
        }

        /**
         * The shim version of {@link Log#d(String, String)}.
         *
         * @param message The {@code String} to log
         */
        public static void d(String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.d(getClassName(2), message);
        }

        /**
         * The shim version of {@link Log#d(String, String)}.  Also calls String.format for
         * convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void d(String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.d(getClassName(2), String.format(format, args));
        }

        /**
         * The shim version of {@link Log#i(String, String)}.
         *
         * @param message The {@code String} to log
         */
        public static void i(String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.i(getClassName(2), message);
        }

        /**
         * The shim version of {@link Log#i(String, String)}.  Also calls String.format for
         * convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void i(String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.i(getClassName(2), String.format(format, args));
        }

        /**
         * The shim version of {@link Log#w(String, String)}.
         *
         * @param message The {@code String} to log
         */
        public static void w(String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.w(getClassName(2), message);
        }

        /**
         * A variation of {@link Log#w(String, String)}, where the stack trace of provided
         * {@link Throwable} is formatted and logged.
         *
         * @param t The {@link Throwable} to log
         */
        public static void w(Throwable t) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.w(getClassName(2), getStackTraceString(t));
        }

        /**
         * The shim version of {@link Log#w(String, String)}.  Also calls String.format for
         * convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void w(String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.w(getClassName(2), String.format(format, args));
        }

        /**
         * The shim version of {@link Log#e(String, String)}.
         *
         * @param message The {@code String} to log
         */
        public static void e(String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.e(getClassName(2), message);
        }

        /**
         * The shim version of {@link Log#e(String, String)}.  Also calls String.format for
         * convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void e(String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.e(getClassName(2), String.format(format, args));
        }

        /**
         * The shim version of {@link Log#e(String, Throwable)}.
         *
         * @param t the {@link Throwable} to output.
         */
        public static void e(Throwable t) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.e(getClassName(2), t);
        }

        /**
         * The shim version of {@link Log#logAndDisplay(LogLevel, String, String)}.
         *
         * @param logLevel the {@link LogLevel}
         * @param message The {@code String} to log
         */
        public static void logAndDisplay(LogLevel logLevel, String message) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.logAndDisplay(logLevel, getClassName(2), message);
        }

        /**
         * The shim version of {@link Log#logAndDisplay(LogLevel, String, String)}.
         *
         * @param logLevel the {@link LogLevel}
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void logAndDisplay(LogLevel logLevel, String format, Object... args) {
            // frame 2: skip frames 0 (#getClassName) and 1 (this method)
            Log.logAndDisplay(logLevel, getClassName(2), String.format(format, args));
        }

        /**
         * What a Terrible Failure: Report a condition that should never happen.
         * The error will always be logged at level ASSERT with the call stack.
         *
         * @param message The message you would like logged.
         */
        public static void wtf(String message) {
            wtf(message, (Throwable) null);
        }

        /**
         * What a Terrible Failure: Report a condition that should never happen.
         * The error will always be logged at level ASSERT with the call stack.
         *
         * @param t (Optional) An exception to log. If null, only message will be logged.
         */
        public static void wtf(Throwable t) {
            wtf(t.getMessage(), t);
        }

        /**
         * What a Terrible Failure: Report a condition that should never happen.
         * The error will always be logged at level ASSERT with the call stack.
         * Also calls String.format for convenience.
         *
         * @param format A format string for the message to log
         * @param args The format string arguments
         */
        public static void wtf(String format, Object... args) {
            wtf(String.format(format, args), (Throwable) null);
        }

        /**
         * What a Terrible Failure: Report a condition that should never happen.
         * The error will always be logged at level ASSERT with the call stack.
         *
         * @param message The message you would like logged.
         * @param t (Optional) An exception to log. If null, only message will be logged.
         */
        public static void wtf(String message, Throwable t) {
            ITerribleFailureHandler wtfHandler = getGlobalConfigInstance().getWtfHandler();

            /* since wtf(String, Throwable) can be called directly or through an overloaded
             * method, ie wtf(String), the stack trace frame of the external class name that
             * called CLog can vary, so we use findCallerClassName to find it */
            String tag = findCallerClassName();
            String logMessage = "WTF - " + message;
            String stackTrace = getStackTraceString(t);
            if (stackTrace.length() > 0) {
               logMessage += "\n" + stackTrace;
            }

            Log.logAndDisplay(LogLevel.ASSERT, tag, logMessage);
            if (wtfHandler != null) {
                wtfHandler.onTerribleFailure(message, t);
            }
        }

        /**
         * Sets the GlobalConfiguration instance for CLog to use - exposed for unit testing
         *
         * @param globalConfig the GlobalConfiguration object for CLog to use
         */
        // @VisibleForTesting
        public static void setGlobalConfigInstance(IGlobalConfiguration globalConfig) {
            sGlobalConfig = globalConfig;
        }

        /**
         * Gets the GlobalConfiguration instance, useful for unit testing
         *
         * @return the GlobalConfiguration singleton instance
         */
        private static IGlobalConfiguration getGlobalConfigInstance() {
            if (sGlobalConfig == null) {
                sGlobalConfig = GlobalConfiguration.getInstance();
            }
            return sGlobalConfig;
        }

        /**
         * A helper method that parses the stack trace string out of the
         * throwable.
         *
         * @param t contains the stack trace information
         * @return A {@link String} containing the stack trace of the throwable.
         */
        private static String getStackTraceString(Throwable t) {
            if (t == null) {
                return "";
            }

            StringWriter sw = new StringWriter();
            PrintWriter pw = new PrintWriter(sw);
            t.printStackTrace(pw);
            pw.flush();
            return sw.toString();
        }

        /**
         * Return the simple classname from the {@code frame}th stack frame in the call path.
         * Note: this method does <emph>not</emph> check array bounds for the stack trace length.
         *
         * @param frame The index of the stack trace frame to inspect for the class name
         * @return The simple class name (or full-qualified if an error occurs getting a ref to the
         *         class) for the given element of the stack trace.
         */
        public static String getClassName(int frame) {
            StackTraceElement[] frames = (new Throwable()).getStackTrace();
            return parseClassName(frames[frame].getClassName());
        }

        /**
         * Finds the external class name that directly called a CLog method.
         *
         * @return The simple class name (or full-qualified if an error occurs getting a ref to
         *         the class) of the external class that called a CLog method, or "Unknown" if
         *         the stack trace is empty or only contains CLog class names.
         */
        public static String findCallerClassName() {
            return findCallerClassName(null);
        }

        /**
         * Finds the external class name that directly called a CLog method.
         *
         * @param t (Optional) the stack trace to search within, exposed for unit testing
         * @return The simple class name (or full-qualified if an error occurs getting a ref to
         *         the class) of the external class that called a CLog method, or "Unknown" if
         *         the stack trace is empty or only contains CLog class names.
         */
        public static String findCallerClassName(Throwable t) {
            String className = "Unknown";

            if (t == null) {
                t = new Throwable();
            }
            StackTraceElement[] frames = t.getStackTrace();
            if (frames.length == 0) {
                return className;
            }

            // starting with the first frame's class name (this CLog class)
            // keep iterating until a frame of a different class name is found
            int f;
            for (f = 0; f < frames.length; f++) {
                className = frames[f].getClassName();
                if (!className.equals(CLASS_NAME)) {
                    break;
                }
            }

            return parseClassName(className);
        }

        /**
         * Parses the simple class name out of the full class name. If the formatting already
         * looks like a simple class name, then just returns that.
         *
         * @param fullName the full class name to parse
         * @return The simple class name
         */
        // @VisibleForTesting
        static String parseClassName(String fullName) {
            int lastdot = fullName.lastIndexOf('.');
            String simpleName = fullName;
            if (lastdot != -1) {
                simpleName = fullName.substring(lastdot + 1);
            }
            // handle inner class names
            int lastdollar = simpleName.lastIndexOf('$');
            if (lastdollar != -1) {
                simpleName = simpleName.substring(0, lastdollar);
            }
            return simpleName;
        }
    }
}
