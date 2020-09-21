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
 * limitations under the License
 */

package android.inputmethodservice.cts;

import static android.inputmethodservice.cts.common.DeviceEventConstants.ACTION_DEVICE_EVENT;
import static android.inputmethodservice.cts.common.DeviceEventConstants.EXTRA_EVENT_TIME;
import static android.inputmethodservice.cts.common.DeviceEventConstants.EXTRA_EVENT_TYPE;
import static android.inputmethodservice.cts.common.DeviceEventConstants.EXTRA_EVENT_SENDER;
import static android.inputmethodservice.cts.common.DeviceEventConstants.RECEIVER_CLASS;
import static android.inputmethodservice.cts.common.DeviceEventConstants.RECEIVER_PACKAGE;

import android.content.ContentValues;
import android.content.Intent;
import android.database.Cursor;
import android.inputmethodservice.cts.common.DeviceEventConstants;
import android.inputmethodservice.cts.common.DeviceEventConstants.DeviceEventType;
import android.inputmethodservice.cts.common.EventProviderConstants.EventTableConstants;
import android.inputmethodservice.cts.common.test.TestInfo;
import android.inputmethodservice.cts.db.Entity;
import android.inputmethodservice.cts.db.Field;
import android.inputmethodservice.cts.db.Table;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import android.util.Log;

import java.util.function.Predicate;
import java.util.stream.Stream;

/**
 * Device event object.
 * <p>Device event is one of IME event and Test event, and is used to test behaviors of Input Method
 * Framework.</p>
 */
public final class DeviceEvent {

    private static final boolean DEBUG_STREAM = false;

    public static final Table<DeviceEvent> TABLE = new DeviceEventTable(EventTableConstants.NAME);

    /**
     * Create an intent to send a device event.
     * @param sender an event sender.
     * @param type an event type defined at {@link DeviceEventType}.
     * @return an intent that has event {@code sender}, {@code type}, time from
     *         {@link SystemClock#uptimeMillis()}, and target component of event receiver.
     */
    public static Intent newDeviceEventIntent(@NonNull final String sender,
            @NonNull final DeviceEventType type) {
        return new Intent()
                .setAction(ACTION_DEVICE_EVENT)
                .setClassName(RECEIVER_PACKAGE, RECEIVER_CLASS)
                .putExtra(EXTRA_EVENT_SENDER, sender)
                .putExtra(EXTRA_EVENT_TYPE, type.name())
                .putExtra(EXTRA_EVENT_TIME, SystemClock.uptimeMillis());
    }

    /**
     * Create an {@link DeviceEvent} object from an intent.
     * @param intent a device event intent defined at {@link DeviceEventConstants}.
     * @return {@link DeviceEvent} object that has event sender, type, and time form an
     *         {@code intent}.
     */
    public static DeviceEvent newEvent(final Intent intent) {
        final String sender = intent.getStringExtra(EXTRA_EVENT_SENDER);
        if (sender == null) {
            throw new IllegalArgumentException(
                    "Intent must have " + EXTRA_EVENT_SENDER + ": " + intent);
        }

        final String typeString = intent.getStringExtra(EXTRA_EVENT_TYPE);
        if (typeString == null) {
            throw new IllegalArgumentException(
                    "Intent must have " + EXTRA_EVENT_TYPE + ": " + intent);
        }
        final DeviceEventType type = DeviceEventType.valueOf(typeString);

        if (!intent.hasExtra(EXTRA_EVENT_TIME)) {
            throw new IllegalArgumentException(
                    "Intent must have " + EXTRA_EVENT_TIME + ": " + intent);
        }

        return new DeviceEvent(sender, type, intent.getLongExtra(EXTRA_EVENT_TIME, 0L));
    }

    /**
     * Build {@link ContentValues} object from {@link DeviceEvent} object.
     * @param event a {@link DeviceEvent} object to be converted.
     * @return a converted {@link ContentValues} object.
     */
    public static ContentValues buildContentValues(final DeviceEvent event) {
        return TABLE.buildContentValues(event);
    }

