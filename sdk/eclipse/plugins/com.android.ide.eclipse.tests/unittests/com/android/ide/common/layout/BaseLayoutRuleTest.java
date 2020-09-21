/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.common.layout;

import static com.android.SdkConstants.ANDROID_URI;
import static com.android.SdkConstants.ATTR_ID;

import com.android.ide.common.api.IDragElement;
import com.android.ide.common.api.INode;
import com.android.ide.common.api.Rect;
import com.android.ide.common.layout.BaseLayoutRule.AttributeFilter;
import com.android.utils.Pair;

import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

// TODO: Check assertions
// TODO: Check equals() but not == strings by using new String("") to prevent interning
// TODO: Rename BaseLayout to BaseLayoutRule, and tests too of course

public class BaseLayoutRuleTest extends LayoutTestBase {

    /** Provides test data used by other test cases */
    private IDragElement[] createSampleElements() {
        IDragElement[] elements = TestDragElement.create(TestDragElement.create(
                "android.widget.Button", new Rect(0, 0, 100, 80)).id("@+id/Button01"),
                TestDragElement.create("android.widget.LinearLayout", new Rect(0, 80, 100, 280))
                        .id("@+id/LinearLayout01").add(
                                TestDragElement.create("android.widget.Button",
                                        new Rect(0, 80, 100, 80)).id("@+id/Button011"),
                                TestDragElement.create("android.widget.Button",
                                        new Rect(0, 180, 100, 80)).id("@+id/Button012")),
                TestDragElement.create("android.widget.Button", new Rect(100, 0, 100, 80)).id(
                        "@+id/Button02"));
        return elements;
    }

    /** Test {@link BaseLayoutRule#collectIds}: Check that basic lookup of id works */
    public final void testCollectIds1() {
        IDragElement[] elements = TestDragElement.create(TestDragElement.create(
                "android.widget.Button", new Rect(0, 0, 100, 80)).id("@+id/Button01"));
        Map<String, Pair<String, String>> idMap = new HashMap<String, Pair<String, String>>();
        Map<String, Pair<String, String>> ids = new BaseLayoutRule().collectIds(idMap, elements);
        assertEquals(1, ids.size());
        assertEquals("@+id/Button01", ids.keySet().iterator().next());
    }

    /**
     * Test {@link BaseLayoutRule#collectIds}: Check that with the wrong URI we
     * don't pick up the ID
     */
    public final void testCollectIds2() {
        IDragElement[] elements = TestDragElement.create(TestDragElement.create(
                "android.widget.Button", new Rect(0, 0, 100, 80)).set("myuri", ATTR_ID,
                "@+id/Button01"));

        Map<String, Pair<String, String>> idMap = new HashMap<String, Pair<String, String>>();
        Map<String, Pair<String, String>> ids = new BaseLayoutRule().collectIds(idMap, elements);
        assertEquals(0, ids.size());
    }

    /**
     * Test {@link BaseLayoutRule#normalizeId(String)}
     */
    public final void testNormalizeId() {
        assertEquals("foo", new BaseLayoutRule().normalizeId("foo"));
        assertEquals("@+id/name", new BaseLayoutRule().normalizeId("@id/name"));
        assertEquals("@+id/name", new BaseLayoutRule().normalizeId("@+id/name"));
    }

    /**
     * Test {@link BaseLayoutRule#collectExistingIds}
     */
    public final void testCollectExistingIds1() {
        Set<String> existing = new HashSet<String>();
        INode node = TestNode.create("android.widget.Button").id("@+id/Button012").add(
                TestNode.create("android.widget.Button").id("@+id/Button2"));

        new BaseLayoutRule().collectExistingIds(node, existing);

        assertEquals(2, existing.size());
        assertContainsSame(Arrays.asList("@+id/Button2", "@+id/Button012"), existing);
    }

    /**
     * Test {@link BaseLayoutRule#collectIds}: Check that with multiple elements and
     * some children we still pick up all the right id's
     */
    public final void testCollectIds3() {
        Map<String, Pair<String, String>> idMap = new HashMap<String, Pair<String, String>>();

        IDragElement[] elements = createSampleElements();
        Map<String, Pair<String, String>> ids = new BaseLayoutRule().collectIds(idMap, elements);
        assertEquals(5, ids.size());
        assertContainsSame(Arrays.asList("@+id/Button01", "@+id/Button02", "@+id/Button011",
                "@+id/Button012", "@+id/LinearLayout01"), ids.keySet());

        // Make sure the Pair has the right stuff too;
        // (having the id again in the pair seems redundant; see if I really
        // need it in the implementation)
        assertEquals(Pair.of("@+id/LinearLayout01", "android.widget.LinearLayout"), ids
                .get("@+id/LinearLayout01"));
    }

