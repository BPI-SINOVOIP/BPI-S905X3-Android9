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

package com.android.cts.mockime;

import android.os.Bundle;
import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import android.view.inputmethod.EditorInfo;

import java.time.Instant;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.Optional;
import java.util.function.Predicate;
import java.util.function.Supplier;

/**
 * A utility class that provides basic query operations and wait primitives for a series of
 * {@link ImeEvent} sent from the {@link MockIme}.
 *
 * <p>All public methods are not thread-safe.</p>
 */
public final class ImeEventStream {

    private static final String LONG_LONG_SPACES = "                                        ";

    private static DateTimeFormatter sSimpleDateTimeFormatter =
            DateTimeFormatter.ofPattern("MM-dd HH:mm:ss.SSS").withZone(ZoneId.systemDefault());

    @NonNull
    private final Supplier<ImeEventArray> mEventSupplier;
    private int mCurrentPosition;

    ImeEventStream(@NonNull Supplier<ImeEventArray> supplier) {
        this(supplier, 0 /* position */);
    }

    private ImeEventStream(@NonNull Supplier<ImeEventArray> supplier, int position) {
        mEventSupplier = supplier;
        mCurrentPosition = position;
    }

    /**
     * Create a copy that starts from the same event position of this stream. Once a copy is created
     * further event position change on this stream will not affect the copy.
     *
     * @return A new copy of this stream
     */
    public ImeEventStream copy() {
        return new ImeEventStream(mEventSupplier, mCurrentPosition);
    }

    /**
     * Advances the current event position by skipping events.
     *
     * @param length number of events to be skipped
     * @throws IllegalArgumentException {@code length} is negative
     */
    public void skip(@IntRange(from = 0) int length) {
        if (length < 0) {
            throw new IllegalArgumentException("length cannot be negative: " + length);
        }
        mCurrentPosition += length;
    }

    /**
     * Advances the current event position to the next to the last position.
     */
    public void skipAll() {
        mCurrentPosition = mEventSupplier.get().mLength;
    }

    /**
     * Find the first event that matches the given condition from the current position.
     *
     * <p>If there is such an event, this method returns such an event without moving the current
     * event position.</p>
     *
     * <p>If there is such an event, this method returns {@link Optional#empty()} without moving the
     * current event position.</p>
     *
     * @param condition the event condition to be matched
     * @return {@link Optional#empty()} if there is no such an event. Otherwise the matched event is
     *         returned
     */
    @NonNull
    public Optional<ImeEvent> findFirst(Predicate<ImeEvent> condition) {
        final ImeEventArray latest = mEventSupplier.get();
        int index = mCurrentPosition;
        while (true) {
            if (index >= latest.mLength) {
                return Optional.empty();
            }
            if (condition.test(latest.mArray[index])) {
                return Optional.of(latest.mArray[index]);
            }
            ++index;
        }
    }

    /**
     * Find the first event that matches the given condition from the current position.
     *
     * <p>If there is such an event, this method returns such an event and set the current event
     * position to that event.</p>
     *
     * <p>If there is such an event, this method returns {@link Optional#empty()} without moving the
     * current event position.</p>
     *
     * @param condition the event condition to be matched
     * @return {@link Optional#empty()} if there is no such an event. Otherwise the matched event is
     *         returned
     */
    @NonNull
    public Optional<ImeEvent> seekToFirst(Predicate<ImeEvent> condition) {
        final ImeEventArray latest = mEventSupplier.get();
        while (true) {
            if (mCurrentPosition >= latest.mLength) {
                return Optional.empty();
            }
            if (condition.test(latest.mArray[mCurrentPosition])) {
                return Optional.of(latest.mArray[mCurrentPosition]);
            }
            ++mCurrentPosition;
        }
    }

