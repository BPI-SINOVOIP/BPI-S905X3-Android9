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

package android.text.method.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.os.SystemClock;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.test.UiThreadTest;
import android.text.Editable;
import android.text.InputType;
import android.text.Selection;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.method.TextKeyListener;
import android.text.method.TextKeyListener.Capitalize;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.widget.TextView.BufferType;

import com.android.compatibility.common.util.CtsKeyEventUtil;

import org.junit.Test;
import org.junit.runner.RunWith;

@MediumTest
@RunWith(AndroidJUnit4.class)
public class TextKeyListenerTest extends KeyListenerTestCase {
    /**
     * time out of MultiTapKeyListener. longer than 2000ms in case the system is sluggish.
     */
    private static final long TIME_OUT = 3000;

    @Test
    public void testConstructor() {
        new TextKeyListener(Capitalize.NONE, true);

        new TextKeyListener(null, true);
    }

    @Test
    public void testShouldCap() {
        String str = "hello world! man";

        // Index of the characters(start with 0):
        // 'h' = 0; 'w' = 6; 'm' = 13; 'a' = 14
        assertFalse(TextKeyListener.shouldCap(Capitalize.NONE, str, 0));
        assertTrue(TextKeyListener.shouldCap(Capitalize.SENTENCES, str, 0));
        assertTrue(TextKeyListener.shouldCap(Capitalize.WORDS, str, 0));
        assertTrue(TextKeyListener.shouldCap(Capitalize.CHARACTERS, str, 0));

        assertFalse(TextKeyListener.shouldCap(Capitalize.NONE, str, 6));
        assertFalse(TextKeyListener.shouldCap(Capitalize.SENTENCES, str, 6));
        assertTrue(TextKeyListener.shouldCap(Capitalize.WORDS, str, 6));
        assertTrue(TextKeyListener.shouldCap(Capitalize.CHARACTERS, str, 6));

        assertFalse(TextKeyListener.shouldCap(Capitalize.NONE, str, 13));
        assertTrue(TextKeyListener.shouldCap(Capitalize.SENTENCES, str, 13));
        assertTrue(TextKeyListener.shouldCap(Capitalize.WORDS, str, 13));
        assertTrue(TextKeyListener.shouldCap(Capitalize.CHARACTERS, str, 13));

        assertFalse(TextKeyListener.shouldCap(Capitalize.NONE, str, 14));
        assertFalse(TextKeyListener.shouldCap(Capitalize.SENTENCES, str, 14));
        assertFalse(TextKeyListener.shouldCap(Capitalize.WORDS, str, 14));
        assertTrue(TextKeyListener.shouldCap(Capitalize.CHARACTERS, str, 14));
    }

    @Test(expected=NullPointerException.class)
    public void testShouldCapNull() {
        TextKeyListener.shouldCap(Capitalize.WORDS, null, 16);
    }

    @Test
    public void testOnSpanAdded() throws Throwable {
        final TextKeyListener mockTextKeyListener = spy(
                new TextKeyListener(Capitalize.CHARACTERS, true));
        final Spannable text = new SpannableStringBuilder("123456");

        verify(mockTextKeyListener, never()).onSpanAdded(any(), any(), anyInt(), anyInt());
        mActivityRule.runOnUiThread(() -> {
            mTextView.setKeyListener(mockTextKeyListener);
            mTextView.setText(text, BufferType.EDITABLE);
        });
        mInstrumentation.waitForIdleSync();
        verify(mockTextKeyListener, atLeastOnce()).onSpanAdded(any(), any(), anyInt(), anyInt());

        mockTextKeyListener.release();
    }

    @Test
    public void testGetInstance1() {
        TextKeyListener listener1 = TextKeyListener.getInstance(true, Capitalize.WORDS);
        TextKeyListener listener2 = TextKeyListener.getInstance(true, Capitalize.WORDS);
        TextKeyListener listener3 = TextKeyListener.getInstance(false, Capitalize.WORDS);
        TextKeyListener listener4 = TextKeyListener.getInstance(true, Capitalize.CHARACTERS);

        assertNotNull(listener1);
        assertNotNull(listener2);
        assertSame(listener1, listener2);

        assertNotSame(listener1, listener3);
        assertNotSame(listener1, listener4);
        assertNotSame(listener4, listener3);

        listener1.release();
        listener2.release();
        listener3.release();
        listener4.release();
    }

