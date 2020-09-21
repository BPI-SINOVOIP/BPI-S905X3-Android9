/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.app.cts.android.app.cts.tools;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Instrumentation;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.util.Log;

import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.regex.Pattern;

/**
 * bit CtsAppTestCases:ActivityManagerProcessStateTest
 */
public class WatchUidRunner {
    static final String TAG = "WatchUidRunner";

    public static final int CMD_PROCSTATE = 0;
    public static final int CMD_ACTIVE = 1;
    public static final int CMD_IDLE = 2;
    public static final int CMD_UNCACHED = 3;
    public static final int CMD_CACHED = 4;
    public static final int CMD_GONE = 5;

    public static final String STATE_PERSISTENT = "PER";
    public static final String STATE_PERSISTENT_UI = "PERU";
    public static final String STATE_TOP = "TOP";
    public static final String STATE_BOUND_FG_SERVICE = "BFGS";
    public static final String STATE_FG_SERVICE = "FGS";
    public static final String STATE_TOP_SLEEPING = "TPSL";
    public static final String STATE_IMPORTANT_FG = "IMPF";
    public static final String STATE_IMPORTANT_BG = "IMPB";
    public static final String STATE_TRANSIENT_BG = "TRNB";
    public static final String STATE_BACKUP = "BKUP";
    public static final String STATE_HEAVY_WEIGHT = "HVY";
    public static final String STATE_SERVICE = "SVC";
    public static final String STATE_RECEIVER = "RCVR";
    public static final String STATE_HOME = "HOME";
    public static final String STATE_LAST = "LAST";
    public static final String STATE_CACHED_ACTIVITY = "CAC";
    public static final String STATE_CACHED_ACTIVITY_CLIENT = "CACC";
    public static final String STATE_CACHED_RECENT = "CRE";
    public static final String STATE_CACHED_EMPTY = "CEM";
    public static final String STATE_NONEXISTENT = "NONE";

    static final String[] COMMAND_TO_STRING = new String[] {
            "procstate", "active", "idle", "uncached", "cached", "gone"
    };

    final Instrumentation mInstrumentation;
    final int mUid;
    final String mUidStr;
    final long mDefaultWaitTime;
    final Pattern mSpaceSplitter;
    final ParcelFileDescriptor mReadFd;
    final FileInputStream mReadStream;
    final BufferedReader mReadReader;
    final ParcelFileDescriptor mWriteFd;
    final FileOutputStream mWriteStream;
    final PrintWriter mWritePrinter;
    final Thread mReaderThread;

    // Shared state is protected by this.
    final ArrayList<String[]> mPendingLines = new ArrayList<>();

    boolean mStopping;

    public WatchUidRunner(Instrumentation instrumentation, int uid) {
        this(instrumentation, uid, 5*1000);
    }

    public WatchUidRunner(Instrumentation instrumentation, int uid, long defaultWaitTime) {
        mInstrumentation = instrumentation;
        mUid = uid;
        mUidStr = Integer.toString(uid);
        mDefaultWaitTime = defaultWaitTime;
        mSpaceSplitter = Pattern.compile("\\s+");
        ParcelFileDescriptor[] pfds = instrumentation.getUiAutomation().executeShellCommandRw(
                "am watch-uids --oom " + uid);
        mReadFd = pfds[0];
        mReadStream = new ParcelFileDescriptor.AutoCloseInputStream(mReadFd);
        mReadReader = new BufferedReader(new InputStreamReader(mReadStream));
        mWriteFd = pfds[1];
        mWriteStream = new ParcelFileDescriptor.AutoCloseOutputStream(mWriteFd);
        mWritePrinter = new PrintWriter(new BufferedOutputStream(mWriteStream));
        // Executing a shell command is asynchronous but we can't proceed further with the test
        // until the 'watch-uids' cmd is executed.
        waitUntilUidObserverReady();
        mReaderThread = new ReaderThread();
        mReaderThread.start();
    }

    private void waitUntilUidObserverReady() {
        try {
            final String line = mReadReader.readLine();
            assertTrue("Unexpected output: " + line, line.startsWith("Watching uid states"));
        } catch (IOException e) {
            fail("Error occurred " + e);
        }
    }

    public void expect(int cmd, String procState) {
        expect(cmd, procState, mDefaultWaitTime);
    }

