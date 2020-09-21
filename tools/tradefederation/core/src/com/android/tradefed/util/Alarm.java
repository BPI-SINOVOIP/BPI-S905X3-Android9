/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.android.tradefed.util;

import java.io.IOException;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

/**
 * A thread which waits for a period of time and then interrupts a specific other thread.
 * Can call {@link Thread#interrupt()} to get a thread out of a blocking wait, or
 * {@link Socket#close()} to stop a thread from blocking on a socket read or write.
 * <p/>
 * All time units are in milliseconds.
 */
public class Alarm extends Thread {
    private final List<Thread> mInterruptThreads = new ArrayList<Thread>();
    private final List<Socket> mInterruptSockets = new ArrayList<Socket>();
    private final long mTimeoutTime;
    private boolean mAlarmFired = false;

    /**
     * Constructor takes the amount of time to wait, in millis.
     *
     * @param timeout The amount of time to wait, in millis
     * @throws IllegalArgumentException if {@code timeout <= 0}.
     */
    public Alarm(long timeout) {
        setName(getClass().getCanonicalName());
        setDaemon(true);

        if (timeout <= 0) {
            throw new IllegalArgumentException(String.format(
                    "Alarm timeout time %d <= 0, which is not valid.", timeout));
        }

        mTimeoutTime = timeout;
    }

    public void addThread(Thread intThread) {
        mInterruptThreads.add(intThread);
    }

    public void addSocket(Socket intSocket) {
        mInterruptSockets.add(intSocket);
    }

    public boolean didAlarmFire() {
        return mAlarmFired;
    }

    @Override
    public void run() {
        try {
            Thread.sleep(mTimeoutTime);
        } catch (InterruptedException e) {
            // Expected; return without interrupt()ing any of our InterruptThreads
            return;
        }
        mAlarmFired = true;
        for (Socket sock : mInterruptSockets) {
            try {
                sock.close();
            } catch (IOException e) {
                // ignore
            }
        }
        for (Thread thread : mInterruptThreads) {
            thread.interrupt();
        }
    }
}

