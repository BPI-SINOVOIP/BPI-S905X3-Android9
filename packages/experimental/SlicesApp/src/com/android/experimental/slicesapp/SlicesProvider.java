/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.experimental.slicesapp;

import android.app.PendingIntent;
import android.app.RemoteInput;
import android.app.slice.Slice;
import android.app.slice.Slice.Builder;
import android.app.slice.SliceProvider;
import android.content.ContentResolver;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.text.format.DateUtils;
import android.util.Log;

import java.util.function.Consumer;


public class SlicesProvider extends SliceProvider {

    private static final String TAG = "SliceProvider";
    public static final String SLICE_INTENT = "android.intent.action.EXAMPLE_SLICE_INTENT";
    public static final String SLICE_ACTION = "android.intent.action.EXAMPLE_SLICE_ACTION";
    public static final String INTENT_ACTION_EXTRA = "android.intent.slicesapp.INTENT_ACTION_EXTRA";

    private final int NUM_LIST_ITEMS = 10;

    private SharedPreferences mSharedPrefs;

    @Override
    public boolean onCreate() {
        mSharedPrefs = getContext().getSharedPreferences("slice", 0);
        return true;
    }

    private Uri getIntentUri() {
        return new Uri.Builder()
                .scheme(ContentResolver.SCHEME_CONTENT)
                .authority(getContext().getPackageName())
                .appendPath("main").appendPath("intent")
                .build();
    }

    //@Override
    public Uri onMapIntentToUri(Intent intent) {
        if (intent.getAction().equals(SLICE_INTENT)) {
            return getIntentUri();
        }
        return null;//super.onMapIntentToUri(intent);
    }

