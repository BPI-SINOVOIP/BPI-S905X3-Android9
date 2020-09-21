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

package com.android.server.notification;

import static junit.framework.Assert.assertEquals;
import static junit.framework.Assert.assertNotNull;
import static junit.framework.Assert.assertNull;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;

import android.app.ActivityManager;
import android.app.Notification;
import android.app.Person;
import android.app.PendingIntent;
import android.app.RemoteInput;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.Icon;
import android.net.Uri;
import android.os.Build;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.style.StyleSpan;
import android.widget.RemoteViews;

import com.android.server.UiServiceTestCase;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
@SmallTest
public class NotificationTest extends UiServiceTestCase {

    @Mock
    ActivityManager mAm;

    @Mock
    Resources mResources;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    public void testStripsExtendersInLowRamModeNoWhitelistNoTv() {
        Notification.Builder nb = new Notification.Builder(mContext, "channel");
        nb.extend(new Notification.CarExtender().setColor(Color.RED));
        nb.extend(new Notification.TvExtender().setChannelId("different channel"));
        nb.extend(new Notification.WearableExtender().setDismissalId("dismiss"));
        Notification before = nb.build();

        // No whitelist
        Context context = spy(getContext());
        when(context.getResources()).thenReturn(mResources);
        when(mResources.getStringArray(anyInt())).thenReturn(new String[0]);

        Notification after = Notification.Builder.maybeCloneStrippedForDelivery(before, true,
                context);

        assertEquals("different channel", new Notification.TvExtender(before).getChannelId());
        assertNull(new Notification.TvExtender(after).getChannelId());

        assertEquals(Color.RED, new Notification.CarExtender(before).getColor());
        assertEquals(Notification.COLOR_DEFAULT, new Notification.CarExtender(after).getColor());

        assertEquals("dismiss", new Notification.WearableExtender(before).getDismissalId());
        assertNull(new Notification.WearableExtender(after).getDismissalId());
    }

    @Test
    public void testStripsExtendersInLowRamModeHasWhitelist() {
        Notification.Builder nb = new Notification.Builder(mContext, "channel");
        nb.extend(new Notification.CarExtender().setColor(Color.RED));
        nb.extend(new Notification.TvExtender().setChannelId("different channel"));
        nb.extend(new Notification.WearableExtender().setDismissalId("dismiss"));
        Notification before = nb.build();

        // Has whitelist
        Context context = spy(mContext);
        when(context.getResources()).thenReturn(mResources);
        when(mResources.getStringArray(anyInt())).thenReturn(new String[1]);

        Notification after = Notification.Builder.maybeCloneStrippedForDelivery(before, true,
                context);

        assertEquals("different channel", new Notification.TvExtender(before).getChannelId());
        assertEquals("different channel", new Notification.TvExtender(after).getChannelId());

        assertEquals(Color.RED, new Notification.CarExtender(before).getColor());
        assertEquals(Color.RED, new Notification.CarExtender(after).getColor());

        assertEquals("dismiss", new Notification.WearableExtender(before).getDismissalId());
        assertEquals("dismiss", new Notification.WearableExtender(after).getDismissalId());
    }

    @Test
    public void testStripsRemoteViewsInLowRamMode() {
        Context context = spy(mContext);
        ApplicationInfo ai = new ApplicationInfo();
        ai.targetSdkVersion = Build.VERSION_CODES.M;
        when(context.getApplicationInfo()).thenReturn(ai);

        final Notification.BigTextStyle style = new Notification.BigTextStyle()
                .bigText("hello")
                .setSummaryText("And the summary");
        Notification before = new Notification.Builder(context, "channel")
                .setContentText("hi")
                .setStyle(style)
                .build();

        Notification after = Notification.Builder.maybeCloneStrippedForDelivery(before, true,
                mContext);
        assertNotNull(before.contentView);
        assertNotNull(before.bigContentView);
        assertNotNull(before.headsUpContentView);
        assertNull(after.contentView);
        assertNull(after.bigContentView);
        assertNull(after.headsUpContentView);
    }

    @Test
    public void testDoesNotStripsExtendersInNormalRamMode() {
        Notification.Builder nb = new Notification.Builder(mContext, "channel");
        nb.extend(new Notification.CarExtender().setColor(Color.RED));
        nb.extend(new Notification.TvExtender().setChannelId("different channel"));
        nb.extend(new Notification.WearableExtender().setDismissalId("dismiss"));
        Notification before = nb.build();
        Notification after = Notification.Builder.maybeCloneStrippedForDelivery(before, false,
                mContext);

        assertTrue(before == after);

        assertEquals("different channel", new Notification.TvExtender(before).getChannelId());
        assertEquals(Color.RED, new Notification.CarExtender(before).getColor());
        assertEquals("dismiss", new Notification.WearableExtender(before).getDismissalId());
    }

