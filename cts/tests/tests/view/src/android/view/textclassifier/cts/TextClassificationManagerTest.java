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
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import android.os.LocaleList;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.textclassifier.SelectionEvent;
import android.view.textclassifier.TextClassification;
import android.view.textclassifier.TextClassificationContext;
import android.view.textclassifier.TextClassificationManager;
import android.view.textclassifier.TextClassifier;
import android.view.textclassifier.TextLinks;
import android.view.textclassifier.TextSelection;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class TextClassificationManagerTest {

    private static final LocaleList LOCALES = LocaleList.forLanguageTags("en");
    private static final int START = 1;
    private static final int END = 3;
    // This text has lots of things that are probably entities in many cases.
    private static final String TEXT = "An email address is test@example.com. A phone number"
            + " might be +12122537077. Somebody lives at 123 Main Street, Mountain View, CA,"
            + " and there's good stuff at https://www.android.com :)";
    private static final TextSelection.Request TEXT_SELECTION_REQUEST =
            new TextSelection.Request.Builder(TEXT, START, END)
                    .setDefaultLocales(LOCALES)
                    .build();
    private static final TextClassification.Request TEXT_CLASSIFICATION_REQUEST =
            new TextClassification.Request.Builder(TEXT, START, END)
                    .setDefaultLocales(LOCALES)
                    .build();

    private TextClassificationManager mManager;
    private TextClassifier mClassifier;

    @Before
    public void setup() {
        mManager = InstrumentationRegistry.getTargetContext()
                .getSystemService(TextClassificationManager.class);
        mManager.setTextClassifier(null); // Resets the classifier.
        mClassifier = mManager.createTextClassificationSession(
                new TextClassificationContext.Builder(
                        InstrumentationRegistry.getTargetContext().getPackageName(),
                        TextClassifier.WIDGET_TYPE_TEXTVIEW)
                        .build());
    }

    @After
    public void tearDown() {
        mClassifier.destroy();
    }

    @Test
    public void testSetTextClassifier() {
        final TextClassifier classifier = mock(TextClassifier.class);
        mManager.setTextClassifier(classifier);
        assertEquals(classifier, mManager.getTextClassifier());
    }

    @Test
    public void testSmartSelection() {
        assertValidResult(mClassifier.suggestSelection(TEXT_SELECTION_REQUEST));
    }

    @Test
    public void testClassifyText() {
        assertValidResult(mClassifier.classifyText(TEXT_CLASSIFICATION_REQUEST));
    }

    @Test
    public void testNoOpClassifier() {
        mManager.setTextClassifier(TextClassifier.NO_OP);
        mClassifier = mManager.getTextClassifier();

        final TextSelection selection = mClassifier.suggestSelection(TEXT_SELECTION_REQUEST);
        assertValidResult(selection);
        assertEquals(START, selection.getSelectionStartIndex());
        assertEquals(END, selection.getSelectionEndIndex());
        assertEquals(0, selection.getEntityCount());

        final TextClassification classification =
                mClassifier.classifyText(TEXT_CLASSIFICATION_REQUEST);
        assertValidResult(classification);
        assertNull(classification.getText());
        assertEquals(0, classification.getEntityCount());
        assertEquals(0, classification.getActions().size());
        assertNull(classification.getIcon());
        assertNull(classification.getLabel());
        assertNull(classification.getIntent());
        assertNull(classification.getOnClickListener());
    }

    @Test
    public void testGenerateLinks() {
        assertValidResult(mClassifier.generateLinks(new TextLinks.Request.Builder(TEXT).build()));
    }

    @Test
    public void testResolveEntityListModifications_only_hints() {
        TextClassifier.EntityConfig entityConfig = TextClassifier.EntityConfig.createWithHints(
                Arrays.asList("some_hint"));
        assertEquals(1, entityConfig.getHints().size());
        assertTrue(entityConfig.getHints().contains("some_hint"));
        assertEquals(new HashSet<String>(Arrays.asList("foo", "bar")),
                entityConfig.resolveEntityListModifications(Arrays.asList("foo", "bar")));
    }

    @Test
    public void testResolveEntityListModifications_include_exclude() {
        TextClassifier.EntityConfig entityConfig = TextClassifier.EntityConfig.create(
                Arrays.asList("some_hint"),
                Arrays.asList("a", "b", "c"),
                Arrays.asList("b", "d", "x"));
        assertEquals(1, entityConfig.getHints().size());
        assertTrue(entityConfig.getHints().contains("some_hint"));
        assertEquals(new HashSet(Arrays.asList("a", "c", "w")),
                new HashSet(entityConfig.resolveEntityListModifications(
                        Arrays.asList("c", "w", "x"))));
    }

    @Test
    public void testResolveEntityListModifications_explicit() {
        TextClassifier.EntityConfig entityConfig =
                TextClassifier.EntityConfig.createWithExplicitEntityList(Arrays.asList("a", "b"));
        assertEquals(Collections.EMPTY_LIST, entityConfig.getHints());
        assertEquals(new HashSet<String>(Arrays.asList("a", "b")),
                entityConfig.resolveEntityListModifications(Arrays.asList("w", "x")));
    }

    @Test
    public void testOnSelectionEvent() {
        // Doesn't crash.
        mClassifier.onSelectionEvent(
                SelectionEvent.createSelectionStartedEvent(SelectionEvent.INVOCATION_MANUAL, 0));
    }

    private static void assertValidResult(TextSelection selection) {
        assertNotNull(selection);
        assertTrue(selection.getSelectionStartIndex() >= 0);
        assertTrue(selection.getSelectionEndIndex() > selection.getSelectionStartIndex());
        assertTrue(selection.getEntityCount() >= 0);
        for (int i = 0; i < selection.getEntityCount(); i++) {
            final String entity = selection.getEntity(i);
            assertNotNull(entity);
            final float confidenceScore = selection.getConfidenceScore(entity);
            assertTrue(confidenceScore >= 0);
            assertTrue(confidenceScore <= 1);
        }
    }

    private static void assertValidResult(TextClassification classification) {
        assertNotNull(classification);
        assertTrue(classification.getEntityCount() >= 0);
        for (int i = 0; i < classification.getEntityCount(); i++) {
            final String entity = classification.getEntity(i);
            assertNotNull(entity);
            final float confidenceScore = classification.getConfidenceScore(entity);
            assertTrue(confidenceScore >= 0);
            assertTrue(confidenceScore <= 1);
        }
        assertNotNull(classification.getActions());
    }

    private static void assertValidResult(TextLinks links) {
        assertNotNull(links);
        for (TextLinks.TextLink link : links.getLinks()) {
            assertTrue(link.getEntityCount() > 0);
            assertTrue(link.getStart() >= 0);
            assertTrue(link.getStart() <= link.getEnd());
            for (int i = 0; i < link.getEntityCount(); i++) {
                String entityType = link.getEntity(i);
                assertNotNull(entityType);
                final float confidenceScore = link.getConfidenceScore(entityType);
                assertTrue(confidenceScore >= 0);
                assertTrue(confidenceScore <= 1);
            }
        }
    }
}