    /**
     * Overriding onBindSlice will generate one Slice for all modes.
     * @param sliceUri
     */
    @Override
    public Slice onBindSlice(Uri sliceUri) {
        Log.w(TAG, "onBindSlice uri: " + sliceUri);
        String type = mSharedPrefs.getString("slice_type", "Default");
        if ("Default".equals(type)) {
            return null;
        }
        Slice.Builder b = new Builder(sliceUri);
        if (mSharedPrefs.getBoolean("show_header", false)) {
            b.addText("Header", null, Slice.HINT_TITLE);
            if (mSharedPrefs.getBoolean("show_sub_header", false)) {
                b.addText("Sub-header", null);
            }
        }
        if (sliceUri.equals(getIntentUri())) {
            type = "Intent";
        }
        switch (type) {
            case "Shortcut":
                b.addColor(Color.CYAN, null).addIcon(
                        Icon.createWithResource(getContext(), R.drawable.mady),
                        null,
                        Slice.HINT_LARGE);
                break;
            case "Single-line":
                b.addSubSlice(makeList(new Slice.Builder(b), this::makeSingleLine,
                        this::addIcon));
                addPrimaryAction(b);
                break;
            case "Single-line action":
                b.addSubSlice(makeList(new Slice.Builder(b), this::makeSingleLine,
                        this::addAltActions));
                addPrimaryAction(b);
                break;
            case "Two-line":
                b.addSubSlice(makeList(new Slice.Builder(b), this::makeTwoLine,
                        this::addIcon));
                addPrimaryAction(b);
                break;
            case "Two-line action":
                b.addSubSlice(makeList(new Slice.Builder(b), this::makeTwoLine,
                        this::addAltActions));
                addPrimaryAction(b);
                break;
            case "Weather":
                b.addSubSlice(createWeather(new Slice.Builder(b).addHints(Slice.HINT_HORIZONTAL)));
                break;
            case "Messaging":
                createConversation(b);
                break;
            case "Keep actions":
                b.addSubSlice(createKeepNote(new Slice.Builder(b)));
                break;
            case "Maps multi":
                b.addSubSlice(createMapsMulti(new Slice.Builder(b)
                        .addHints(Slice.HINT_HORIZONTAL)));
                break;
            case "Intent":
                b.addSubSlice(createIntentSlice(new Slice.Builder(b)
                        .addHints(Slice.HINT_HORIZONTAL)));
                break;
            case "Settings":
                createSettingsSlice(b);
                break;
            case "Settings content":
                createSettingsContentSlice(b);
                break;
        }
        if (mSharedPrefs.getBoolean("show_action_row", false)) {
            Intent intent = new Intent(getContext(), SlicesActivity.class);
            PendingIntent pendingIntent = PendingIntent.getActivity(getContext(), 0, intent, 0);
            b.addSubSlice(new Slice.Builder(b).addHints(Slice.HINT_ACTIONS)
                    .addAction(pendingIntent, new Slice.Builder(b)
                            .addText("Action1", null)
                            .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_add),
                                    null)
                            .build())
                    .addAction(pendingIntent, new Slice.Builder(b)
                            .addText("Action2", null)
                            .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_remove),
                                    null)
                            .build())
                    .addAction(pendingIntent, new Slice.Builder(b)
                            .addText("Action3", null)
                            .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_add),
                                    null)
                            .build())
                    .build());
        }
        return b.build();
    }

    private Slice createWeather(Builder grid) {
        grid.addSubSlice(new Slice.Builder(grid)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.weather_1),
                        Slice.HINT_LARGE)
                .addText("MON", null)
                .addText("69\u00B0", null, Slice.HINT_LARGE).build());
        grid.addSubSlice(new Slice.Builder(grid)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.weather_2),
                        Slice.HINT_LARGE)
                .addText("TUE", null)
                .addText("71\u00B0", null, Slice.HINT_LARGE).build());
        grid.addSubSlice(new Slice.Builder(grid)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.weather_3),
                        Slice.HINT_LARGE)
                .addText("WED", null)
                .addText("76\u00B0", null, Slice.HINT_LARGE).build());
        grid.addSubSlice(new Slice.Builder(grid)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.weather_4),
                        Slice.HINT_LARGE)
                .addText("THU", null)
                .addText("69\u00B0", null, Slice.HINT_LARGE).build());
        grid.addSubSlice(new Slice.Builder(grid)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.weather_2),
                        Slice.HINT_LARGE)
                .addText("FRI", null)
                .addText("71\u00B0", null, Slice.HINT_LARGE).build());
        return grid.build();
    }

    private Slice createConversation(Builder b2) {
        b2.addHints(Slice.HINT_LIST);
        b2.addSubSlice(new Slice.Builder(b2)
                .addText("yo home \uD83C\uDF55, I emailed you the info", null)
                .addTimestamp(System.currentTimeMillis() - 20 * DateUtils.MINUTE_IN_MILLIS, null)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.mady),
                        Slice.SUBTYPE_SOURCE,
                        Slice.HINT_TITLE, Slice.HINT_LARGE)
                .build(), Slice.SUBTYPE_MESSAGE);
        b2.addSubSlice(new Builder(b2)
                .addText("just bought my tickets", null)
                .addTimestamp(System.currentTimeMillis() - 10 * DateUtils.MINUTE_IN_MILLIS, null)
                .build(), Slice.SUBTYPE_MESSAGE);
        b2.addSubSlice(new Builder(b2)
                .addText("yay! can't wait for getContext() weekend!\n"
                        + "\uD83D\uDE00", null)
                .addTimestamp(System.currentTimeMillis() - 5 * DateUtils.MINUTE_IN_MILLIS, null)
                .addIcon(Icon.createWithResource(getContext(), R.drawable.mady),
                        Slice.SUBTYPE_SOURCE,
                        Slice.HINT_LARGE)
                .build(), Slice.SUBTYPE_MESSAGE);
        RemoteInput ri = new RemoteInput.Builder("someKey").setLabel("someLabel")
                .setAllowFreeFormInput(true).build();
        b2.addRemoteInput(ri, null);
        return b2.build();
    }

    private Slice addIcon(Builder b) {
        b.addIcon(Icon.createWithResource(getContext(), R.drawable.ic_add), null);
        return b.build();
    }

    private void addAltActions(Builder builder) {
        Intent intent = new Intent(getContext(), SlicesActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(getContext(), 0, intent, 0);
        builder.addSubSlice(new Slice.Builder(builder).addHints(Slice.HINT_ACTIONS)
                .addAction(pendingIntent, new Slice.Builder(builder)
                        .addText("Alt1", null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_add), null)
                        .build())
                .addAction(pendingIntent, new Slice.Builder(builder)
                        .addText("Alt2", null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_remove), null)
                        .build())
                .build());
    }

    private void makeSingleLine(Builder b) {
        b.addText("Single-line list item text", null, Slice.HINT_TITLE);
    }

    private void makeTwoLine(Builder b) {
        b.addText("Two-line list item text", null, Slice.HINT_TITLE);
        b.addText("Secondary text", null);
    }

    private void addPrimaryAction(Builder b) {
        Intent intent = new Intent(getContext(), SlicesActivity.class);
        PendingIntent pi = PendingIntent.getActivity(getContext(), 0, intent, 0);
        b.addSubSlice(new Slice.Builder(b).addAction(pi,
                new Slice.Builder(b).addColor(0xFFFF5722, null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_slice),
                                Slice.HINT_TITLE)
                        .addText("Slice App", null, Slice.HINT_TITLE)
                        .build()).addHints(Slice.HINT_HIDDEN, Slice.HINT_TITLE).build());
    }

    private Slice makeList(Builder list, Consumer<Builder> lineCreator,
            Consumer<Builder> lineHandler) {
        list.addHints(Slice.HINT_LIST);
        for (int i = 0; i < NUM_LIST_ITEMS; i++) {
            Builder b = new Builder(list);
            lineCreator.accept(b);
            lineHandler.accept(b);
            list.addSubSlice(b.build());
        }
        return list.build();
    }

    private Slice createKeepNote(Builder b) {
        Intent intent = new Intent(getContext(), SlicesActivity.class);
        PendingIntent pi = PendingIntent.getActivity(getContext(), 0, intent, 0);
        RemoteInput ri = new RemoteInput.Builder("someKey").setLabel("someLabel")
                .setAllowFreeFormInput(true).build();
        return b.addText("Create new note", null, Slice.HINT_TITLE).addText("with keep", null)
                .addColor(0xffffc107, null)
                .addAction(pi, new Slice.Builder(b)
                        .addText("List", null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_list), null)
                        .build())
                .addAction(pi, new Slice.Builder(b)
                        .addText("Voice note", null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_voice), null)
                        .build())
                .addAction(pi, new Slice.Builder(b)
                        .addText("Camera", null)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_camera), null)
                        .build())
                .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_create), null)
                .addRemoteInput(ri, null)
                .build();
    }

    private Slice createMapsMulti(Builder b) {
        Intent intent = new Intent(getContext(), SlicesActivity.class);
        PendingIntent pi = PendingIntent.getActivity(getContext(), 0, intent, 0);
        b.addHints(Slice.HINT_HORIZONTAL, Slice.HINT_LIST);

        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_home), null)
                        .build())
                .addText("Home", null, Slice.HINT_LARGE)
                .addText("25 min", null).build());
        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_work), null)
                        .build())
                .addText("Work", null, Slice.HINT_LARGE)
                .addText("1 hour 23 min", null).build());
        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_car), null)
                        .build())
                .addText("Mom's", null, Slice.HINT_LARGE)
                .addText("37 min", null).build());
        b.addColor(0xff0B8043, null);
        return b.build();
    }

    private Slice createIntentSlice(Builder b) {
        Intent intent = new Intent(getContext(), SlicesActivity.class);

        PendingIntent pi = PendingIntent.getActivity(getContext(), 0, intent, 0);

        b.addHints(Slice.HINT_LIST).addColor(0xff0B8043, null);
        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_next), null)
                        .build())
                .addText("Next", null, Slice.HINT_LARGE).build());
        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_play), null)
                        .build())
                .addText("Play", null, Slice.HINT_LARGE).build());
        b.addSubSlice(new Slice.Builder(b)
                .addAction(pi, new Slice.Builder(b)
                        .addIcon(Icon.createWithResource(getContext(), R.drawable.ic_prev), null)
                        .build())
                .addText("Previous", null, Slice.HINT_LARGE).build());
        return b.build();
    }

    private Slice.Builder createSettingsSlice(Builder b) {
        b.addSubSlice(new Slice.Builder(b)
                .addAction(getIntent("toggled"), new Slice.Builder(b)
                        .addText("Wi-fi", null)
                        .addText("GoogleGuest", null)
                        .addHints(Slice.HINT_TOGGLE, Slice.HINT_SELECTED)
                        .build())
                .build());
        return b;
    }

    private Slice.Builder createSettingsContentSlice(Builder b) {
        b.addSubSlice(new Slice.Builder(b)
                .addAction(getIntent("main content"),
                        new Slice.Builder(b)
                                .addText("Wi-fi", null)
                                .addText("GoogleGuest", null)
                                .build())
                .addAction(getIntent("toggled"),
                        new Slice.Builder(b)
                                .addHints(Slice.HINT_TOGGLE, Slice.HINT_SELECTED)
                                .build())
                .build());
        return b;
    }

    private PendingIntent getIntent(String message) {
        Intent intent = new Intent(SLICE_ACTION);
        intent.setClass(getContext(), SlicesBroadcastReceiver.class);
        intent.putExtra(INTENT_ACTION_EXTRA, message);
        PendingIntent pi = PendingIntent.getBroadcast(getContext(), 0, intent,
                PendingIntent.FLAG_CANCEL_CURRENT);
        return pi;
    }
}