    @Test
    public void testStyleChangeVisiblyDifferent_noStyles() {
        Notification.Builder n1 = new Notification.Builder(mContext, "test");
        Notification.Builder n2 = new Notification.Builder(mContext, "test");

        assertFalse(Notification.areStyledNotificationsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testStyleChangeVisiblyDifferent_noStyleToStyle() {
        Notification.Builder n1 = new Notification.Builder(mContext, "test");
        Notification.Builder n2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigTextStyle());

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testStyleChangeVisiblyDifferent_styleToNoStyle() {
        Notification.Builder n2 = new Notification.Builder(mContext, "test");
        Notification.Builder n1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigTextStyle());

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testStyleChangeVisiblyDifferent_changeStyle() {
        Notification.Builder n1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.InboxStyle());
        Notification.Builder n2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigTextStyle());

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testInboxTextChange() {
        Notification.Builder nInbox1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.InboxStyle().addLine("a").addLine("b"));
        Notification.Builder nInbox2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.InboxStyle().addLine("b").addLine("c"));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nInbox1, nInbox2));
    }

    @Test
    public void testBigTextTextChange() {
        Notification.Builder nBigText1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigTextStyle().bigText("something"));
        Notification.Builder nBigText2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigTextStyle().bigText("else"));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nBigText1, nBigText2));
    }

    @Test
    public void testBigPictureChange() {
        Bitmap bitA = mock(Bitmap.class);
        when(bitA.getGenerationId()).thenReturn(100);
        Bitmap bitB = mock(Bitmap.class);
        when(bitB.getGenerationId()).thenReturn(200);

        Notification.Builder nBigPic1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigPictureStyle().bigPicture(bitA));
        Notification.Builder nBigPic2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.BigPictureStyle().bigPicture(bitB));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nBigPic1, nBigPic2));
    }

    @Test
    public void testMessagingChange_text() {
        Notification.Builder nM1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 100, mock(Person.class))));
        Notification.Builder nM2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 100, mock(Person.class)))
                        .addMessage(new Notification.MessagingStyle.Message(
                                "b", 100, mock(Person.class)))
                );

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nM1, nM2));
    }

    @Test
    public void testMessagingChange_data() {
        Notification.Builder nM1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 100, mock(Person.class))
                                .setData("text", mock(Uri.class))));
        Notification.Builder nM2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 100, mock(Person.class))));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nM1, nM2));
    }

    @Test
    public void testMessagingChange_sender() {
        Person a = mock(Person.class);
        when(a.getName()).thenReturn("A");
        Person b = mock(Person.class);
        when(b.getName()).thenReturn("b");
        Notification.Builder nM1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message("a", 100, b)));
        Notification.Builder nM2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message("a", 100, a)));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nM1, nM2));
    }

    @Test
    public void testMessagingChange_key() {
        Person a = mock(Person.class);
        when(a.getKey()).thenReturn("A");
        Person b = mock(Person.class);
        when(b.getKey()).thenReturn("b");
        Notification.Builder nM1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message("a", 100, a)));
        Notification.Builder nM2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message("a", 100, b)));

        assertTrue(Notification.areStyledNotificationsVisiblyDifferent(nM1, nM2));
    }

    @Test
    public void testMessagingChange_ignoreTimeChange() {
        Notification.Builder nM1 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 100, mock(Person.class))));
        Notification.Builder nM2 = new Notification.Builder(mContext, "test")
                .setStyle(new Notification.MessagingStyle("")
                        .addMessage(new Notification.MessagingStyle.Message(
                                "a", 1000, mock(Person.class)))
                );

        assertFalse(Notification.areStyledNotificationsVisiblyDifferent(nM1, nM2));
    }

    @Test
    public void testRemoteViews_nullChange() {
        Notification.Builder n1 = new Notification.Builder(mContext, "test")
                .setContent(mock(RemoteViews.class));
        Notification.Builder n2 = new Notification.Builder(mContext, "test");
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test");
        n2 = new Notification.Builder(mContext, "test")
                .setContent(mock(RemoteViews.class));
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test")
                .setCustomBigContentView(mock(RemoteViews.class));
        n2 = new Notification.Builder(mContext, "test");
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test");
        n2 = new Notification.Builder(mContext, "test")
                .setCustomBigContentView(mock(RemoteViews.class));
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test");
        n2 = new Notification.Builder(mContext, "test");
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));
    }

    @Test
    public void testRemoteViews_layoutChange() {
        RemoteViews a = mock(RemoteViews.class);
        when(a.getLayoutId()).thenReturn(234);
        RemoteViews b = mock(RemoteViews.class);
        when(b.getLayoutId()).thenReturn(189);

        Notification.Builder n1 = new Notification.Builder(mContext, "test").setContent(a);
        Notification.Builder n2 = new Notification.Builder(mContext, "test").setContent(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomBigContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomBigContentView(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));
    }

    @Test
    public void testRemoteViews_layoutSame() {
        RemoteViews a = mock(RemoteViews.class);
        when(a.getLayoutId()).thenReturn(234);
        RemoteViews b = mock(RemoteViews.class);
        when(b.getLayoutId()).thenReturn(234);

        Notification.Builder n1 = new Notification.Builder(mContext, "test").setContent(a);
        Notification.Builder n2 = new Notification.Builder(mContext, "test").setContent(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomBigContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomBigContentView(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));
    }

    @Test
    public void testRemoteViews_sequenceChange() {
        RemoteViews a = mock(RemoteViews.class);
        when(a.getLayoutId()).thenReturn(234);
        when(a.getSequenceNumber()).thenReturn(1);
        RemoteViews b = mock(RemoteViews.class);
        when(b.getLayoutId()).thenReturn(234);
        when(b.getSequenceNumber()).thenReturn(2);

        Notification.Builder n1 = new Notification.Builder(mContext, "test").setContent(a);
        Notification.Builder n2 = new Notification.Builder(mContext, "test").setContent(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomBigContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomBigContentView(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(b);
        assertTrue(Notification.areRemoteViewsChanged(n1, n2));
    }

    @Test
    public void testRemoteViews_sequenceSame() {
        RemoteViews a = mock(RemoteViews.class);
        when(a.getLayoutId()).thenReturn(234);
        when(a.getSequenceNumber()).thenReturn(1);
        RemoteViews b = mock(RemoteViews.class);
        when(b.getLayoutId()).thenReturn(234);
        when(b.getSequenceNumber()).thenReturn(1);

        Notification.Builder n1 = new Notification.Builder(mContext, "test").setContent(a);
        Notification.Builder n2 = new Notification.Builder(mContext, "test").setContent(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomBigContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomBigContentView(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));

        n1 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(a);
        n2 = new Notification.Builder(mContext, "test").setCustomHeadsUpContentView(b);
        assertFalse(Notification.areRemoteViewsChanged(n1, n2));
    }

    @Test
    public void testActionsDifferent_null() {
        Notification n1 = new Notification.Builder(mContext, "test")
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .build();

        assertFalse(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsDifferentSame() {
        PendingIntent intent = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent).build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent).build())
                .build();

        assertFalse(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsDifferentText() {
        PendingIntent intent = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent).build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 2", intent).build())
                .build();

        assertTrue(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsDifferentSpannables() {
        PendingIntent intent = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon,
                        new SpannableStringBuilder().append("test1",
                                new StyleSpan(Typeface.BOLD),
                                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE),
                        intent).build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "test1", intent).build())
                .build();

        assertFalse(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsDifferentNumber() {
        PendingIntent intent = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent).build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent).build())
                .addAction(new Notification.Action.Builder(icon, "TEXT 2", intent).build())
                .build();

        assertTrue(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsDifferentIntent() {
        PendingIntent intent1 = mock(PendingIntent.class);
        PendingIntent intent2 = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent1).build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent2).build())
                .build();

        assertFalse(Notification.areActionsVisiblyDifferent(n1, n2));
    }

    @Test
    public void testActionsIgnoresRemoteInputs() {
        PendingIntent intent = mock(PendingIntent.class);
        Icon icon = mock(Icon.class);

        Notification n1 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent)
                        .addRemoteInput(new RemoteInput.Builder("a")
                                .setChoices(new CharSequence[] {"i", "m"})
                                .build())
                        .build())
                .build();
        Notification n2 = new Notification.Builder(mContext, "test")
                .addAction(new Notification.Action.Builder(icon, "TEXT 1", intent)
                        .addRemoteInput(new RemoteInput.Builder("a")
                                .setChoices(new CharSequence[] {"t", "m"})
                                .build())
                        .build())
                .build();

        assertFalse(Notification.areActionsVisiblyDifferent(n1, n2));
    }
}

