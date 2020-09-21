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

import android.os.SystemClock;
import androidx.annotation.NonNull;
import android.text.TextUtils;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputBinding;

import java.util.Optional;
import java.util.concurrent.TimeoutException;
import java.util.function.Predicate;

/**
 * A set of utility methods to avoid boilerplate code when writing end-to-end tests.
 */
public final class ImeEventStreamTestUtils {
    private static final long TIME_SLICE = 50;  // msec

    /**
     * Cannot be instantiated
     */
    private ImeEventStreamTestUtils() {}

    /**
     * Behavior mode of {@link #expectEvent(ImeEventStream, Predicate, EventFilterMode, long)}
     */
    public enum EventFilterMode {
        /**
         * All {@link ImeEvent} events should be checked
         */
        CHECK_ALL,
        /**
         * Only events that return {@code true} from {@link ImeEvent#isEnterEvent()} should be
         * checked
         */
        CHECK_ENTER_EVENT_ONLY,
        /**
         * Only events that return {@code false} from {@link ImeEvent#isEnterEvent()} should be
         * checked
         */
        CHECK_EXIT_EVENT_ONLY,
    }

    /**
     * Wait until an event that matches the given {@code condition} is found in the stream.
     *
     * <p>When this method succeeds to find an event that matches the given {@code condition}, the
     * stream position will be set to the next to the found object then the event found is returned.
     * </p>
     *
     * <p>For convenience, this method automatically filter out exit events (events that return
     * {@code false} from {@link ImeEvent#isEnterEvent()}.</p>
     *
     * <p>TODO: Consider renaming this to {@code expectEventEnter} or something like that.</p>
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param condition the event condition to be matched
     * @param timeout timeout in millisecond
     * @return {@link ImeEvent} found
     * @throws TimeoutException when the no event is matched to the given condition within
     *                          {@code timeout}
     */
    @NonNull
    public static ImeEvent expectEvent(@NonNull ImeEventStream stream,
            @NonNull Predicate<ImeEvent> condition, long timeout) throws TimeoutException {
        return expectEvent(stream, condition, EventFilterMode.CHECK_ENTER_EVENT_ONLY, timeout);
    }

    /**
     * Wait until an event that matches the given {@code condition} is found in the stream.
     *
     * <p>When this method succeeds to find an event that matches the given {@code condition}, the
     * stream position will be set to the next to the found object then the event found is returned.
     * </p>
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param condition the event condition to be matched
     * @param filterMode controls how events are filtered out
     * @param timeout timeout in millisecond
     * @return {@link ImeEvent} found
     * @throws TimeoutException when the no event is matched to the given condition within
     *                          {@code timeout}
     */
    @NonNull
    public static ImeEvent expectEvent(@NonNull ImeEventStream stream,
            @NonNull Predicate<ImeEvent> condition, EventFilterMode filterMode, long timeout)
            throws TimeoutException {
        try {
            Optional<ImeEvent> result;
            while (true) {
                if (timeout < 0) {
                    throw new TimeoutException(
                            "event not found within the timeout: " + stream.dump());
                }
                final Predicate<ImeEvent> combinedCondition;
                switch (filterMode) {
                    case CHECK_ALL:
                        combinedCondition = condition;
                        break;
                    case CHECK_ENTER_EVENT_ONLY:
                        combinedCondition = event -> event.isEnterEvent() && condition.test(event);
                        break;
                    case CHECK_EXIT_EVENT_ONLY:
                        combinedCondition = event -> !event.isEnterEvent() && condition.test(event);
                        break;
                    default:
                        throw new IllegalArgumentException("Unknown filterMode " + filterMode);
                }
                result = stream.seekToFirst(combinedCondition);
                if (result.isPresent()) {
                    break;
                }
                Thread.sleep(TIME_SLICE);
                timeout -= TIME_SLICE;
            }
            final ImeEvent event = result.get();
            if (event == null) {
                throw new NullPointerException("found event is null: " + stream.dump());
            }
            stream.skip(1);
            return event;
        } catch (InterruptedException e) {
            throw new RuntimeException("expectEvent failed: " + stream.dump(), e);
        }
    }

    /**
     * Checks if {@param eventName} has occurred on the EditText(or TextView) of the current
     * activity.
     * @param eventName event name to check
     * @param marker Test marker set to {@link android.widget.EditText#setPrivateImeOptions(String)}
     * @return true if event occurred.
     */
    public static Predicate<ImeEvent> editorMatcher(
        @NonNull String eventName, @NonNull String marker) {
        return event -> {
            if (!TextUtils.equals(eventName, event.getEventName())) {
                return false;
            }
            final EditorInfo editorInfo = event.getArguments().getParcelable("editorInfo");
            return TextUtils.equals(marker, editorInfo.privateImeOptions);
        };
    }

    /**
     * Wait until an event that matches the given command is consumed by the {@link MockIme}.
     *
     * <p>For convenience, this method automatically filter out enter events (events that return
     * {@code true} from {@link ImeEvent#isEnterEvent()}.</p>
     *
     * <p>TODO: Consider renaming this to {@code expectCommandConsumed} or something like that.</p>
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param command {@link ImeCommand} to be waited for.
     * @param timeout timeout in millisecond
     * @return {@link ImeEvent} found
     * @throws TimeoutException when the no event is matched to the given condition within
     *                          {@code timeout}
     */
    @NonNull
    public static ImeEvent expectCommand(@NonNull ImeEventStream stream,
            @NonNull ImeCommand command, long timeout) throws TimeoutException {
        final Predicate<ImeEvent> predicate = event -> {
            if (!TextUtils.equals("onHandleCommand", event.getEventName())) {
                return false;
            }
            final ImeCommand eventCommand =
                    ImeCommand.fromBundle(event.getArguments().getBundle("command"));
            return eventCommand.getId() == command.getId();
        };
        return expectEvent(stream, predicate, EventFilterMode.CHECK_EXIT_EVENT_ONLY, timeout);
    }