    private static void dumpEvent(@NonNull StringBuilder sb, @NonNull ImeEvent event,
            boolean fused) {
        final String indentation = getWhiteSpaces(event.getNestLevel() * 2 + 2);
        final long wallTime =
                fused ? event.getEnterWallTime() :
                        event.isEnterEvent() ? event.getEnterWallTime() : event.getExitWallTime();
        sb.append(sSimpleDateTimeFormatter.format(Instant.ofEpochMilli(wallTime)))
                .append("  ")
                .append(String.format("%5d", event.getThreadId()))
                .append(indentation);
        sb.append(fused ? "" : event.isEnterEvent() ? "[" : "]");
        if (fused || event.isEnterEvent()) {
            sb.append(event.getEventName())
                    .append(':')
                    .append(" args=");
            dumpBundle(sb, event.getArguments());
        }
        sb.append('\n');
    }

    /**
     * @return Debug info as a {@link String}.
     */
    public String dump() {
        final ImeEventArray latest = mEventSupplier.get();
        final StringBuilder sb = new StringBuilder();
        sb.append("ImeEventStream:\n");
        sb.append("  latest: array[").append(latest.mArray.length).append("] + {\n");
        for (int i = 0; i < latest.mLength; ++i) {
            // To compress the dump message, if the current event is an enter event and the next
            // one is a corresponding exit event, we unify the output.
            final boolean fused = areEnterExitPairedMessages(latest, i);
            if (i == mCurrentPosition || (fused && ((i + 1) == mCurrentPosition))) {
                sb.append("  ======== CurrentPosition ========  \n");
            }
            dumpEvent(sb, latest.mArray[fused ? ++i : i], fused);
        }
        if (mCurrentPosition >= latest.mLength) {
            sb.append("  ======== CurrentPosition ========  \n");
        }
        sb.append("}\n");
        return sb.toString();
    }

    /**
     * @param array event array to be checked
     * @param i index to be checked
     * @return {@code true} if {@code array.mArray[i]} and {@code array.mArray[i + 1]} are two
     *         paired events.
     */
    private static boolean areEnterExitPairedMessages(@NonNull ImeEventArray array,
            @IntRange(from = 0) int i) {
        return array.mArray[i] != null
                && array.mArray[i].isEnterEvent()
                && (i + 1) < array.mLength
                && array.mArray[i + 1] != null
                && array.mArray[i].getEventName().equals(array.mArray[i + 1].getEventName())
                && array.mArray[i].getEnterTimestamp() == array.mArray[i + 1].getEnterTimestamp();
    }

    /**
     * @param length length of the requested white space string
     * @return {@link String} object whose length is {@code length}
     */
    private static String getWhiteSpaces(@IntRange(from = 0) final int length) {
        if (length < LONG_LONG_SPACES.length()) {
            return LONG_LONG_SPACES.substring(0, length);
        }
        final char[] indentationChars = new char[length];
        Arrays.fill(indentationChars, ' ');
        return new String(indentationChars);
    }

    private static void dumpBundle(@NonNull StringBuilder sb, @NonNull Bundle bundle) {
        sb.append('{');
        boolean first = true;
        for (String key : bundle.keySet()) {
            if (first) {
                first = false;
            } else {
                sb.append(' ');
            }
            final Object object = bundle.get(key);
            sb.append(key);
            sb.append('=');
            if (object instanceof EditorInfo) {
                final EditorInfo info = (EditorInfo) object;
                sb.append("EditorInfo{packageName=").append(info.packageName);
                sb.append(" fieldId=").append(info.fieldId);
                sb.append(" hintText=").append(info.hintText);
                sb.append(" privateImeOptions=").append(info.privateImeOptions);
                sb.append("}");
            } else {
                sb.append(object);
            }
        }
        sb.append('}');
    }

    static class ImeEventArray {
        @NonNull
        public final ImeEvent[] mArray;
        public final int mLength;
        ImeEventArray(ImeEvent[] array, int length) {
            mArray = array;
            mLength = length;
        }
    }
}
