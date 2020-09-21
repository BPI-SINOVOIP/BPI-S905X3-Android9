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

package android.text.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.res.Configuration;
import android.os.LocaleList;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.method.LinkMovementMethod;
import android.text.style.URLSpan;
import android.util.Patterns;
import android.view.textclassifier.SystemTextClassifier;
import android.view.textclassifier.TextClassificationConstants;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextClassifierImpl;
import android.view.textclassifier.TextLinks.TextLinkSpan;
import android.widget.TextView;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/**
 * LinkifyTest tests {@link Linkify}.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class LinkifyTest {

    private static final LocaleList LOCALE_LIST_US = new LocaleList(Locale.US);
    private LocaleList mOriginalLocales;
    private Context mContext;

    @Before
    public void setup() {
        mContext = InstrumentationRegistry.getContext();
        mOriginalLocales = LocaleList.getDefault();
        LocaleList.setDefault(LOCALE_LIST_US);
    }

    @After
    public void teardown() {
        LocaleList.setDefault(mOriginalLocales);
    }

    @Test
    public void testNothing() {
        TextView tv;

        tv = new TextView(createUsEnglishContext());
        tv.setText("Hey, foo@google.com, call 415-555-1212.");

        assertFalse(tv.getMovementMethod() instanceof LinkMovementMethod);
        assertTrue(tv.getUrls().length == 0);
    }

    @Test
    public void testNormal() {
        TextView tv;

        tv = new TextView(createUsEnglishContext());
        tv.setAutoLinkMask(Linkify.ALL);
        tv.setText("Hey, foo@google.com, call +1-415-555-1212.");

        assertTrue(tv.getMovementMethod() instanceof LinkMovementMethod);
        assertTrue(tv.getUrls().length == 2);
    }

    @Test
    public void testUnclickable() {
        TextView tv;

        tv = new TextView(createUsEnglishContext());
        tv.setAutoLinkMask(Linkify.ALL);
        tv.setLinksClickable(false);
        tv.setText("Hey, foo@google.com, call +1-415-555-1212.");

        assertFalse(tv.getMovementMethod() instanceof LinkMovementMethod);
        assertTrue(tv.getUrls().length == 2);
    }

    private Context createUsEnglishContext() {
        final Configuration overrideConfig = new Configuration();
        overrideConfig.setLocales(LOCALE_LIST_US);
        return mContext.createConfigurationContext(overrideConfig);
    }

    @Test
    public void testAddLinks_addsLinksWhenDefaultSchemeIsNull() {
        Spannable spannable = new SpannableString("any https://android.com any android.com any");
        Linkify.addLinks(spannable, Patterns.AUTOLINK_WEB_URL, null, null, null);

        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        assertEquals("android.com and https://android.com should be linkified", 2, spans.length);
        assertEquals("https://android.com", spans[0].getURL());
        assertEquals("android.com", spans[1].getURL());
    }

    @Test
    public void testAddLinks_addsLinksWhenSchemesArrayIsNull() {
        Spannable spannable = new SpannableString("any https://android.com any android.com any");
        Linkify.addLinks(spannable, Patterns.AUTOLINK_WEB_URL, "http://", null, null);

        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        assertEquals("android.com and https://android.com should be linkified", 2, spans.length);
        // expected behavior, passing null schemes array means: prepend defaultScheme to all links.
        assertEquals("http://https://android.com", spans[0].getURL());
        assertEquals("http://android.com", spans[1].getURL());
    }

    @Test
    public void testAddLinks_prependsDefaultSchemeToBeginingOfLink() {
        Spannable spannable = new SpannableString("any android.com any");
        Linkify.addLinks(spannable, Patterns.AUTOLINK_WEB_URL, "http://",
                new String[] { "http://", "https://"}, null, null);

        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        assertEquals("android.com should be linkified", 1, spans.length);
        assertEquals("http://android.com", spans[0].getURL());
    }

    @Test
    public void testAddLinks_doesNotPrependSchemeIfSchemeExists() {
        Spannable spannable = new SpannableString("any https://android.com any");
        Linkify.addLinks(spannable, Patterns.AUTOLINK_WEB_URL, "http://",
                new String[] { "http://", "https://"}, null, null);

        URLSpan[] spans = spannable.getSpans(0, spannable.length(), URLSpan.class);
        assertEquals("android.com should be linkified", 1, spans.length);
        assertEquals("https://android.com", spans[0].getURL());
    }

    @Test
    public void testAddLinksAsync_useLegacyIfSmartDisabled_localTextClassifier()
            throws Exception {
        final TextClassificationConstants settings =
                TextClassificationConstants.loadFromString("smart_linkify_enabled=false");
        final TextClassifier classifier = new TextClassifierImpl(mContext, settings);
        testAddLinksAsync_useLegacyIfSmartDisabled(classifier);
    }

    @Test
    public void testAddLinksAsync_useLegacyIfSmartDisabled_systemTextClassifier()
            throws Exception {
        final TextClassificationConstants settings =
                TextClassificationConstants.loadFromString("smart_linkify_enabled=false");
        final TextClassifier classifier = new SystemTextClassifier(mContext, settings);
        testAddLinksAsync_useLegacyIfSmartDisabled(classifier);
    }

    private void testAddLinksAsync_useLegacyIfSmartDisabled(TextClassifier classifier)
            throws Exception {
        final int linkMask = Linkify.EMAIL_ADDRESSES | Linkify.PHONE_NUMBERS | Linkify.WEB_URLS;
        final String string = "example@android.com\n"
                + "(415) 555-1212\n"
                + "http://android.com\n"
                + "100 Android Rd California";
        final Spannable expected = new SpannableString(string);
        final Spannable actual = new SpannableString(string);

        Linkify.addLinks(expected, linkMask);  // legacy.
        Linkify.addLinksAsync(actual, classifier, linkMask).get();

        final URLSpan[] expectedSpans = expected.getSpans(0, expected.length(), URLSpan.class);
        final TextLinkSpan[] actualSpans = actual.getSpans(0, actual.length(), TextLinkSpan.class);
        assertEquals(expectedSpans.length, actualSpans.length);
        final Set<List> expectedLinkSpec = new HashSet<>();
        final Set<List> actualLinkSpec = new HashSet<>();
        for (int i = 0; i < expectedSpans.length; i++) {
            final URLSpan expectedSpan = expectedSpans[i];
            final TextLinkSpan actualSpan = actualSpans[i];
            expectedLinkSpec.add(Arrays.asList(
                    expected.getSpanStart(expectedSpan),
                    expected.getSpanEnd(expectedSpan),
                    expectedSpan.getURL()));
            actualLinkSpec.add(Arrays.asList(
                    actual.getSpanStart(actualSpan),
                    actual.getSpanEnd(actualSpan),
                    actualSpan.getUrl()));
        }
        assertEquals(expectedLinkSpec, actualLinkSpec);
    }
}