    /**
     * Assert that an event that matches the given {@code condition} will no be found in the stream
     * within the given {@code timeout}.
     *
     * <p>When this method succeeds, the stream position will not change.</p>
     *
     * <p>For convenience, this method automatically filter out exit events (events that return
     * {@code false} from {@link ImeEvent#isEnterEvent()}.</p>
     *
     * <p>TODO: Consider renaming this to {@code notExpectEventEnter} or something like that.</p>
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param condition the event condition to be matched
     * @param timeout timeout in millisecond
     * @throws AssertionError if such an event is found within the given {@code timeout}
     */
    public static void notExpectEvent(@NonNull ImeEventStream stream,
            @NonNull Predicate<ImeEvent> condition, long timeout) {
        notExpectEvent(stream, condition, EventFilterMode.CHECK_ENTER_EVENT_ONLY, timeout);
    }

    /**
     * Assert that an event that matches the given {@code condition} will no be found in the stream
     * within the given {@code timeout}.
     *
     * <p>When this method succeeds, the stream position will not change.</p>
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param condition the event condition to be matched
     * @param filterMode controls how events are filtered out
     * @param timeout timeout in millisecond
     * @throws AssertionError if such an event is found within the given {@code timeout}
     */
    public static void notExpectEvent(@NonNull ImeEventStream stream,
            @NonNull Predicate<ImeEvent> condition, EventFilterMode filterMode, long timeout) {
        final Predicate<ImeEvent> combinedCondition;
        switch (filterMode) {
            case CHECK_ALL:
                combinedCondition = condition;
                break;
            case CHECK_ENTER_EVENT_ONLY:
                combinedCondition = event -> event.isEnterEvent() && condition.test(event);
                break;
            case CHECK_EXIT_EVENT_ONLY:
                combinedCondition = event -> !event.isEnterEvent() && condition.test(event);
                break;
            default:
                throw new IllegalArgumentException("Unknown filterMode " + filterMode);
        }
        try {
            while (true) {
                if (timeout < 0) {
                    return;
                }
                if (stream.findFirst(combinedCondition).isPresent()) {
                    throw new AssertionError("notExpectEvent failed: " + stream.dump());
                }
                Thread.sleep(TIME_SLICE);
                timeout -= TIME_SLICE;
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("notExpectEvent failed: " + stream.dump(), e);
        }
    }

    /**
     * A specialized version of {@link #expectEvent(ImeEventStream, Predicate, long)} to wait for
     * {@link android.view.inputmethod.InputMethod#bindInput(InputBinding)}.
     *
     * @param stream {@link ImeEventStream} to be checked.
     * @param targetProcessPid PID to be matched to {@link InputBinding#getPid()}
     * @param timeout timeout in millisecond
     * @throws TimeoutException when "bindInput" is not called within {@code timeout} msec
     */
    public static void expectBindInput(@NonNull ImeEventStream stream, int targetProcessPid,
            long timeout) throws TimeoutException {
        expectEvent(stream, event -> {
            if (!TextUtils.equals("bindInput", event.getEventName())) {
                return false;
            }
            final InputBinding binding = event.getArguments().getParcelable("binding");
            return binding.getPid() == targetProcessPid;
        }, EventFilterMode.CHECK_EXIT_EVENT_ONLY,  timeout);
    }

    /**
     * Waits until {@code MockIme} does not send {@code "onInputViewLayoutChanged"} event
     * for a certain period of time ({@code stableThresholdTime} msec).
     *
     * <p>When this returns non-null {@link ImeLayoutInfo}, the stream position will be set to
     * the next event of the returned layout event.  Otherwise this method does not change stream
     * position.</p>
     * @param stream {@link ImeEventStream} to be checked.
     * @param stableThresholdTime threshold time to consider that {@link MockIme}'s layout is
     *                            stable, in millisecond
     * @return last {@link ImeLayoutInfo} if {@link MockIme} sent one or more
     *         {@code "onInputViewLayoutChanged"} event.  Otherwise {@code null}
     */
    public static ImeLayoutInfo waitForInputViewLayoutStable(@NonNull ImeEventStream stream,
            long stableThresholdTime) {
        ImeLayoutInfo lastLayout = null;
        final Predicate<ImeEvent> layoutFilter = event ->
                !event.isEnterEvent() && event.getEventName().equals("onInputViewLayoutChanged");
        try {
            long deadline = SystemClock.elapsedRealtime() + stableThresholdTime;
            while (true) {
                if (deadline < SystemClock.elapsedRealtime()) {
                    return lastLayout;
                }
                final Optional<ImeEvent> event = stream.seekToFirst(layoutFilter);
                if (event.isPresent()) {
                    // Remember the last event and extend the deadline again.
                    lastLayout = ImeLayoutInfo.readFromBundle(event.get().getArguments());
                    deadline = SystemClock.elapsedRealtime() + stableThresholdTime;
                    stream.skip(1);
                }
                Thread.sleep(TIME_SLICE);
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("notExpectEvent failed: " + stream.dump(), e);
        }
    }
}