    /**
     * Build {@link Stream<DeviceEvent>} object from {@link Cursor} comes from Content Provider.
     * @param cursor a {@link Cursor} object to be converted.
     * @return a converted {@link Stream<DeviceEvent>} object.
     */
    public static Stream<DeviceEvent> buildStream(final Cursor cursor) {
        return TABLE.buildStream(cursor);
    }

    /**
     * Build {@link Predicate<DeviceEvent>} whether a device event comes from {@code sender}
     *
     * @param sender event sender.
     * @return {@link Predicate<DeviceEvent>} object.
     */
    public static Predicate<DeviceEvent> isFrom(final String sender) {
        return e -> e.sender.equals(sender);
    }

    /**
     * Build {@link Predicate<DeviceEvent>} whether a device event has an event {@code type}.
     *
     * @param type a event type defined in {@link DeviceEventType}.
     * @return {@link Predicate<DeviceEvent>} object.
     */
    public static Predicate<DeviceEvent> isType(final DeviceEventType type) {
        return e -> e.type == type;
    }

    /**
     * Build {@link Predicate<DeviceEvent>} whether a device event is newer than or equals to
     * {@code time}.
     *
     * @param time a time to compare against.
     * @return {@link Predicate<DeviceEvent>} object.
     */
    public static Predicate<DeviceEvent> isNewerThan(final long time) {
        return e -> e.time >= time;
    }

    /**
     * Event source, either Input Method class name or {@link TestInfo#getTestName()}.
     */
    @NonNull
    public final String sender;

    /**
     * Event type, either IME event or Test event.
     */
    @NonNull
    public final DeviceEventType type;

    /**
     * Event time, value is from {@link SystemClock#uptimeMillis()}.
     */
    public final long time;

    private DeviceEvent(final String sender, final DeviceEventType type, final long time) {
        this.sender = sender;
        this.type = type;
        this.time = time;
    }

    @Override
    public String toString() {
        return "Event{ time:" + time + " type:" + type + " sender:" + sender + " }";
    }

    /**
     * Abstraction of device event table in database.
     */
    private static final class DeviceEventTable extends Table<DeviceEvent> {

        private static final String LOG_TAG = DeviceEventTable.class.getSimpleName();

        private final Field SENDER;
        private final Field TYPE;
        private final Field TIME;

        private DeviceEventTable(final String name) {
            super(name, new Entity.Builder<DeviceEvent>()
                    .addField(EventTableConstants.SENDER, Cursor.FIELD_TYPE_STRING)
                    .addField(EventTableConstants.TYPE, Cursor.FIELD_TYPE_STRING)
                    .addField(EventTableConstants.TIME, Cursor.FIELD_TYPE_INTEGER)
                    .build());
            SENDER = getField(EventTableConstants.SENDER);
            TYPE = getField(EventTableConstants.TYPE);
            TIME = getField(EventTableConstants.TIME);
        }

        @Override
        public ContentValues buildContentValues(final DeviceEvent event) {
            final ContentValues values = new ContentValues();
            SENDER.putString(values, event.sender);
            TYPE.putString(values, event.type.name());
            TIME.putLong(values, event.time);
            return values;
        }

        @Override
        public Stream<DeviceEvent> buildStream(Cursor cursor) {
            if (DEBUG_STREAM) {
                Log.d(LOG_TAG, "buildStream:");
            }
            final Stream.Builder<DeviceEvent> builder = Stream.builder();
            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                final DeviceEvent event = new DeviceEvent(
                        SENDER.getString(cursor),
                        DeviceEventType.valueOf(TYPE.getString(cursor)),
                        TIME.getLong(cursor));
                builder.accept(event);
                if (DEBUG_STREAM) {
                    Log.d(LOG_TAG, " event=" +event);
                }
            }
            return builder.build();
        }
    }
}
