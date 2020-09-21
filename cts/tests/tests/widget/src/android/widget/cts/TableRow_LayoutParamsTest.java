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

package android.widget.cts;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.support.test.annotation.UiThreadTest;
import android.support.test.filters.SmallTest;
import android.support.test.rule.ActivityTestRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.AttributeSet;
import android.util.Xml;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewGroup.MarginLayoutParams;
import android.widget.TableLayout;
import android.widget.TableRow;
import android.widget.cts.util.XmlUtils;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.xmlpull.v1.XmlPullParser;

/**
 * Test {@link TableRow.LayoutParams}.
 */
@SmallTest
@RunWith(AndroidJUnit4.class)
public class TableRow_LayoutParamsTest {
    private Activity mActivity;

    @Rule
    public ActivityTestRule<TableCtsActivity> mActivityRule =
            new ActivityTestRule<>(TableCtsActivity.class);

    @Before
    public void setup() {
        mActivity = mActivityRule.getActivity();
    }

    @UiThreadTest
    @Test
    public void testConstructor() {
        new TableRow.LayoutParams(mActivity, null);

        TableRow.LayoutParams layoutParams = new TableRow.LayoutParams(200, 300);
        assertEquals(200, layoutParams.width);
        assertEquals(300, layoutParams.height);
        assertEquals(-1, layoutParams.column);
        assertEquals(1, layoutParams.span);
        ViewGroup.LayoutParams oldParams = layoutParams;

        layoutParams = new TableRow.LayoutParams(200, 300, 1.2f);
        assertEquals(200, layoutParams.width);
        assertEquals(300, layoutParams.height);
        assertEquals(1.2f, layoutParams.weight, 0.0f);
        assertEquals(-1, layoutParams.column);
        assertEquals(1, layoutParams.span);
        MarginLayoutParams oldMarginParams = layoutParams;

        layoutParams = new TableRow.LayoutParams();
        assertEquals(-1, layoutParams.column);
        assertEquals(1, layoutParams.span);

        layoutParams = new TableRow.LayoutParams(5);
        assertEquals(5, layoutParams.column);
        assertEquals(1, layoutParams.span);

        layoutParams = new TableRow.LayoutParams(oldParams);
        assertEquals(200, layoutParams.width);
        assertEquals(300, layoutParams.height);
        assertEquals(0, layoutParams.column);
        assertEquals(0, layoutParams.span);

        layoutParams = new TableRow.LayoutParams(oldMarginParams);
        assertEquals(200, layoutParams.width);
        assertEquals(300, layoutParams.height);
        assertEquals(0, layoutParams.column);
        assertEquals(0, layoutParams.span);

        mActivity.setContentView(R.layout.table_layout_2);
        int idTable = R.id.table2;
        TableLayout tableLayout = (TableLayout) mActivity.findViewById(idTable);
        View vVirtual1 = ((TableRow) tableLayout.getChildAt(0)).getVirtualChildAt(1);
        layoutParams = (TableRow.LayoutParams) vVirtual1.getLayoutParams();
        assertEquals(1, layoutParams.column);
        View vVirtual2 = ((TableRow) tableLayout.getChildAt(0)).getVirtualChildAt(2);
        layoutParams = (TableRow.LayoutParams) vVirtual2.getLayoutParams();
        assertEquals(2, layoutParams.span);
    }

