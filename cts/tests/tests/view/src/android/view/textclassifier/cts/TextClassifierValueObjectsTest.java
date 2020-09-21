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

package android.view.textclassifier.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.app.PendingIntent;
import android.app.RemoteAction;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.Icon;
import android.os.LocaleList;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.View;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextSelection;

import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * TextClassifier value objects tests.
 *
 * <p>Contains unit tests for value objects passed to/from TextClassifier APIs.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TextClassifierValueObjectsTest {

    private static final double ACCEPTED_DELTA = 0.0000001;
    private static final String TEXT = "abcdefghijklmnopqrstuvwxyz";
    private static final int START = 5;
    private static final int END = 20;
    private static final String ID = "id123";
    private static final LocaleList LOCALES = LocaleList.forLanguageTags("fr,en,de,es");

    @Test
    public void testTextSelection() {
        final float addressScore = 0.1f;
        final float emailScore = 0.9f;

        final TextSelection selection = new TextSelection.Builder(START, END)
                .setEntityType(TextClassifier.TYPE_ADDRESS, addressScore)
                .setEntityType(TextClassifier.TYPE_EMAIL, emailScore)
                .setId(ID)
                .build();

        assertEquals(START, selection.getSelectionStartIndex());
        assertEquals(END, selection.getSelectionEndIndex());
        assertEquals(2, selection.getEntityCount());
        assertEquals(TextClassifier.TYPE_EMAIL, selection.getEntity(0));
        assertEquals(TextClassifier.TYPE_ADDRESS, selection.getEntity(1));
        assertEquals(addressScore, selection.getConfidenceScore(TextClassifier.TYPE_ADDRESS),
                ACCEPTED_DELTA);
        assertEquals(emailScore, selection.getConfidenceScore(TextClassifier.TYPE_EMAIL),
                ACCEPTED_DELTA);
        assertEquals(0, selection.getConfidenceScore("random_type"), ACCEPTED_DELTA);
        assertEquals(ID, selection.getId());
    }

    @Test
    public void testTextSelection_differentParams() {
        final int start = 0;
        final int end = 1;
        final float confidenceScore = 0.5f;
        final String id = "2hukwu3m3k44f1gb0";

        final TextSelection selection = new TextSelection.Builder(start, end)
                .setEntityType(TextClassifier.TYPE_URL, confidenceScore)
                .setId(id)
                .build();

        assertEquals(start, selection.getSelectionStartIndex());
        assertEquals(end, selection.getSelectionEndIndex());
        assertEquals(1, selection.getEntityCount());
        assertEquals(TextClassifier.TYPE_URL, selection.getEntity(0));
        assertEquals(confidenceScore, selection.getConfidenceScore(TextClassifier.TYPE_URL),
                ACCEPTED_DELTA);
        assertEquals(0, selection.getConfidenceScore("random_type"), ACCEPTED_DELTA);
        assertEquals(id, selection.getId());
    }

    @Test
    public void testTextSelection_defaultValues() {
        TextSelection selection = new TextSelection.Builder(START, END).build();
        assertEquals(0, selection.getEntityCount());
        assertNull(selection.getId());
    }

    @Test
    public void testTextSelection_prunedConfidenceScore() {
        final float phoneScore = -0.1f;
        final float prunedPhoneScore = 0f;
        final float otherScore = 1.5f;
        final float prunedOtherScore = 1.0f;

        final TextSelection selection = new TextSelection.Builder(START, END)
                .setEntityType(TextClassifier.TYPE_PHONE, phoneScore)
                .setEntityType(TextClassifier.TYPE_OTHER, otherScore)
                .build();

        assertEquals(prunedPhoneScore, selection.getConfidenceScore(TextClassifier.TYPE_PHONE),
                ACCEPTED_DELTA);
        assertEquals(prunedOtherScore, selection.getConfidenceScore(TextClassifier.TYPE_OTHER),
                ACCEPTED_DELTA);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testTextSelection_invalidStartParams() {
        new TextSelection.Builder(-1 /* start */, END)
                .build();
    }

    @Test(expected = IllegalArgumentException.class)
    public void testTextSelection_invalidEndParams() {
        new TextSelection.Builder(START, 0 /* end */)
                .build();
    }

    @Test(expected = IndexOutOfBoundsException.class)
    public void testTextSelection_entityIndexOutOfBounds() {
        final TextSelection selection = new TextSelection.Builder(START, END).build();
        final int outOfBoundsIndex = selection.getEntityCount();
        selection.getEntity(outOfBoundsIndex);
    }

    @Test
    public void testTextSelectionRequest() {
        final TextSelection.Request request = new TextSelection.Request.Builder(TEXT, START, END)
                .setDefaultLocales(LOCALES)
                .build();
        assertEquals(TEXT, request.getText());
        assertEquals(START, request.getStartIndex());
        assertEquals(END, request.getEndIndex());
        assertEquals(LOCALES, request.getDefaultLocales());
    }

    @Test
    public void testTextSelectionRequest_nullValues() {
        final TextSelection.Request request =
                new TextSelection.Request.Builder(TEXT, START, END)
                        .setDefaultLocales(null)
                        .build();
        assertNull(request.getDefaultLocales());
    }

    @Test
    public void testTextSelectionRequest_defaultValues() {
        final TextSelection.Request request =
                new TextSelection.Request.Builder(TEXT, START, END).build();
        assertNull(request.getDefaultLocales());
    }

    @Test
    public void testTextClassification() {
        final float addressScore = 0.1f;
        final float emailScore = 0.9f;
        final PendingIntent intent1 = PendingIntent.getActivity(
                InstrumentationRegistry.getTargetContext(), 0, new Intent(), 0);
        final String label1 = "label1";
        final String description1 = "description1";
        final Icon icon1 = generateTestIcon(16, 16, Color.RED);
        final PendingIntent intent2 = PendingIntent.getActivity(
                InstrumentationRegistry.getTargetContext(), 0, new Intent(), 0);
        final String label2 = "label2";
        final String description2 = "description2";
        final Icon icon2 = generateTestIcon(16, 16, Color.GREEN);

        final TextClassification classification = new TextClassification.Builder()
                .setText(TEXT)
                .setEntityType(TextClassifier.TYPE_ADDRESS, addressScore)
                .setEntityType(TextClassifier.TYPE_EMAIL, emailScore)
                .addAction(new RemoteAction(icon1, label1, description1, intent1))
                .addAction(new RemoteAction(icon2, label2, description2, intent2))
                .setId(ID)
                .build();

        assertEquals(TEXT, classification.getText());
        assertEquals(2, classification.getEntityCount());
        assertEquals(TextClassifier.TYPE_EMAIL, classification.getEntity(0));
        assertEquals(TextClassifier.TYPE_ADDRESS, classification.getEntity(1));
        assertEquals(addressScore, classification.getConfidenceScore(TextClassifier.TYPE_ADDRESS),
                ACCEPTED_DELTA);
        assertEquals(emailScore, classification.getConfidenceScore(TextClassifier.TYPE_EMAIL),
                ACCEPTED_DELTA);
        assertEquals(0, classification.getConfidenceScore("random_type"), ACCEPTED_DELTA);

        // Legacy API
        assertNull(classification.getIntent());
        assertNull(classification.getLabel());
        assertNull(classification.getIcon());
        assertNull(classification.getOnClickListener());

        assertEquals(2, classification.getActions().size());
        assertEquals(label1, classification.getActions().get(0).getTitle());
        assertEquals(description1, classification.getActions().get(0).getContentDescription());
        assertEquals(intent1, classification.getActions().get(0).getActionIntent());
        assertNotNull(classification.getActions().get(0).getIcon());
        assertEquals(label2, classification.getActions().get(1).getTitle());
        assertEquals(description2, classification.getActions().get(1).getContentDescription());
        assertEquals(intent2, classification.getActions().get(1).getActionIntent());
        assertNotNull(classification.getActions().get(1).getIcon());
        assertEquals(ID, classification.getId());
    }

    @Test
    public void testTextClassificationLegacy() {
        final float addressScore = 0.1f;
        final float emailScore = 0.9f;
        final Intent intent = new Intent();
        final String label = "label";
        final Drawable icon = new ColorDrawable(Color.RED);
        final View.OnClickListener onClick = v -> { };

        final TextClassification classification = new TextClassification.Builder()
                .setText(TEXT)
                .setEntityType(TextClassifier.TYPE_ADDRESS, addressScore)
                .setEntityType(TextClassifier.TYPE_EMAIL, emailScore)
                .setIntent(intent)
                .setLabel(label)
                .setIcon(icon)
                .setOnClickListener(onClick)
                .setId(ID)
                .build();

        assertEquals(TEXT, classification.getText());
        assertEquals(2, classification.getEntityCount());
        assertEquals(TextClassifier.TYPE_EMAIL, classification.getEntity(0));
        assertEquals(TextClassifier.TYPE_ADDRESS, classification.getEntity(1));
        assertEquals(addressScore, classification.getConfidenceScore(TextClassifier.TYPE_ADDRESS),
                ACCEPTED_DELTA);
        assertEquals(emailScore, classification.getConfidenceScore(TextClassifier.TYPE_EMAIL),
                ACCEPTED_DELTA);
        assertEquals(0, classification.getConfidenceScore("random_type"), ACCEPTED_DELTA);

        assertEquals(intent, classification.getIntent());
        assertEquals(label, classification.getLabel());
        assertEquals(icon, classification.getIcon());
        assertEquals(onClick, classification.getOnClickListener());
        assertEquals(ID, classification.getId());
    }

    @Test
    public void testTextClassification_defaultValues() {
        final TextClassification classification = new TextClassification.Builder().build();

        assertEquals(null, classification.getText());
        assertEquals(0, classification.getEntityCount());
        assertEquals(null, classification.getIntent());
        assertEquals(null, classification.getLabel());
        assertEquals(null, classification.getIcon());
        assertEquals(null, classification.getOnClickListener());
        assertEquals(0, classification.getActions().size());
        assertNull(classification.getId());
    }

    @Test
    public void testTextClassificationRequest() {
        final TextClassification.Request request =
                new TextClassification.Request.Builder(TEXT, START, END)
                        .setDefaultLocales(LOCALES)
                        .build();
        assertEquals(LOCALES, request.getDefaultLocales());
    }

    @Test
    public void testTextClassificationRequest_nullValues() {
        final TextClassification.Request request =
                new TextClassification.Request.Builder(TEXT, START, END)
                        .setDefaultLocales(null)
                        .build();
        assertNull(request.getDefaultLocales());
    }

    @Test
    public void testTextClassificationRequest_defaultValues() {
        final TextClassification.Request request =
                new TextClassification.Request.Builder(TEXT, START, END).build();
        assertNull(request.getDefaultLocales());
    }

    // TODO: Add more tests.

    /** Helper to generate Icons for testing. */
    private Icon generateTestIcon(int width, int height, int colorValue) {
        final int numPixels = width * height;
        final int[] colors = new int[numPixels];
        for (int i = 0; i < numPixels; ++i) {
            colors[i] = colorValue;
        }
        final Bitmap bitmap = Bitmap.createBitmap(colors, width, height, Bitmap.Config.ARGB_8888);
        return Icon.createWithBitmap(bitmap);
    }
}