    public void expect(int cmd, String procState, long timeout) {
        long waitUntil = SystemClock.uptimeMillis() + timeout;
        String[] line = waitForNextLine(waitUntil, cmd, procState);
        if (!COMMAND_TO_STRING[cmd].equals(line[1])) {
            String msg = "Expected cmd " + COMMAND_TO_STRING[cmd]
                    + " uid " + mUid + " but next report was " + Arrays.toString(line);
            Log.d(TAG, msg);
            logRemainingLines();
            throw new IllegalStateException(msg);
        }
        if (procState != null && (line.length < 3 || !procState.equals(line[2]))) {
            String msg = "Expected procstate " + procState
                    + " uid " + mUid + " but next report was " + Arrays.toString(line);
            Log.d(TAG, msg);
            logRemainingLines();
            throw new IllegalStateException(msg);
        }
        Log.d(TAG, "Got expected: " + Arrays.toString(line));
    }

    public void waitFor(int cmd, String procState) {
        waitFor(cmd, procState, mDefaultWaitTime);
    }

    public void waitFor(int cmd, String procState, long timeout) {
        long waitUntil = SystemClock.uptimeMillis() + timeout;
        while (true) {
            String[] line = waitForNextLine(waitUntil, cmd, procState);
            if (COMMAND_TO_STRING[cmd].equals(line[1])) {
                if (procState == null) {
                    Log.d(TAG, "Waited for: " + Arrays.toString(line));
                    return;
                }
                if (line.length >= 3 && procState.equals(line[2])) {
                    Log.d(TAG, "Waited for: " + Arrays.toString(line));
                    return;
                } else {
                    Log.d(TAG, "Skipping because procstate not " + procState + ": "
                            + Arrays.toString(line));
                }
            } else {
                Log.d(TAG, "Skipping because not " + COMMAND_TO_STRING[cmd] + ": "
                        + Arrays.toString(line));
            }
        }
    }

    void logRemainingLines() {
        synchronized (mPendingLines) {
            while (mPendingLines.size() > 0) {
                String[] res = mPendingLines.remove(0);
                if (res[0].startsWith("#")) {
                    Log.d(TAG, "Remaining: " + res[0]);
                } else {
                    Log.d(TAG, "Remaining: " + Arrays.toString(res));
                }
            }
        }
    }

    String[] waitForNextLine(long waitUntil, int cmd, String procState) {
        synchronized (mPendingLines) {
            while (true) {
                while (mPendingLines.size() == 0) {
                    long now = SystemClock.uptimeMillis();
                    if (now >= waitUntil) {
                        String msg = "Timed out waiting for next line: "
                                + "cmd=" + COMMAND_TO_STRING[cmd] + " procState=" + procState;
                        Log.d(TAG, msg);
                        throw new IllegalStateException(msg);
                    }
                    try {
                        mPendingLines.wait(waitUntil - now);
                    } catch (InterruptedException e) {
                    }
                }
                String[] res = mPendingLines.remove(0);
                if (res[0].startsWith("#")) {
                    Log.d(TAG, "Note: " + res[0]);
                } else {
                    return res;
                }
            }
        }
    }

    public void finish() {
        synchronized (mPendingLines) {
            mStopping = true;
        }
        mWritePrinter.println("q");
        try {
            mWriteStream.close();
        } catch (IOException e) {
        }
        try {
            mReadStream.close();
        } catch (IOException e) {
        }
    }

    final class ReaderThread extends Thread {
        String mLastReadLine;

        @Override
        public void run() {
            String[] line;
            try {
                while ((line = readNextLine()) != null) {
                    boolean comment = line.length == 1 && line[0].startsWith("#");
                    if (!comment) {
                        if (line.length < 2) {
                            Log.d(TAG, "Skipping too short: " + mLastReadLine);
                            continue;
                        }
                        if (!line[0].equals(mUidStr)) {
                            Log.d(TAG, "Skipping ignored uid: " + mLastReadLine);
                            continue;
                        }
                    }
                    //Log.d(TAG, "Enqueueing: " + mLastReadLine);
                    synchronized (mPendingLines) {
                        if (mStopping) {
                            return;
                        }
                        mPendingLines.add(line);
                        mPendingLines.notifyAll();
                    }
                }
            } catch (IOException e) {
                Log.w(TAG, "Failed reading", e);
            }
        }

        String[] readNextLine() throws IOException {
            mLastReadLine = mReadReader.readLine();
            if (mLastReadLine == null) {
                return null;
            }
            if (mLastReadLine.startsWith("#")) {
                return new String[] { mLastReadLine };
            }
            return mSpaceSplitter.split(mLastReadLine);
        }
    }
}
