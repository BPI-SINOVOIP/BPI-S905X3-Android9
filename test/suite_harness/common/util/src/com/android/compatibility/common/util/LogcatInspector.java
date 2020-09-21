package com.android.compatibility.common.util;

import static junit.framework.TestCase.fail;

import com.google.common.base.Joiner;
import com.google.common.io.Closeables;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

/**
 * Inherit this class and implement {@link #executeShellCommand(String)} to be able to assert that
 * logcat contains what you want.
 */
public abstract class LogcatInspector {
    private static final int SMALL_LOGCAT_DELAY = 1000;

    /**
     * Should execute adb shell {@param command} and return an {@link InputStream} with the result.
     */
    protected abstract InputStream executeShellCommand(String command) throws IOException;

    /**
     * Logs an unique string using tag {@param tag} and wait until it appears to continue execution.
     *
     * @return a unique separator string.
     * @throws IOException if error while executing command.
     */
    public String mark(String tag) throws IOException {
        String uniqueString = ":::" + UUID.randomUUID().toString();
        executeShellCommand("log -t " + tag + " " + uniqueString);
        // This is to guarantee that we only return after the string has been logged, otherwise
        // in practice the case where calling Log.?(<message1>) right after clearAndMark() resulted
        // in <message1> appearing before the unique identifier. It's not guaranteed per the docs
        // that log command will have written when returning, so better be safe. 5s should be fine.
        assertLogcatContainsInOrder(tag + ":* *:S", 5, uniqueString);
        return uniqueString;
    }

    /**
     * Wait for up to {@param maxTimeoutInSeconds} for the given {@param logcatStrings} strings to
     * appear in logcat in the given order. By passing the separator returned by {@link
     * #mark(String)} as the first string you can ensure that only logs emitted after that
     * call to mark() are found. Repeated strings are not supported.
     *
     * @throws AssertionError if the strings are not found in the given time.
     * @throws IOException if error while reading.
     */
    public void assertLogcatContainsInOrder(
            String filterSpec, int maxTimeoutInSeconds, String... logcatStrings)
            throws AssertionError, IOException {
        try {
            int nextStringIndex =
                    numberOfLogcatStringsFound(filterSpec, maxTimeoutInSeconds, logcatStrings);
            if (nextStringIndex < logcatStrings.length) {
                fail(
                        "Couldn't find "
                                + logcatStrings[nextStringIndex]
                                + (nextStringIndex > 0
                                        ? " after " + logcatStrings[nextStringIndex - 1]
                                        : "")
                                + " within "
                                + maxTimeoutInSeconds
                                + " seconds ");
            }
        } catch (InterruptedException e) {
            fail("Thread interrupted unexpectedly: " + e.getMessage());
        }
    }

    /**
     * Wait for up to {@param timeInSeconds}, if all the strings {@param logcatStrings} are found in
     * order then the assertion fails, otherwise it succeeds.
     *
     * @throws AssertionError if all the strings are found in order in the given time.
     * @throws IOException if error while reading.
     */
    public void assertLogcatDoesNotContainInOrder(int timeInSeconds, String... logcatStrings)
            throws IOException {
        try {
            int stringsFound = numberOfLogcatStringsFound("", timeInSeconds, logcatStrings);
            if (stringsFound == logcatStrings.length) {
                fail("Found " + Joiner.on(", ").join(logcatStrings) + " that weren't expected");
            }
        } catch (InterruptedException e) {
            fail("Thread interrupted unexpectedly: " + e.getMessage());
        }
    }

    private int numberOfLogcatStringsFound(
            String filterSpec, int timeInSeconds, String... logcatStrings)
            throws InterruptedException, IOException {
        long timeout = System.currentTimeMillis() + TimeUnit.SECONDS.toMillis(timeInSeconds);
        int stringIndex = 0;
        while (timeout >= System.currentTimeMillis()) {
            InputStream logcatStream = executeShellCommand("logcat -v brief -d " + filterSpec);
            BufferedReader logcat = new BufferedReader(new InputStreamReader(logcatStream));
            String line;
            stringIndex = 0;
            while ((line = logcat.readLine()) != null) {
                if (line.contains(logcatStrings[stringIndex])) {
                    stringIndex++;
                    if (stringIndex >= logcatStrings.length) {
                        drainAndClose(logcat);
                        return stringIndex;
                    }
                }
            }
            Closeables.closeQuietly(logcat);
            // In case the key has not been found, wait for the log to update before
            // performing the next search.
            Thread.sleep(SMALL_LOGCAT_DELAY);
        }
        return stringIndex;
    }

    private static void drainAndClose(BufferedReader reader) {
        try {
            while (reader.read() >= 0) {
                // do nothing.
            }
        } catch (IOException ignored) {
        }
        Closeables.closeQuietly(reader);
    }
}
