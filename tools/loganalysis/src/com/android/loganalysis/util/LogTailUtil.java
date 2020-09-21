/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.loganalysis.util;

import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;

/**
 * A utility class for storing a part of the log for retrieval later.
 * <p>
 * Stores that last N lines in a ring buffer along with an id.  At a later time, the last X lines
 * or that last Y lines which match a given id can be retrieved.  For example, this class can be
 * used to retrieve the last 15 lines of logcat or the last 15 lines of logcat matching a given PID
 * from before when an event occurred.
 * </p>
 */
public class LogTailUtil {
    private LinkedList<LogLine> mRingBuffer = new LinkedList<LogLine>();
    private int mMaxBufferSize;
    private int mLastTailSize;
    private int mIdTailSize;

    /**
     * Store a line and id for retrieval later.
     */
    private class LogLine {
        public Integer mId = null;
        public String mLine = null;

        public LogLine(Integer id, String line) {
            mId = id;
            mLine = line;
        }
    }

    /**
     * Constructor for {@link LogTailUtil} with the default arguments.
     */
    public LogTailUtil() {
        this(500, 15, 15);
    }

    /**
     * Constructor for {@link LogTailUtil}.
     *
     * @param maxBufferSize the size of the ring buffer
     * @param lastTailSize the number of lines to retrieve when getting the last tail
     * @param idTailSize the number of lines to retrieve when getting the id tail
     */
    public LogTailUtil(int maxBufferSize, int lastTailSize, int idTailSize) {
        mMaxBufferSize = maxBufferSize;
        mLastTailSize = lastTailSize;
        mIdTailSize = idTailSize;
    }

    /**
     * Add a line to the ring buffer.
     *
     * @param id the id of the line
     * @param line the
     */
    public void addLine(Integer id, String line) {
        mRingBuffer.add(new LogLine(id, line));
        if (mRingBuffer.size() > mMaxBufferSize) {
            mRingBuffer.removeFirst();
        }
    }

    /**
     * Get the last lines of the log.
     *
     * @return The last lines of the log joined as a {@link String}.
     */
    public String getLastTail() {
        return getLastTail(mLastTailSize);
    }

    /**
     * Get the last lines of the log.
     *
     * @param size the number of lines to return.
     * @return The last {@code size} lines of the log joined as a {@link String}.
     */
    public String getLastTail(int size) {
        final int toIndex = mRingBuffer.size();
        final int fromIndex = Math.max(toIndex - size, 0);

        List<String> tail = new LinkedList<String>();
        for (LogLine line : mRingBuffer.subList(fromIndex, toIndex)) {
            tail.add(line.mLine);
        }
        return ArrayUtil.join("\n", tail).trim();
    }

    /**
     * Get the last lines of the log which match the given id.
     *
     * @param id the id of the lines to filter by
     * @return The last lines of the log joined as a {@link String}.
     */
    public String getIdTail(int id) {
        return getIdTail(id, mIdTailSize);
    }

    /**
     * Get the last lines of the log which match the given id.
     *
     * @param id the id of the lines to filter by
     * @param size the number of lines to return
     * @return The last {@code size} lines of the log joined as a {@link String}.
     */
    public String getIdTail(int id, int size) {
        LinkedList<String> tail = new LinkedList<String>();
        ListIterator<LogLine> li = mRingBuffer.listIterator(mRingBuffer.size());
        while (li.hasPrevious() && tail.size() < size) {
            LogLine line = li.previous();

            if (line.mId != null && line.mId.equals(id)) {
                tail.addFirst(line.mLine);
            }
        }
        return ArrayUtil.join("\n", tail).trim();
    }
}
