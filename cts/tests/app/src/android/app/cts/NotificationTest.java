/*
 * Copyright (C) 2008 The Android Open Source Project
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

package android.app.cts;

import android.app.Notification;
import android.app.Notification.Action.Builder;
import android.app.Notification.MessagingStyle;
import android.app.Notification.MessagingStyle.Message;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Person;
import android.app.RemoteInput;
import android.app.stubs.R;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import androidx.annotation.Nullable;

import android.os.StrictMode;
import android.test.AndroidTestCase;
import android.widget.RemoteViews;

import java.util.ArrayList;
import java.util.function.Consumer;

public class NotificationTest extends AndroidTestCase {
    private static final String TEXT_RESULT_KEY = "text";
    private static final String DATA_RESULT_KEY = "data";
    private static final String DATA_AND_TEXT_RESULT_KEY = "data and text";

    private Notification.Action mAction;
    private Notification mNotification;
    private Context mContext;

    private static final String TICKER_TEXT = "tickerText";
    private static final String CONTENT_TITLE = "contentTitle";
    private static final String CONTENT_TEXT = "contentText";
    private static final String URI_STRING = "uriString";
    private static final String ACTION_TITLE = "actionTitle";
    private static final int TOLERANCE = 200;
    private static final long TIMEOUT = 4000;
    private static final NotificationChannel CHANNEL = new NotificationChannel("id", "name",
            NotificationManager.IMPORTANCE_HIGH);
    private static final String SHORTCUT_ID = "shortcutId";
    private static final String SETTING_TEXT = "work chats";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mContext = getContext();
        mNotification = new Notification();
    }

    public void testConstructor() {
        mNotification = null;
        mNotification = new Notification();
        assertNotNull(mNotification);
        assertTrue(System.currentTimeMillis() - mNotification.when < TOLERANCE);

        mNotification = null;
        final int notificationTime = 200;
        mNotification = new Notification(0, TICKER_TEXT, notificationTime);
        assertEquals(notificationTime, mNotification.when);
        assertEquals(0, mNotification.icon);
        assertEquals(TICKER_TEXT, mNotification.tickerText);
        assertEquals(0, mNotification.number);
    }

    public void testBuilderConstructor() {
        mNotification = new Notification.Builder(mContext, CHANNEL.getId()).build();
        assertEquals(CHANNEL.getId(), mNotification.getChannelId());
        assertEquals(Notification.BADGE_ICON_NONE, mNotification.getBadgeIconType());
        assertNull(mNotification.getShortcutId());
        assertEquals(Notification.GROUP_ALERT_ALL, mNotification.getGroupAlertBehavior());
        assertEquals((long) 0, mNotification.getTimeoutAfter());
    }

    public void testDescribeContents() {
        final int expected = 0;
        mNotification = new Notification();
        assertEquals(expected, mNotification.describeContents());
    }

    public void testCategories() {
        assertNotNull(Notification.CATEGORY_ALARM);
        assertNotNull(Notification.CATEGORY_CALL);
        assertNotNull(Notification.CATEGORY_EMAIL);
        assertNotNull(Notification.CATEGORY_ERROR);
        assertNotNull(Notification.CATEGORY_EVENT);
        assertNotNull(Notification.CATEGORY_MESSAGE);
        assertNotNull(Notification.CATEGORY_NAVIGATION);
        assertNotNull(Notification.CATEGORY_PROGRESS);
        assertNotNull(Notification.CATEGORY_PROMO);
        assertNotNull(Notification.CATEGORY_RECOMMENDATION);
        assertNotNull(Notification.CATEGORY_REMINDER);
        assertNotNull(Notification.CATEGORY_SERVICE);
        assertNotNull(Notification.CATEGORY_SOCIAL);
        assertNotNull(Notification.CATEGORY_STATUS);
        assertNotNull(Notification.CATEGORY_SYSTEM);
        assertNotNull(Notification.CATEGORY_TRANSPORT);
    }

    public void testWriteToParcel() {

        mNotification = new Notification.Builder(mContext, CHANNEL.getId())
                .setBadgeIconType(Notification.BADGE_ICON_SMALL)
                .setShortcutId(SHORTCUT_ID)
                .setTimeoutAfter(TIMEOUT)
                .setSettingsText(SETTING_TEXT)
                .setGroupAlertBehavior(Notification.GROUP_ALERT_CHILDREN)
                .build();
        mNotification.icon = 0;
        mNotification.number = 1;
        final Intent intent = new Intent();
        final PendingIntent pendingIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
        mNotification.contentIntent = pendingIntent;
        final Intent deleteIntent = new Intent();
        final PendingIntent delPendingIntent = PendingIntent.getBroadcast(
                mContext, 0, deleteIntent, 0);
        mNotification.deleteIntent = delPendingIntent;
        mNotification.tickerText = TICKER_TEXT;

        final RemoteViews contentView = new RemoteViews(mContext.getPackageName(),
                android.R.layout.simple_list_item_1);
        mNotification.contentView = contentView;
        mNotification.defaults = 0;
        mNotification.flags = 0;
        final Uri uri = Uri.parse(URI_STRING);
        mNotification.sound = uri;
        mNotification.audioStreamType = 0;
        final long[] longArray = { 1l, 2l, 3l };
        mNotification.vibrate = longArray;
        mNotification.ledARGB = 0;
        mNotification.ledOnMS = 0;
        mNotification.ledOffMS = 0;
        mNotification.iconLevel = 0;

        Parcel parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        // Test Notification(Parcel)
        Notification result = new Notification(parcel);
        assertEquals(mNotification.icon, result.icon);
        assertEquals(mNotification.when, result.when);
        assertEquals(mNotification.number, result.number);
        assertNotNull(result.contentIntent);
        assertNotNull(result.deleteIntent);
        assertEquals(mNotification.tickerText, result.tickerText);
        assertNotNull(result.contentView);
        assertEquals(mNotification.defaults, result.defaults);
        assertEquals(mNotification.flags, result.flags);
        assertNotNull(result.sound);
        assertEquals(mNotification.audioStreamType, result.audioStreamType);
        assertEquals(mNotification.vibrate[0], result.vibrate[0]);
        assertEquals(mNotification.vibrate[1], result.vibrate[1]);
        assertEquals(mNotification.vibrate[2], result.vibrate[2]);
        assertEquals(mNotification.ledARGB, result.ledARGB);
        assertEquals(mNotification.ledOnMS, result.ledOnMS);
        assertEquals(mNotification.ledOffMS, result.ledOffMS);
        assertEquals(mNotification.iconLevel, result.iconLevel);
        assertEquals(mNotification.getShortcutId(), result.getShortcutId());
        assertEquals(mNotification.getBadgeIconType(), result.getBadgeIconType());
        assertEquals(mNotification.getTimeoutAfter(), result.getTimeoutAfter());
        assertEquals(mNotification.getChannelId(), result.getChannelId());
        assertEquals(mNotification.getSettingsText(), result.getSettingsText());
        assertEquals(mNotification.getGroupAlertBehavior(), result.getGroupAlertBehavior());

        mNotification.contentIntent = null;
        parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        result = new Notification(parcel);
        assertNull(result.contentIntent);

        mNotification.deleteIntent = null;
        parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        result = new Notification(parcel);
        assertNull(result.deleteIntent);

        mNotification.tickerText = null;
        parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        result = new Notification(parcel);
        assertNull(result.tickerText);

        mNotification.contentView = null;
        parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        result = new Notification(parcel);
        assertNull(result.contentView);

        mNotification.sound = null;
        parcel = Parcel.obtain();
        mNotification.writeToParcel(parcel, 0);
        parcel.setDataPosition(0);
        result = new Notification(parcel);
        assertNull(result.sound);
    }

    public void testColorizeNotification() {
        mNotification = new Notification.Builder(mContext, "channel_id")
                .setSmallIcon(1)
                .setContentTitle(CONTENT_TITLE)
                .setColorized(true)
                .build();

        assertTrue(mNotification.extras.getBoolean(Notification.EXTRA_COLORIZED));
    }

    public void testBuilder() {
        final Intent intent = new Intent();
        final PendingIntent contentIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
        mNotification = new Notification.Builder(mContext, CHANNEL.getId())
                .setSmallIcon(1)
                .setContentTitle(CONTENT_TITLE)
                .setContentText(CONTENT_TEXT)
                .setContentIntent(contentIntent)
                .setBadgeIconType(Notification.BADGE_ICON_SMALL)
                .setShortcutId(SHORTCUT_ID)
                .setTimeoutAfter(TIMEOUT)
                .setSettingsText(SETTING_TEXT)
                .setGroupAlertBehavior(Notification.GROUP_ALERT_SUMMARY)
                .build();
        assertEquals(CONTENT_TEXT, mNotification.extras.getString(Notification.EXTRA_TEXT));
        assertEquals(CONTENT_TITLE, mNotification.extras.getString(Notification.EXTRA_TITLE));
        assertEquals(1, mNotification.icon);
        assertEquals(contentIntent, mNotification.contentIntent);
        assertEquals(CHANNEL.getId(), mNotification.getChannelId());
        assertEquals(Notification.BADGE_ICON_SMALL, mNotification.getBadgeIconType());
        assertEquals(SHORTCUT_ID, mNotification.getShortcutId());
        assertEquals(TIMEOUT, mNotification.getTimeoutAfter());
        assertEquals(SETTING_TEXT, mNotification.getSettingsText());
        assertEquals(Notification.GROUP_ALERT_SUMMARY, mNotification.getGroupAlertBehavior());
    }

    public void testBuilder_getStyle() {
        MessagingStyle ms = new MessagingStyle(new Person.Builder().setName("Test name").build());
        Notification.Builder builder = new Notification.Builder(mContext, CHANNEL.getId());

        builder.setStyle(ms);

        assertEquals(ms, builder.getStyle());
    }

    public void testActionBuilder() {
        final Intent intent = new Intent();
        final PendingIntent actionIntent = PendingIntent.getBroadcast(mContext, 0, intent, 0);
        mAction = null;
        mAction = new Notification.Action.Builder(0, ACTION_TITLE, actionIntent).build();
        assertEquals(ACTION_TITLE, mAction.title);
        assertEquals(actionIntent, mAction.actionIntent);
        assertEquals(true, mAction.getAllowGeneratedReplies());
    }

    public void testNotification_addPerson() {
        String name = "name";
        String key = "key";
        String uri = "name:name";
        Person person = new Person.Builder()
                .setName(name)
                .setIcon(Icon.createWithResource(mContext, 1))
                .setKey(key)
                .setUri(uri)
                .build();
        mNotification = new Notification.Builder(mContext, CHANNEL.getId())
                .setSmallIcon(1)
                .setContentTitle(CONTENT_TITLE)
                .addPerson(person)
                .build();

        ArrayList<Person> restoredPeople = mNotification.extras.getParcelableArrayList(
                Notification.EXTRA_PEOPLE_LIST);
        assertNotNull(restoredPeople);
        Person restoredPerson = restoredPeople.get(0);
        assertNotNull(restoredPerson);
        assertNotNull(restoredPerson.getIcon());
        assertEquals(name, restoredPerson.getName());
        assertEquals(key, restoredPerson.getKey());
        assertEquals(uri, restoredPerson.getUri());
    }

    public void testNotification_MessagingStyle_people() {
        String name = "name";
        String key = "key";
        String uri = "name:name";
        Person user = new Person.Builder()
                .setName(name)
                .setIcon(Icon.createWithResource(mContext, 1))
                .setKey(key)
                .setUri(uri)
                .build();
        Person participant = new Person.Builder().setName("sender").build();
        Notification.MessagingStyle messagingStyle = new Notification.MessagingStyle(user)
                .addMessage("text", 0, participant)
                .addMessage(new Message("text 2", 0, participant));
        mNotification = new Notification.Builder(mContext, CHANNEL.getId())
                .setSmallIcon(1)
                .setStyle(messagingStyle)
                .build();

        Person restoredPerson = mNotification.extras.getParcelable(
                Notification.EXTRA_MESSAGING_PERSON);
        assertNotNull(restoredPerson);
        assertNotNull(restoredPerson.getIcon());
        assertEquals(name, restoredPerson.getName());
        assertEquals(key, restoredPerson.getKey());
        assertEquals(uri, restoredPerson.getUri());
        assertNotNull(mNotification.extras.getParcelableArray(Notification.EXTRA_MESSAGES));
    }


    public void testMessagingStyle_historicMessages() {
        mNotification = new Notification.Builder(mContext, CHANNEL.getId())
                .setSmallIcon(1)
                .setContentTitle(CONTENT_TITLE)
                .setStyle(new Notification.MessagingStyle("self name")
                        .addMessage("text", 0, "sender")
                        .addMessage(new Message("image", 0, "sender")
                                .setData("image/png", Uri.parse("http://example.com/image.png")))
                        .addHistoricMessage(new Message("historic text", 0, "historic sender"))
                        .setConversationTitle("title")
                ).build();

        assertNotNull(
                mNotification.extras.getParcelableArray(Notification.EXTRA_HISTORIC_MESSAGES));
    }

    public void testMessagingStyle_isGroupConversation() {
        mContext.getApplicationInfo().targetSdkVersion = Build.VERSION_CODES.P;
        Notification.MessagingStyle messagingStyle = new Notification.MessagingStyle("self name")
                .setGroupConversation(true)
                .setConversationTitle("test conversation title");
        Notification notification = new Notification.Builder(mContext, "test id")
                .setSmallIcon(1)
                .setContentTitle("test title")
                .setStyle(messagingStyle)
                .build();

        assertTrue(messagingStyle.isGroupConversation());
        assertTrue(notification.extras.getBoolean(Notification.EXTRA_IS_GROUP_CONVERSATION));
    }

    public void testMessagingStyle_isGroupConversation_noConversationTitle() {
        mContext.getApplicationInfo().targetSdkVersion = Build.VERSION_CODES.P;
        Notification.MessagingStyle messagingStyle = new Notification.MessagingStyle("self name")
                .setGroupConversation(true)
                .setConversationTitle(null);
        Notification notification = new Notification.Builder(mContext, "test id")
                .setSmallIcon(1)
                .setContentTitle("test title")
                .setStyle(messagingStyle)
                .build();

        assertTrue(messagingStyle.isGroupConversation());
        assertTrue(notification.extras.getBoolean(Notification.EXTRA_IS_GROUP_CONVERSATION));
    }

    public void testMessagingStyle_isGroupConversation_withConversationTitle_legacy() {
        // In legacy (version < P), isGroupConversation is controlled by conversationTitle.
        mContext.getApplicationInfo().targetSdkVersion = Build.VERSION_CODES.O;
        Notification.MessagingStyle messagingStyle = new Notification.MessagingStyle("self name")
                .setGroupConversation(false)
                .setConversationTitle("test conversation title");
        Notification notification = new Notification.Builder(mContext, "test id")
                .setSmallIcon(1)
                .setContentTitle("test title")
                .setStyle(messagingStyle)
                .build();

        assertTrue(messagingStyle.isGroupConversation());
        assertFalse(notification.extras.getBoolean(Notification.EXTRA_IS_GROUP_CONVERSATION));
    }

    public void testMessagingStyle_isGroupConversation_withoutConversationTitle_legacy() {
        // In legacy (version < P), isGroupConversation is controlled by conversationTitle.
        mContext.getApplicationInfo().targetSdkVersion = Build.VERSION_CODES.O;
        Notification.MessagingStyle messagingStyle = new Notification.MessagingStyle("self name")
                .setGroupConversation(true)
                .setConversationTitle(null);
        Notification notification = new Notification.Builder(mContext, "test id")
                .setSmallIcon(1)
                .setContentTitle("test title")
                .setStyle(messagingStyle)
                .build();

        assertFalse(messagingStyle.isGroupConversation());
        assertTrue(notification.extras.getBoolean(Notification.EXTRA_IS_GROUP_CONVERSATION));
    }

    public void testMessagingStyle_getUser() {
        Person user = new Person.Builder().setName("Test name").build();

        MessagingStyle messagingStyle = new MessagingStyle(user);

        assertEquals(user, messagingStyle.getUser());
    }

    public void testMessage() {
        Person sender = new Person.Builder().setName("Test name").build();
        String text = "Test message";
        long timestamp = 400;

        Message message = new Message(text, timestamp, sender);

        assertEquals(text, message.getText());
        assertEquals(timestamp, message.getTimestamp());
        assertEquals(sender, message.getSenderPerson());
    }

    public void testToString() {
        mNotification = new Notification();
        assertNotNull(mNotification.toString());
        mNotification = null;
    }

    public void testNotificationActionBuilder_setDataOnlyRemoteInput() throws Throwable {
        Notification.Action a = newActionBuilder()
                .addRemoteInput(newDataOnlyRemoteInput()).build();
        RemoteInput[] textInputs = a.getRemoteInputs();
        assertTrue(textInputs == null || textInputs.length == 0);
        verifyRemoteInputArrayHasSingleResult(a.getDataOnlyRemoteInputs(), DATA_RESULT_KEY);
    }

    public void testNotificationActionBuilder_setTextAndDataOnlyRemoteInput() throws Throwable {
        Notification.Action a = newActionBuilder()
                .addRemoteInput(newDataOnlyRemoteInput())
                .addRemoteInput(newTextRemoteInput())
                .build();

        verifyRemoteInputArrayHasSingleResult(a.getRemoteInputs(), TEXT_RESULT_KEY);
        verifyRemoteInputArrayHasSingleResult(a.getDataOnlyRemoteInputs(), DATA_RESULT_KEY);
    }

    public void testNotificationActionBuilder_setTextAndDataOnlyAndBothRemoteInput()
            throws Throwable {
        Notification.Action a = newActionBuilder()
                .addRemoteInput(newDataOnlyRemoteInput())
                .addRemoteInput(newTextRemoteInput())
                .addRemoteInput(newTextAndDataRemoteInput())
                .build();

        assertTrue(a.getRemoteInputs() != null && a.getRemoteInputs().length == 2);
        assertEquals(TEXT_RESULT_KEY, a.getRemoteInputs()[0].getResultKey());
        assertFalse(a.getRemoteInputs()[0].isDataOnly());
        assertEquals(DATA_AND_TEXT_RESULT_KEY, a.getRemoteInputs()[1].getResultKey());
        assertFalse(a.getRemoteInputs()[1].isDataOnly());

        verifyRemoteInputArrayHasSingleResult(a.getDataOnlyRemoteInputs(), DATA_RESULT_KEY);
        assertTrue(a.getDataOnlyRemoteInputs()[0].isDataOnly());
    }

    public void testAction_builder_hasDefault() {
        Notification.Action action = makeNotificationAction(null);
        assertEquals(Notification.Action.SEMANTIC_ACTION_NONE, action.getSemanticAction());
    }

    public void testAction_builder_setSemanticAction() {
        Notification.Action action = makeNotificationAction(
                builder -> builder.setSemanticAction(Notification.Action.SEMANTIC_ACTION_REPLY));
        assertEquals(Notification.Action.SEMANTIC_ACTION_REPLY, action.getSemanticAction());
    }

    public void testAction_parcel() {
        Notification.Action action = writeAndReadParcelable(
                makeNotificationAction(builder -> {
                    builder.setSemanticAction(Notification.Action.SEMANTIC_ACTION_ARCHIVE);
                    builder.setAllowGeneratedReplies(true);
                }));

        assertEquals(Notification.Action.SEMANTIC_ACTION_ARCHIVE, action.getSemanticAction());
        assertTrue(action.getAllowGeneratedReplies());
    }

    public void testAction_clone() {
        Notification.Action action = makeNotificationAction(
                builder -> builder.setSemanticAction(Notification.Action.SEMANTIC_ACTION_DELETE));
        assertEquals(
                Notification.Action.SEMANTIC_ACTION_DELETE,
                action.clone().getSemanticAction());
    }

    public void testBuildStrictMode() {
        try {
            StrictMode.setThreadPolicy(
                    new StrictMode.ThreadPolicy.Builder().detectAll().penaltyDeath().build());
            Notification.Action a = newActionBuilder()
                    .addRemoteInput(newDataOnlyRemoteInput())
                    .addRemoteInput(newTextRemoteInput())
                    .addRemoteInput(newTextAndDataRemoteInput())
                    .build();
            Notification.Builder b = new Notification.Builder(mContext, "id")
                    .setStyle(new Notification.BigTextStyle().setBigContentTitle("Big content"))
                    .setContentTitle("title")
                    .setActions(a);

            b.build();
        } finally {
            StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder().permitAll().build());
        }
    }

    private static RemoteInput newDataOnlyRemoteInput() {
        return new RemoteInput.Builder(DATA_RESULT_KEY)
            .setAllowFreeFormInput(false)
            .setAllowDataType("mimeType", true)
            .build();
    }

    private static RemoteInput newTextAndDataRemoteInput() {
        return new RemoteInput.Builder(DATA_AND_TEXT_RESULT_KEY)
            .setAllowDataType("mimeType", true)
            .build();  // allowFreeForm defaults to true
    }

    private static RemoteInput newTextRemoteInput() {
        return new RemoteInput.Builder(TEXT_RESULT_KEY).build();  // allowFreeForm defaults to true
    }

    private static void verifyRemoteInputArrayHasSingleResult(
            RemoteInput[] remoteInputs, String expectedResultKey) {
        assertTrue(remoteInputs != null && remoteInputs.length == 1);
        assertEquals(expectedResultKey, remoteInputs[0].getResultKey());
    }

    private static Notification.Action.Builder newActionBuilder() {
        return new Notification.Action.Builder(0, "title", null);
    }

    /**
     * Writes an arbitrary {@link Parcelable} into a {@link Parcel} using its writeToParcel
     * method before reading it out again to check that it was sent properly.
     */
    private static <T extends Parcelable> T writeAndReadParcelable(T original) {
        Parcel p = Parcel.obtain();
        p.writeParcelable(original, /* flags */ 0);
        p.setDataPosition(0);
        return p.readParcelable(/* classLoader */ null);
    }

    /**
     * Creates a Notification.Action by mocking initial dependencies and then applying
     * transformations if they're defined.
     */
    private Notification.Action makeNotificationAction(
            @Nullable Consumer<Builder> transformation) {
        Notification.Action.Builder actionBuilder =
            new Notification.Action.Builder(null, "Test Title", null);
        if (transformation != null) {
            transformation.accept(actionBuilder);
        }
        return actionBuilder.build();
    }
}