    /**
     * Test {@link BaseLayoutRule#remapIds}: Ensure that it identifies a conflict
     */
    public final void testRemapIds1() {
        Map<String, Pair<String, String>> idMap = new HashMap<String, Pair<String, String>>();
        BaseLayoutRule baseLayout = new BaseLayoutRule();
        IDragElement[] elements = createSampleElements();
        baseLayout.collectIds(idMap, elements);
        INode node = TestNode.create("android.widget.Button").id("@+id/Button012").add(
                TestNode.create("android.widget.Button").id("@+id/Button2"));

        assertEquals(5, idMap.size());
        Map<String, Pair<String, String>> remapped = baseLayout.remapIds(node, idMap);
        // 4 original from the sample elements, plus overlap with one
        // (Button012) - one new
        // button added in
        assertEquals(6, remapped.size());

        // TODO: I'm a little confused about what exactly this method should do;
        // check with Raphael.
    }


    /**
     * Test {@link BaseLayoutRule#getDropIdMap}
     */
    public final void testGetDropIdMap() {
        BaseLayoutRule baseLayout = new BaseLayoutRule();
        IDragElement[] elements = createSampleElements();
        INode node = TestNode.create("android.widget.Button").id("@+id/Button012").add(
                TestNode.create("android.widget.Button").id("@+id/Button2"));

        Map<String, Pair<String, String>> idMap = baseLayout.getDropIdMap(node, elements, true);
        assertContainsSame(Arrays.asList("@+id/Button01", "@+id/Button012", "@+id/Button011",
                "@id/Button012", "@+id/Button02", "@+id/LinearLayout01"), idMap
                .keySet());

        // TODO: I'm a little confused about what exactly this method should do;
        // check with Raphael.
    }

    public final void testAddAttributes1() {
        BaseLayoutRule layout = new BaseLayoutRule();

        // First try with no filter
        IDragElement oldElement = TestDragElement.create("a.w.B").id("@+id/foo");
        INode newNode = TestNode.create("a.w.B").id("@+id/foo").set("u", "key", "value").set("u",
                "nothidden", "nothiddenvalue");
        ;
        AttributeFilter filter = null;
        // No references in this test case
        Map<String, Pair<String, String>> idMap = null;

        layout.addAttributes(newNode, oldElement, idMap, filter);
        assertEquals("value", newNode.getStringAttr("u", "key"));
        assertEquals("nothiddenvalue", newNode.getStringAttr("u", "nothidden"));
    }

    public final void testAddAttributes2() {
        // Test filtering
        BaseLayoutRule layout = new BaseLayoutRule();

        // First try with no filter
        IDragElement oldElement = TestDragElement.create("a.w.B").id("@+id/foo");
        INode newNode = TestNode.create("a.w.B").id("@+id/foo").set("u", "key", "value").set("u",
                "hidden", "hiddenvalue");
        AttributeFilter filter = new AttributeFilter() {

            @Override
            public String replace(String attributeUri, String attributeName,
                    String attributeValue) {
                if (attributeName.equals("hidden")) {
                    return null;
                }

                return attributeValue;
            }
        };
        // No references in this test case
        Map<String, Pair<String, String>> idMap = null;

        layout.addAttributes(newNode, oldElement, idMap, filter);
        assertEquals("value", newNode.getStringAttr("u", "key"));
    }

    public final void testFindNewId() {
        BaseLayoutRule baseLayout = new BaseLayoutRule();
        Set<String> existing = new HashSet<String>();
        assertEquals("@+id/Widget01", baseLayout.findNewId("a.w.Widget", existing));

        existing.add("@+id/Widget01");
        assertEquals("@+id/Widget02", baseLayout.findNewId("a.w.Widget", existing));

        existing.add("@+id/Widget02");
        assertEquals("@+id/Widget03", baseLayout.findNewId("a.w.Widget", existing));

        existing.remove("@+id/Widget02");
        assertEquals("@+id/Widget02", baseLayout.findNewId("a.w.Widget", existing));
    }

    public final void testDefaultAttributeFilter() {
        assertEquals("true", BaseLayoutRule.DEFAULT_ATTR_FILTER.replace("myuri", "layout_alignRight",
                "true"));
        assertEquals(null, BaseLayoutRule.DEFAULT_ATTR_FILTER.replace(ANDROID_URI,
                "layout_alignRight", "true"));
        assertEquals("true", BaseLayoutRule.DEFAULT_ATTR_FILTER.replace(ANDROID_URI,
                "myproperty", "true"));
    }

    public final void testAddInnerElements() {
        IDragElement oldElement = TestDragElement.create("root").add(
                TestDragElement.create("a.w.B").id("@+id/child1")
                        .set("uri", "childprop1", "value1"),
                TestDragElement.create("a.w.B").id("@+id/child2").set("uri", "childprop2a",
                        "value2a").set("uri", "childprop2b", "value2b"));
        INode newNode = TestNode.create("a.w.B").id("@+id/foo");
        Map<String, Pair<String, String>> idMap = new HashMap<String, Pair<String, String>>();
        BaseLayoutRule layout = new BaseLayoutRule();
        layout.addInnerElements(newNode, oldElement, idMap);
        assertEquals(2, newNode.getChildren().length);

        assertEquals("value2b", newNode.getChildren()[1].getStringAttr("uri", "childprop2b"));
    }
}