    @Test
    public void testGetInstance2() {
        TextKeyListener listener1 = TextKeyListener.getInstance();
        TextKeyListener listener2 = TextKeyListener.getInstance();

        assertNotNull(listener1);
        assertNotNull(listener2);
        assertSame(listener1, listener2);

        listener1.release();
        listener2.release();
    }

    @Test
    public void testOnSpanChanged() {
        TextKeyListener textKeyListener = TextKeyListener.getInstance();
        final Spannable text = new SpannableStringBuilder("123456");
        textKeyListener.onSpanChanged(text, Selection.SELECTION_END, 0, 0, 0, 0);

        textKeyListener.release();
    }

    @Test(expected=NullPointerException.class)
    public void testOnSpanChangedNull() {
        TextKeyListener textKeyListener = TextKeyListener.getInstance();
        textKeyListener.onSpanChanged(null, Selection.SELECTION_END, 0, 0, 0, 0);
    }

    @UiThreadTest
    @Test
    public void testClear() {
        CharSequence text = "123456";
        mTextView.setText(text, BufferType.EDITABLE);

        Editable content = mTextView.getText();
        assertEquals(text, content.toString());

        TextKeyListener.clear(content);
        assertEquals("", content.toString());
    }

    @Test
    public void testOnSpanRemoved() {
        TextKeyListener textKeyListener = new TextKeyListener(Capitalize.CHARACTERS, true);
        final Spannable text = new SpannableStringBuilder("123456");
        textKeyListener.onSpanRemoved(text, new Object(), 0, 0);

        textKeyListener.release();
    }

    /**
     * Wait for TIME_OUT, or listener will accept key event as multi tap rather than a new key.
     */
    private void waitForListenerTimeout() {
        try {
            Thread.sleep(TIME_OUT);
        } catch (InterruptedException e) {
            fail("thrown unexpected InterruptedException when sleep.");
        }
    }

    /**
     * Check point:
     * 1. press KEYCODE_4 once. if it's ALPHA key board, text will be "4", if it's
     *    NUMERIC key board, text will be "g", else text will be "".
     */
    @Test
    public void testPressKey() throws Throwable {
        final TextKeyListener textKeyListener
                = TextKeyListener.getInstance(false, Capitalize.NONE);

        mActivityRule.runOnUiThread(() -> {
            mTextView.setText("", BufferType.EDITABLE);
            Selection.setSelection(mTextView.getText(), 0, 0);
            mTextView.setKeyListener(textKeyListener);
            mTextView.requestFocus();
        });
        mInstrumentation.waitForIdleSync();
        assertEquals("", mTextView.getText().toString());

        CtsKeyEventUtil.sendKeys(mInstrumentation, mTextView, KeyEvent.KEYCODE_4);
        waitForListenerTimeout();
        String text = mTextView.getText().toString();
        int keyType = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD).getKeyboardType();
        if (KeyCharacterMap.ALPHA == keyType
                || KeyCharacterMap.FULL == keyType) {
            assertEquals("4", text);
        } else if (KeyCharacterMap.NUMERIC == keyType) {
            assertEquals("g", text);
        } else {
            assertEquals("", text);
        }

        textKeyListener.release();
    }

    @Test
    public void testOnKeyOther() throws Throwable {
        final String text = "abcd";
        final TextKeyListener textKeyListener
                = TextKeyListener.getInstance(false, Capitalize.NONE);

        mActivityRule.runOnUiThread(() -> {
            mTextView.setText("", BufferType.EDITABLE);
            Selection.setSelection(mTextView.getText(), 0, 0);
            mTextView.setKeyListener(textKeyListener);
        });
        mInstrumentation.waitForIdleSync();
        assertEquals("", mTextView.getText().toString());

        // test ACTION_MULTIPLE KEYCODE_UNKNOWN key event.
        KeyEvent event = new KeyEvent(SystemClock.uptimeMillis(), text,
                1, KeyEvent.FLAG_WOKE_HERE);
        CtsKeyEventUtil.sendKey(mInstrumentation, mTextView, event);
        mInstrumentation.waitForIdleSync();
        // the text of TextView is never changed, onKeyOther never works.
//        assertEquals(text, mTextView.getText().toString()); issue 1731439

        textKeyListener.release();
    }

    @Test
    public void testGetInputType() {
        TextKeyListener listener = TextKeyListener.getInstance(false, Capitalize.NONE);
        int expected = InputType.TYPE_CLASS_TEXT;
        assertEquals(expected, listener.getInputType());

        listener = TextKeyListener.getInstance(false, Capitalize.CHARACTERS);
        expected = InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS;
        assertEquals(expected, listener.getInputType());

        listener.release();
    }
}
