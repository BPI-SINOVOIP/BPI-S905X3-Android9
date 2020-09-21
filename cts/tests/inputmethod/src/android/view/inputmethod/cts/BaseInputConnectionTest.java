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

package android.view.inputmethod.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.content.ClipDescription;
import android.net.Uri;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.text.Editable;
import android.text.Selection;
import android.text.Spannable;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.TextUtils;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CompletionInfo;
import android.view.inputmethod.ExtractedTextRequest;
import android.view.inputmethod.InputContentInfo;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.cts.util.InputConnectionTestUtils;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class BaseInputConnectionTest {

    private static BaseInputConnection createBaseInputConnection() {
        final View view = new View(InstrumentationRegistry.getTargetContext());
        return new BaseInputConnection(view, true);
    }

    @Test
    public void testDefaultMethods() {
        // These methods are default to return fixed result.
        final BaseInputConnection connection = createBaseInputConnection();

        assertFalse(connection.beginBatchEdit());
        assertFalse(connection.endBatchEdit());

        // only fit for test default implementation of commitCompletion.
        int completionId = 1;
        String completionString = "commitCompletion test";
        assertFalse(connection.commitCompletion(new CompletionInfo(completionId,
                0, completionString)));

        assertNull(connection.getExtractedText(new ExtractedTextRequest(), 0));

        // only fit for test default implementation of performEditorAction.
        int actionCode = 1;
        int actionId = 2;
        String action = "android.intent.action.MAIN";
        assertTrue(connection.performEditorAction(actionCode));
        assertFalse(connection.performContextMenuAction(actionId));
        assertFalse(connection.performPrivateCommand(action, new Bundle()));
    }

    @Test
    public void testOpComposingSpans() {
        Spannable text = new SpannableString("Test ComposingSpans");
        BaseInputConnection.setComposingSpans(text);
        assertTrue(BaseInputConnection.getComposingSpanStart(text) > -1);
        assertTrue(BaseInputConnection.getComposingSpanEnd(text) > -1);
        BaseInputConnection.removeComposingSpans(text);
        assertTrue(BaseInputConnection.getComposingSpanStart(text) == -1);
        assertTrue(BaseInputConnection.getComposingSpanEnd(text) == -1);
    }

    /**
     * getEditable: Return the target of edit operations. The default implementation
     *              returns its own fake editable that is just used for composing text.
     * clearMetaKeyStates: Default implementation uses
     *              MetaKeyKeyListener#clearMetaKeyState(long, int) to clear the state.
     *              BugId:1738511
     * commitText: Default implementation replaces any existing composing text with the given
     *             text.
     * deleteSurroundingText: The default implementation performs the deletion around the current
     *              selection position of the editable text.
     * getCursorCapsMode: The default implementation uses TextUtils.getCapsMode to get the
     *                  cursor caps mode for the current selection position in the editable text.
     *                  TextUtils.getCapsMode is tested fully in TextUtilsTest#testGetCapsMode.
     * getTextBeforeCursor, getTextAfterCursor: The default implementation performs the deletion
     *                          around the current selection position of the editable text.
     * setSelection: changes the selection position in the current editable text.
     */
    @Test
    public void testOpTextMethods() {
        final BaseInputConnection connection = createBaseInputConnection();

        // return is an default Editable instance with empty source
        final Editable text = connection.getEditable();
        assertNotNull(text);
        assertEquals(0, text.length());

        // Test commitText, not dummy mode
        CharSequence str = "TestCommit ";
        Editable inputText = Editable.Factory.getInstance().newEditable(str);
        connection.commitText(inputText, inputText.length());
        final Editable text2 = connection.getEditable();
        int strLength = str.length();
        assertEquals(strLength, text2.length());
        assertEquals(str.toString(), text2.toString());
        assertEquals(TextUtils.CAP_MODE_WORDS,
                connection.getCursorCapsMode(TextUtils.CAP_MODE_WORDS));
        int offLength = 3;
        CharSequence expected = str.subSequence(strLength - offLength, strLength);
        assertEquals(expected.toString(), connection.getTextBeforeCursor(offLength,
                BaseInputConnection.GET_TEXT_WITH_STYLES).toString());
        connection.setSelection(0, 0);
        expected = str.subSequence(0, offLength);
        assertEquals(expected.toString(), connection.getTextAfterCursor(offLength,
                BaseInputConnection.GET_TEXT_WITH_STYLES).toString());

        // Test deleteSurroundingText
        int end = text2.length();
        connection.setSelection(end, end);
        // Delete the ending space
        assertTrue(connection.deleteSurroundingText(1, 2));
        Editable text3 = connection.getEditable();
        assertEquals(strLength - 1, text3.length());
        String expectedDelString = "TestCommit";
        assertEquals(expectedDelString, text3.toString());
    }

    /**
     * finishComposingText: The default implementation removes the composing state from the
     *                      current editable text.
     * setComposingText: The default implementation places the given text into the editable,
     *                  replacing any existing composing text
     */
    @Test
    public void testFinishComposingText() {
        final BaseInputConnection connection = createBaseInputConnection();
        CharSequence str = "TestFinish";
        Editable inputText = Editable.Factory.getInstance().newEditable(str);
        connection.commitText(inputText, inputText.length());
        final Editable text = connection.getEditable();
        // Test finishComposingText, not dummy mode
        BaseInputConnection.setComposingSpans(text);
        assertTrue(BaseInputConnection.getComposingSpanStart(text) > -1);
        assertTrue(BaseInputConnection.getComposingSpanEnd(text) > -1);
        connection.finishComposingText();
        assertTrue(BaseInputConnection.getComposingSpanStart(text) == -1);
        assertTrue(BaseInputConnection.getComposingSpanEnd(text) == -1);
    }

    /**
     * Updates InputMethodManager with the current fullscreen mode.
     */
    @Test
    public void testReportFullscreenMode() {
        final InputMethodManager imm = InstrumentationRegistry.getInstrumentation()
                .getTargetContext().getSystemService(InputMethodManager.class);
        final BaseInputConnection connection = createBaseInputConnection();
        connection.reportFullscreenMode(false);
        assertFalse(imm.isFullscreenMode());
        connection.reportFullscreenMode(true);
        // Only IMEs are allowed to report full-screen mode.  Calling this method from the
        // application should have no effect.
        assertFalse(imm.isFullscreenMode());
    }

    /**
     * An utility method to create an instance of {@link BaseInputConnection} in the full editor
     * mode with an initial text and selection range.
     *
     * @param source the initial text.
     * @return {@link BaseInputConnection} instantiated in the full editor mode with {@code source}
     *         and selection range from {@code selectionStart} to {@code selectionEnd}
     */
    private static BaseInputConnection createConnectionWithSelection(CharSequence source) {
        final int selectionStart = Selection.getSelectionStart(source);
        final int selectionEnd = Selection.getSelectionEnd(source);
        final Editable editable = Editable.Factory.getInstance().newEditable(source);
        Selection.setSelection(editable, selectionStart, selectionEnd);
        final View view = new View(InstrumentationRegistry.getTargetContext());
        return new BaseInputConnection(view, true) {
            @Override
            public Editable getEditable() {
                return editable;
            }
        };
    }

    private static void verifyDeleteSurroundingTextMain(final String initialState,
            final int deleteBefore, final int deleteAfter, final String expectedState) {
        final CharSequence source = InputConnectionTestUtils.formatString(initialState);
        final BaseInputConnection ic = createConnectionWithSelection(source);
        ic.deleteSurroundingText(deleteBefore, deleteAfter);

        final CharSequence expectedString = InputConnectionTestUtils.formatString(expectedState);
        final int expectedSelectionStart = Selection.getSelectionStart(expectedString);
        final int expectedSelectionEnd = Selection.getSelectionEnd(expectedString);

        // It is sufficient to check the surrounding text up to source.length() characters, because
        // InputConnection.deleteSurroundingText() is not supposed to increase the text length.
        final int retrievalLength = source.length();
        if (expectedSelectionStart == 0) {
            assertTrue(TextUtils.isEmpty(ic.getTextBeforeCursor(retrievalLength, 0)));
        } else {
            assertEquals(expectedString.subSequence(0, expectedSelectionStart).toString(),
                    ic.getTextBeforeCursor(retrievalLength, 0).toString());
        }
        if (expectedSelectionStart == expectedSelectionEnd) {
            assertTrue(TextUtils.isEmpty(ic.getSelectedText(0)));  // null is allowed.
        } else {
            assertEquals(expectedString.subSequence(expectedSelectionStart,
                    expectedSelectionEnd).toString(), ic.getSelectedText(0).toString());
        }
        if (expectedSelectionEnd == expectedString.length()) {
            assertTrue(TextUtils.isEmpty(ic.getTextAfterCursor(retrievalLength, 0)));
        } else {
            assertEquals(expectedString.subSequence(expectedSelectionEnd,
                    expectedString.length()).toString(),
                    ic.getTextAfterCursor(retrievalLength, 0).toString());
        }
    }

    /**
     * Tests {@link BaseInputConnection#deleteSurroundingText(int, int)} comprehensively.
     */
    @Test
    public void testDeleteSurroundingText() {
        verifyDeleteSurroundingTextMain("012[]3456789", 0, 0, "012[]3456789");
        verifyDeleteSurroundingTextMain("012[]3456789", -1, -1, "012[]3456789");
        verifyDeleteSurroundingTextMain("012[]3456789", 1, 2, "01[]56789");
        verifyDeleteSurroundingTextMain("012[]3456789", 10, 1, "[]456789");
        verifyDeleteSurroundingTextMain("012[]3456789", 1, 10, "01[]");
        verifyDeleteSurroundingTextMain("[]0123456789", 3, 3, "[]3456789");
        verifyDeleteSurroundingTextMain("0123456789[]", 3, 3, "0123456[]");
        verifyDeleteSurroundingTextMain("012[345]6789", 0, 0, "012[345]6789");
        verifyDeleteSurroundingTextMain("012[345]6789", -1, -1, "012[345]6789");
        verifyDeleteSurroundingTextMain("012[345]6789", 1, 2, "01[345]89");
        verifyDeleteSurroundingTextMain("012[345]6789", 10, 1, "[345]789");
        verifyDeleteSurroundingTextMain("012[345]6789", 1, 10, "01[345]");
        verifyDeleteSurroundingTextMain("[012]3456789", 3, 3, "[012]6789");
        verifyDeleteSurroundingTextMain("0123456[789]", 3, 3, "0123[789]");
        verifyDeleteSurroundingTextMain("[0123456789]", 0, 0, "[0123456789]");
        verifyDeleteSurroundingTextMain("[0123456789]", 1, 1, "[0123456789]");

        // Surrogate characters do not have any special meanings.  Validating the character sequence
        // is beyond the goal of this API.
        verifyDeleteSurroundingTextMain("0<>[]3456789", 1, 0, "0<[]3456789");
        verifyDeleteSurroundingTextMain("0<>[]3456789", 2, 0, "0[]3456789");
        verifyDeleteSurroundingTextMain("0<>[]3456789", 3, 0, "[]3456789");
        verifyDeleteSurroundingTextMain("012[]<>56789", 0, 1, "012[]>56789");
        verifyDeleteSurroundingTextMain("012[]<>56789", 0, 2, "012[]56789");
        verifyDeleteSurroundingTextMain("012[]<>56789", 0, 3, "012[]6789");
        verifyDeleteSurroundingTextMain("0<<[]3456789", 1, 0, "0<[]3456789");
        verifyDeleteSurroundingTextMain("0<<[]3456789", 2, 0, "0[]3456789");
        verifyDeleteSurroundingTextMain("0<<[]3456789", 3, 0, "[]3456789");
        verifyDeleteSurroundingTextMain("012[]<<56789", 0, 1, "012[]<56789");
        verifyDeleteSurroundingTextMain("012[]<<56789", 0, 2, "012[]56789");
        verifyDeleteSurroundingTextMain("012[]<<56789", 0, 3, "012[]6789");
        verifyDeleteSurroundingTextMain("0>>[]3456789", 1, 0, "0>[]3456789");
        verifyDeleteSurroundingTextMain("0>>[]3456789", 2, 0, "0[]3456789");
        verifyDeleteSurroundingTextMain("0>>[]3456789", 3, 0, "[]3456789");
        verifyDeleteSurroundingTextMain("012[]>>56789", 0, 1, "012[]>56789");
        verifyDeleteSurroundingTextMain("012[]>>56789", 0, 2, "012[]56789");
        verifyDeleteSurroundingTextMain("012[]>>56789", 0, 3, "012[]6789");
    }

    private static void verifyDeleteSurroundingTextInCodePointsMain(String initialState,
            int deleteBeforeInCodePoints, int deleteAfterInCodePoints, String expectedState) {
        final CharSequence source = InputConnectionTestUtils.formatString(initialState);
        final BaseInputConnection ic = createConnectionWithSelection(source);
        ic.deleteSurroundingTextInCodePoints(deleteBeforeInCodePoints, deleteAfterInCodePoints);

        final CharSequence expectedString = InputConnectionTestUtils.formatString(expectedState);
        final int expectedSelectionStart = Selection.getSelectionStart(expectedString);
        final int expectedSelectionEnd = Selection.getSelectionEnd(expectedString);

        // It is sufficient to check the surrounding text up to source.length() characters, because
        // InputConnection.deleteSurroundingTextInCodePoints() is not supposed to increase the text
        // length.
        final int retrievalLength = source.length();
        if (expectedSelectionStart == 0) {
            assertTrue(TextUtils.isEmpty(ic.getTextBeforeCursor(retrievalLength, 0)));
        } else {
            assertEquals(expectedString.subSequence(0, expectedSelectionStart).toString(),
                    ic.getTextBeforeCursor(retrievalLength, 0).toString());
        }
        if (expectedSelectionStart == expectedSelectionEnd) {
            assertTrue(TextUtils.isEmpty(ic.getSelectedText(0)));  // null is allowed.
        } else {
            assertEquals(expectedString.subSequence(expectedSelectionStart,
                    expectedSelectionEnd).toString(), ic.getSelectedText(0).toString());
        }
        if (expectedSelectionEnd == expectedString.length()) {
            assertTrue(TextUtils.isEmpty(ic.getTextAfterCursor(retrievalLength, 0)));
        } else {
            assertEquals(expectedString.subSequence(expectedSelectionEnd,
                    expectedString.length()).toString(),
                    ic.getTextAfterCursor(retrievalLength, 0).toString());
        }
    }

    /**
     * Tests {@link BaseInputConnection#deleteSurroundingTextInCodePoints(int, int)}
     * comprehensively.
     */
    @Test
    public void testDeleteSurroundingTextInCodePoints() {
        verifyDeleteSurroundingTextInCodePointsMain("012[]3456789", 0, 0, "012[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]3456789", -1, -1, "012[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]3456789", 1, 2, "01[]56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]3456789", 10, 1, "[]456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]3456789", 1, 10, "01[]");
        verifyDeleteSurroundingTextInCodePointsMain("[]0123456789", 3, 3, "[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0123456789[]", 3, 3, "0123456[]");
        verifyDeleteSurroundingTextInCodePointsMain("012[345]6789", 0, 0, "012[345]6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[345]6789", -1, -1, "012[345]6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[345]6789", 1, 2, "01[345]89");
        verifyDeleteSurroundingTextInCodePointsMain("012[345]6789", 10, 1, "[345]789");
        verifyDeleteSurroundingTextInCodePointsMain("012[345]6789", 1, 10, "01[345]");
        verifyDeleteSurroundingTextInCodePointsMain("[012]3456789", 3, 3, "[012]6789");
        verifyDeleteSurroundingTextInCodePointsMain("0123456[789]", 3, 3, "0123[789]");
        verifyDeleteSurroundingTextInCodePointsMain("[0123456789]", 0, 0, "[0123456789]");
        verifyDeleteSurroundingTextInCodePointsMain("[0123456789]", 1, 1, "[0123456789]");

        verifyDeleteSurroundingTextInCodePointsMain("0<>[]3456789", 1, 0, "0[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<>[]3456789", 2, 0, "[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<>[]3456789", 3, 0, "[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>56789", 0, 1, "012[]56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>56789", 0, 2, "012[]6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>56789", 0, 3, "012[]789");

        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 0, "[]<><><><><>");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 1, "[]<><><><>");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 2, "[]<><><>");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 3, "[]<><>");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 4, "[]<>");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 5, "[]");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 6, "[]");
        verifyDeleteSurroundingTextInCodePointsMain("[]<><><><><>", 0, 1000, "[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 0, 0, "<><><><><>[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 1, 0, "<><><><>[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 2, 0, "<><><>[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 3, 0, "<><>[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 4, 0, "<>[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 5, 0, "[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 6, 0, "[]");
        verifyDeleteSurroundingTextInCodePointsMain("<><><><><>[]", 1000, 0, "[]");

        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 1, 0, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 2, 0, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 3, 0, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 0, 1, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 0, 2, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 0, 3, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 1, 0, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 2, 0, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 3, 0, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 0, 1, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 0, 2, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 0, 3, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("01<[]>456789", 1, 0, "01<[]>456789");
        verifyDeleteSurroundingTextInCodePointsMain("01<[]>456789", 0, 1, "01<[]>456789");
        verifyDeleteSurroundingTextInCodePointsMain("<12[]3456789", 1, 0, "<1[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("<12[]3456789", 2, 0, "<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("<12[]3456789", 3, 0, "<12[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("<<>[]3456789", 1, 0, "<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("<<>[]3456789", 2, 0, "<<>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("<<>[]3456789", 3, 0, "<<>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]34>6789", 0, 1, "012[]4>6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]34>6789", 0, 2, "012[]>6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]34>6789", 0, 3, "012[]34>6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>>6789", 0, 1, "012[]>6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>>6789", 0, 2, "012[]<>>6789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<>>6789", 0, 3, "012[]<>>6789");

        // Atomicity test.
        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 1, 1, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 2, 1, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0<<[]3456789", 3, 1, "0<<[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 1, 1, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 1, 2, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]<<56789", 1, 3, "012[]<<56789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 1, 1, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 2, 1, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("0>>[]3456789", 3, 1, "0>>[]3456789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 1, 1, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 1, 2, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("012[]>>56789", 1, 3, "012[]>>56789");
        verifyDeleteSurroundingTextInCodePointsMain("01<[]>456789", 1, 1, "01<[]>456789");

        // Do not verify the character sequences in the selected region.
        verifyDeleteSurroundingTextInCodePointsMain("01[><]456789", 1, 0, "0[><]456789");
        verifyDeleteSurroundingTextInCodePointsMain("01[><]456789", 0, 1, "01[><]56789");
        verifyDeleteSurroundingTextInCodePointsMain("01[><]456789", 1, 1, "0[><]56789");
    }

    @Test
    public void testCloseConnection() {
        final BaseInputConnection connection = createBaseInputConnection();

        final CharSequence source = "0123456789";
        connection.commitText(source, source.length());
        connection.setComposingRegion(2, 5);
        final Editable text = connection.getEditable();
        assertEquals(2, BaseInputConnection.getComposingSpanStart(text));
        assertEquals(5, BaseInputConnection.getComposingSpanEnd(text));

        // BaseInputConnection#closeConnection() must clear the on-going composition.
        connection.closeConnection();
        assertEquals(-1, BaseInputConnection.getComposingSpanStart(text));
        assertEquals(-1, BaseInputConnection.getComposingSpanEnd(text));
    }

    @Test
    public void testGetHandler() {
        final BaseInputConnection connection = createBaseInputConnection();

        // BaseInputConnection must not implement getHandler().
        assertNull(connection.getHandler());
    }

    @Test
    public void testCommitContent() {
        final BaseInputConnection connection = createBaseInputConnection();

        final InputContentInfo inputContentInfo = new InputContentInfo(
                Uri.parse("content://com.example/path"),
                new ClipDescription("sample content", new String[]{"image/png"}),
                Uri.parse("https://example.com"));
        // The default implementation should do nothing and just return false.
        assertFalse(connection.commitContent(inputContentInfo, 0 /* flags */, null /* opts */));
    }

    @Test
    public void testGetSelectedText_wrongSelection() {
        final BaseInputConnection connection = createBaseInputConnection();
        Editable editable = connection.getEditable();
        editable.append("hello");
        editable.setSpan(Selection.SELECTION_START, 4, 4, Spanned.SPAN_POINT_POINT);
        editable.removeSpan(Selection.SELECTION_END);

        // Should not crash.
        connection.getSelectedText(0);
    }
}
