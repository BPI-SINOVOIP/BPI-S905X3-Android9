/*
 * Copyright (C) 2018 The Android Open Source Project
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

package android.widget.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.content.Context;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;
import android.view.LayoutInflater;
import android.view.ViewGroup;
import android.widget.TextView;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * Test for font weight in TextView
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TextViewFontWeightTest {
    private TextView getTextView(int id) {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final LayoutInflater inflater = LayoutInflater.from(context);
        final ViewGroup container =
                (ViewGroup) inflater.inflate(R.layout.textview_weight_test_layout, null);
        return (TextView) container.findViewById(id);
    }

    private static class FontStyle {
        FontStyle(int weight, boolean italic) {
            mWeight = weight;
            mItalic = italic;
        }

        public int getWeight() {
            return mWeight;
        }

        public boolean isItalic() {
            return mItalic;
        }

        private int mWeight;
        private boolean mItalic;
    };

    private static final Map<Character, FontStyle> CHAR_FONT_MAP;
    static {
        // This mapping needs to be synced with res/font/fullfamily.xml
        final HashMap<Character, FontStyle> map = new HashMap<>();
        map.put('a', new FontStyle(100, false));
        map.put('b', new FontStyle(100, true));
        map.put('c', new FontStyle(200, false));
        map.put('d', new FontStyle(200, true));
        map.put('e', new FontStyle(300, false));
        map.put('f', new FontStyle(300, true));
        map.put('g', new FontStyle(400, false));
        map.put('h', new FontStyle(400, true));
        map.put('i', new FontStyle(500, false));
        map.put('j', new FontStyle(500, true));
        map.put('k', new FontStyle(600, false));
        map.put('l', new FontStyle(600, true));
        map.put('m', new FontStyle(700, false));
        map.put('n', new FontStyle(700, true));
        map.put('o', new FontStyle(800, false));
        map.put('p', new FontStyle(800, true));
        map.put('q', new FontStyle(900, false));
        map.put('r', new FontStyle(900, true));
        CHAR_FONT_MAP = Collections.unmodifiableMap(map);
    }

    private static void assertFontSelected(TextView tv, FontStyle style) {
        // In this tests, the following special font is used for testing typeface.
        // All fonts support 'a' to 'z' characters and all character has 1em advance except for one
        // character. For example, 'ascii_a3em_weight100_upright.ttf' supports 'a' to 'z' characters
        // and 'a' has 3em advance and others has 1em advance. Also, the metadata has width=100 and
        // slant=upright information.
        Typeface typeface = tv.getTypeface();
        assertNotNull(typeface);
        assertEquals(style.getWeight(), typeface.getWeight());
        assertEquals(style.isItalic(), typeface.isItalic());

        // Check if the correct underlying font is selected.
        char threeEmChar = 0;
        for (char c = 'a'; c <= 'z'; c++) {
            Paint paint = new Paint();
            paint.setTextSize(10.0f);  // Make 1em=10px
            paint.setTypeface(typeface);

            final float width = paint.measureText(new char[] { c }, 0 /* index */, 1 /* count */);
            if (width == 30.0f) {
                if (threeEmChar != 0) {
                    throw new IllegalStateException(
                        "There are multiple 3em characters. Incorrect test set up?");
                }
                threeEmChar = c;
            }
        }

        FontStyle fontStyle = CHAR_FONT_MAP.get(threeEmChar);
        assertNotNull(fontStyle);
        assertEquals(style.getWeight(), fontStyle.getWeight());
        assertEquals(style.isItalic(), fontStyle.isItalic());
    }

    @Test
    public void testWeight() {
        assertFontSelected(getTextView(R.id.textView_weight100_upright), new FontStyle(100, false));
        assertFontSelected(getTextView(R.id.textView_weight100_italic), new FontStyle(100, true));
        assertFontSelected(getTextView(R.id.textView_weight200_upright), new FontStyle(200, false));
        assertFontSelected(getTextView(R.id.textView_weight200_italic), new FontStyle(200, true));
        assertFontSelected(getTextView(R.id.textView_weight300_upright), new FontStyle(300, false));
        assertFontSelected(getTextView(R.id.textView_weight300_italic), new FontStyle(300, true));
        assertFontSelected(getTextView(R.id.textView_weight400_upright), new FontStyle(400, false));
        assertFontSelected(getTextView(R.id.textView_weight400_italic), new FontStyle(400, true));
        assertFontSelected(getTextView(R.id.textView_weight500_upright), new FontStyle(500, false));
        assertFontSelected(getTextView(R.id.textView_weight500_italic), new FontStyle(500, true));
        assertFontSelected(getTextView(R.id.textView_weight600_upright), new FontStyle(600, false));
        assertFontSelected(getTextView(R.id.textView_weight600_italic), new FontStyle(600, true));
        assertFontSelected(getTextView(R.id.textView_weight700_upright), new FontStyle(700, false));
        assertFontSelected(getTextView(R.id.textView_weight700_italic), new FontStyle(700, true));
        assertFontSelected(getTextView(R.id.textView_weight800_upright), new FontStyle(800, false));
        assertFontSelected(getTextView(R.id.textView_weight800_italic), new FontStyle(800, true));
        assertFontSelected(getTextView(R.id.textView_weight900_upright), new FontStyle(900, false));
        assertFontSelected(getTextView(R.id.textView_weight900_italic), new FontStyle(900, true));
    }

    @Test
    public void testTextAppearance() {
        assertFontSelected(getTextView(R.id.textAppearance_weight100_upright),
                new FontStyle(100, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight100_italic),
                new FontStyle(100, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight200_upright),
                new FontStyle(200, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight200_italic),
                new FontStyle(200, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight300_upright),
                new FontStyle(300, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight300_italic),
                new FontStyle(300, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight400_upright),
                new FontStyle(400, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight400_italic),
                new FontStyle(400, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight500_upright),
                new FontStyle(500, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight500_italic),
                new FontStyle(500, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight600_upright),
                new FontStyle(600, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight600_italic),
                new FontStyle(600, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight700_upright),
                new FontStyle(700, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight700_italic),
                new FontStyle(700, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight800_upright),
                new FontStyle(800, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight800_italic),
                new FontStyle(800, true));
        assertFontSelected(getTextView(R.id.textAppearance_weight900_upright),
                new FontStyle(900, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight900_italic),
                new FontStyle(900, true));
    }

    @Test
    public void testStyle() {
        assertFontSelected(getTextView(R.id.textView_normal), new FontStyle(400, false));
        assertFontSelected(getTextView(R.id.textView_bold), new FontStyle(700, false));
        assertFontSelected(getTextView(R.id.textView_italic), new FontStyle(400, true));
        assertFontSelected(getTextView(R.id.textView_bold_italic), new FontStyle(700, true));
        assertFontSelected(getTextView(R.id.textAppearance_normal), new FontStyle(400, false));
        assertFontSelected(getTextView(R.id.textAppearance_bold), new FontStyle(700, false));
        assertFontSelected(getTextView(R.id.textAppearance_italic), new FontStyle(400, true));
        assertFontSelected(getTextView(R.id.textAppearance_bold_italic), new FontStyle(700, true));
    }

    @Test
    public void testWeightStyleResolve() {
        // If both weight and style=bold is specified, ignore the boldness and use weight.
        assertFontSelected(getTextView(R.id.textView_weight100_bold), new FontStyle(100, false));
        assertFontSelected(getTextView(R.id.textAppearance_weight100_bold),
                new FontStyle(100, false));
    }
}