    /**
     * Test
     * {@link TableRow.LayoutParams#
     * setBaseAttributes(android.content.res.TypedArray, int, int)}
     * .
     */
    @Test
    public void testSetBaseAttributes() {
        MockTableRow_LayoutParams mockLayoutParams = new MockTableRow_LayoutParams(200, 300);
        assertEquals(200, mockLayoutParams.width);
        assertEquals(300, mockLayoutParams.height);

        // base_attr_pixel: layout_width = 400px, layout_height = 600px
        AttributeSet attrs = getAttrs("base_attr_pixel");
        TypedArray a = mActivity.obtainStyledAttributes(attrs,
                android.R.styleable.ViewGroup_Layout);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_width,
                android.R.styleable.ViewGroup_Layout_layout_height);
        assertEquals(400, mockLayoutParams.width);
        assertEquals(600, mockLayoutParams.height);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_height,
                android.R.styleable.ViewGroup_Layout_layout_width);
        assertEquals(600, mockLayoutParams.width);
        assertEquals(400, mockLayoutParams.height);

        a.recycle();
        // base_attr_fillwrap: layout_width = "match_parent", layout_height = "wrap_content"
        attrs = getAttrs("base_attr_fillwrap");
        a = mActivity.obtainStyledAttributes(attrs, android.R.styleable.ViewGroup_Layout);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_width,
                android.R.styleable.ViewGroup_Layout_layout_height);
        assertEquals(TableLayout.LayoutParams.MATCH_PARENT, mockLayoutParams.width);
        assertEquals(TableLayout.LayoutParams.WRAP_CONTENT, mockLayoutParams.height);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_height,
                android.R.styleable.ViewGroup_Layout_layout_width);
        assertEquals(TableLayout.LayoutParams.WRAP_CONTENT, mockLayoutParams.width);
        assertEquals(TableLayout.LayoutParams.MATCH_PARENT, mockLayoutParams.height);

        a.recycle();
        // base_attr_noheight: layout_width = 600px, no layout_height.
        attrs = getAttrs("base_attr_noheight");
        a = mActivity.obtainStyledAttributes(attrs, android.R.styleable.ViewGroup_Layout);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_width,
                android.R.styleable.ViewGroup_Layout_layout_height);
        assertEquals(600, mockLayoutParams.width);
        assertEquals(TableLayout.LayoutParams.WRAP_CONTENT, mockLayoutParams.height);

        mockLayoutParams.setBaseAttributes(a, android.R.styleable.ViewGroup_Layout_layout_height,
                android.R.styleable.ViewGroup_Layout_layout_width);
        assertEquals(TableLayout.LayoutParams.MATCH_PARENT, mockLayoutParams.width);
        assertEquals(600, mockLayoutParams.height);

        try {
            mockLayoutParams.setBaseAttributes(null,
                    android.R.styleable.ViewGroup_Layout_layout_width,
                    android.R.styleable.ViewGroup_Layout_layout_height);
            fail("Should throw NullPointerException");
        } catch (NullPointerException e) {
        }

        try {
            mockLayoutParams.setBaseAttributes(a, -1,
                    android.R.styleable.ViewGroup_Layout_layout_height);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (ArrayIndexOutOfBoundsException e) {
        }

        try {
            mockLayoutParams.setBaseAttributes(null,
                    android.R.styleable.ViewGroup_Layout_layout_width, -1);
            fail("Should throw ArrayIndexOutOfBoundsException");
        } catch (NullPointerException e) {
        }
    }

    private AttributeSet getAttrs(String searchedNodeName) {
        XmlResourceParser parser = null;
        AttributeSet attrs = null;
        try {
            parser = mActivity.getResources().getXml(R.xml.base_attributes);

            int type;
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && type != XmlPullParser.START_TAG) {
            }
            String nodeName = parser.getName();
            if (!"alias".equals(nodeName)) {
                throw new RuntimeException();
            }
            int outerDepth = parser.getDepth();
            while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                    && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                    continue;
                }
                nodeName = parser.getName();
                if (searchedNodeName.equals(nodeName)) {
                    outerDepth = parser.getDepth();
                    while ((type = parser.next()) != XmlPullParser.END_DOCUMENT
                            && (type != XmlPullParser.END_TAG || parser.getDepth() > outerDepth)) {
                        if (type == XmlPullParser.END_TAG || type == XmlPullParser.TEXT) {
                            continue;
                        }
                        nodeName = parser.getName();
                        if ("Attributes".equals(nodeName)) {
                            attrs = Xml.asAttributeSet(parser);
                            break;
                        } else {
                            XmlUtils.skipCurrentTag(parser);
                        }
                    }
                    break;
                } else {
                    XmlUtils.skipCurrentTag(parser);
                }
            }
        } catch (Exception e) {
        }
        return attrs;
    }

    /*
     * Mock class for TableRow.LayoutParams to test protected methods
     */
    private class MockTableRow_LayoutParams extends TableRow.LayoutParams {
        public MockTableRow_LayoutParams(int w, int h) {
            super(w, h);
        }

        @Override
        protected void setBaseAttributes(TypedArray a, int widthAttr,
                int heightAttr) {
            super.setBaseAttributes(a, widthAttr, heightAttr);
        }
    }
}
