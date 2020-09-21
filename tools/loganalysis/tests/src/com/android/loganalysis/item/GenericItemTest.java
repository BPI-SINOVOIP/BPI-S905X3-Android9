/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.loganalysis.item;

import junit.framework.TestCase;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.Date;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * Unit test for {@link GenericItem}.
 */
public class GenericItemTest extends TestCase {
    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            "integer", "string"));

    private String mStringAttribute = "String";
    private Integer mIntegerAttribute = 1;

    /** Empty item with no attributes set */
    private GenericItem mEmptyItem1;
    /** Empty item with no attributes set */
    private GenericItem mEmptyItem2;
    /** Item with only the string attribute set */
    private GenericItem mStringItem;
    /** Item with only the integer attribute set */
    private GenericItem mIntegerItem;
    /** Item with both attributes set, product of mStringItem and mIntegerItem */
    private GenericItem mFullItem1;
    /** Item with both attributes set, product of mStringItem and mIntegerItem */
    private GenericItem mFullItem2;
    /** Item that is inconsistent with the others */
    private GenericItem mInconsistentItem;

    @Override
    public void setUp() {
        mEmptyItem1 = new GenericItem(ATTRIBUTES);
        mEmptyItem2 = new GenericItem(ATTRIBUTES);
        mStringItem = new GenericItem(ATTRIBUTES);
        mStringItem.setAttribute("string", mStringAttribute);
        mIntegerItem = new GenericItem(ATTRIBUTES);
        mIntegerItem.setAttribute("integer", mIntegerAttribute);
        mFullItem1 = new GenericItem(ATTRIBUTES);
        mFullItem1.setAttribute("string", mStringAttribute);
        mFullItem1.setAttribute("integer", mIntegerAttribute);
        mFullItem2 = new GenericItem(ATTRIBUTES);
        mFullItem2.setAttribute("string", mStringAttribute);
        mFullItem2.setAttribute("integer", mIntegerAttribute);
        mInconsistentItem = new GenericItem(ATTRIBUTES);
        mInconsistentItem.setAttribute("string", "gnirts");
        mInconsistentItem.setAttribute("integer", 2);
    }

    /**
     * Test for {@link GenericItem#mergeAttributes(IItem, Set)}.
     */
    public void testMergeAttributes() throws ConflictingItemException {
        Map<String, Object> attributes;

        attributes = mEmptyItem1.mergeAttributes(mEmptyItem1, ATTRIBUTES);
        assertNull(attributes.get("string"));
        assertNull(attributes.get("integer"));

        attributes = mEmptyItem1.mergeAttributes(mEmptyItem2, ATTRIBUTES);
        assertNull(attributes.get("string"));
        assertNull(attributes.get("integer"));

        attributes = mEmptyItem2.mergeAttributes(mEmptyItem1, ATTRIBUTES);
        assertNull(attributes.get("string"));
        assertNull(attributes.get("integer"));

        attributes = mEmptyItem1.mergeAttributes(mStringItem, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertNull(attributes.get("integer"));

        attributes = mStringItem.mergeAttributes(mEmptyItem1, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertNull(attributes.get("integer"));

        attributes = mIntegerItem.mergeAttributes(mStringItem, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertEquals(mIntegerAttribute, attributes.get("integer"));

        attributes = mEmptyItem1.mergeAttributes(mFullItem1, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertEquals(mIntegerAttribute, attributes.get("integer"));

        attributes = mFullItem1.mergeAttributes(mEmptyItem1, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertEquals(mIntegerAttribute, attributes.get("integer"));

        attributes = mFullItem1.mergeAttributes(mFullItem2, ATTRIBUTES);
        assertEquals(mStringAttribute, attributes.get("string"));
        assertEquals(mIntegerAttribute, attributes.get("integer"));

        try {
            mFullItem1.mergeAttributes(mInconsistentItem, ATTRIBUTES);
            fail("Expecting a ConflictingItemException");
        } catch (ConflictingItemException e) {
            // Expected
        }
    }

    /**
     * Test for {@link GenericItem#isConsistent(IItem)}.
     */
    public void testIsConsistent() {
        assertTrue(mEmptyItem1.isConsistent(mEmptyItem1));
        assertFalse(mEmptyItem1.isConsistent(null));
        assertTrue(mEmptyItem1.isConsistent(mEmptyItem2));
        assertTrue(mEmptyItem2.isConsistent(mEmptyItem1));
        assertTrue(mEmptyItem1.isConsistent(mStringItem));
        assertTrue(mStringItem.isConsistent(mEmptyItem1));
        assertTrue(mIntegerItem.isConsistent(mStringItem));
        assertTrue(mEmptyItem1.isConsistent(mFullItem1));
        assertTrue(mFullItem1.isConsistent(mEmptyItem1));
        assertTrue(mFullItem1.isConsistent(mFullItem2));
        assertFalse(mFullItem1.isConsistent(mInconsistentItem));
    }

    /** Test {@link GenericItem#equals(Object)}. */
    @SuppressWarnings("SelfEquals")
    public void testEquals() {
        assertTrue(mEmptyItem1.equals(mEmptyItem1));
        assertFalse(mEmptyItem1.equals(null));
        assertTrue(mEmptyItem1.equals(mEmptyItem2));
        assertTrue(mEmptyItem2.equals(mEmptyItem1));
        assertFalse(mEmptyItem1.equals(mStringItem));
        assertFalse(mStringItem.equals(mEmptyItem1));
        assertFalse(mIntegerItem.equals(mStringItem));
        assertFalse(mEmptyItem1.equals(mFullItem1));
        assertFalse(mFullItem1.equals(mEmptyItem1));
        assertTrue(mFullItem1.equals(mFullItem2));
        assertFalse(mFullItem1.equals(mInconsistentItem));
    }

    /**
     * Test for {@link GenericItem#setAttribute(String, Object)} and
     * {@link GenericItem#getAttribute(String)}.
     */
    public void testAttributes() {
        GenericItem item = new GenericItem(ATTRIBUTES);

        assertNull(item.getAttribute("string"));
        assertNull(item.getAttribute("integer"));

        item.setAttribute("string", mStringAttribute);
        item.setAttribute("integer", mIntegerAttribute);

        assertEquals(mStringAttribute, item.getAttribute("string"));
        assertEquals(mIntegerAttribute, item.getAttribute("integer"));

        item.setAttribute("string", null);
        item.setAttribute("integer", null);

        assertNull(item.getAttribute("string"));
        assertNull(item.getAttribute("integer"));

        try {
            item.setAttribute("object", new Object());
            fail("Failed to throw IllegalArgumentException");
        } catch (IllegalArgumentException e) {
            // Expected because "object" is not "string" or "integer".
        }
    }

    /**
     * Test for {@link GenericItem#areEqual(Object, Object)}
     */
    public void testAreEqual() {
        assertTrue(GenericItem.areEqual(null, null));
        assertTrue(GenericItem.areEqual("test", "test"));
        assertFalse(GenericItem.areEqual(null, "test"));
        assertFalse(GenericItem.areEqual("test", null));
        assertFalse(GenericItem.areEqual("test", ""));
    }

    /**
     * Test for {@link GenericItem#areConsistent(Object, Object)}
     */
    public void testAreConsistent() {
        assertTrue(GenericItem.areConsistent(null, null));
        assertTrue(GenericItem.areConsistent("test", "test"));
        assertTrue(GenericItem.areConsistent(null, "test"));
        assertTrue(GenericItem.areConsistent("test", null));
        assertFalse(GenericItem.areConsistent("test", ""));
    }

    /**
     * Test for {@link GenericItem#mergeObjects(Object, Object)}
     */
    public void testMergeObjects() throws ConflictingItemException {
        assertNull(GenericItem.mergeObjects(null, null));
        assertEquals("test", GenericItem.mergeObjects("test", "test"));
        assertEquals("test", GenericItem.mergeObjects(null, "test"));
        assertEquals("test", GenericItem.mergeObjects("test", null));

        try {
            assertEquals("test", GenericItem.mergeObjects("test", ""));
            fail("Expected ConflictingItemException to be thrown");
        } catch (ConflictingItemException e) {
            // Expected because "test" conflicts with "".
        }
    }

    /**
     * Test that {@link GenericItem#toJson()} returns correctly.
     */
    public void testToJson() throws JSONException {
        GenericItem item = new GenericItem(new HashSet<String>(Arrays.asList(
                "string", "date", "object", "integer", "long", "float", "double", "item", "null")));
        Date date = new Date();
        Object object = new Object();
        NativeCrashItem subItem = new NativeCrashItem();

        item.setAttribute("string", "foo");
        item.setAttribute("date", date);
        item.setAttribute("object", object);
        item.setAttribute("integer", 0);
        item.setAttribute("long", 1L);
        item.setAttribute("float", 2.5f);
        item.setAttribute("double", 3.5);
        item.setAttribute("item", subItem);
        item.setAttribute("null", null);

        // Convert to JSON string and back again
        JSONObject output = new JSONObject(item.toJson().toString());

        assertTrue(output.has("string"));
        assertEquals("foo", output.get("string"));
        assertTrue(output.has("date"));
        assertEquals(date.toString(), output.get("date"));
        assertTrue(output.has("object"));
        assertEquals(object.toString(), output.get("object"));
        assertTrue(output.has("integer"));
        assertEquals(0, output.get("integer"));
        assertTrue(output.has("long"));
        assertEquals(1, output.get("long"));
        assertTrue(output.has("float"));
        assertEquals(2.5, output.get("float"));
        assertTrue(output.has("double"));
        assertEquals(3.5, output.get("double"));
        assertTrue(output.has("item"));
        assertTrue(output.get("item") instanceof JSONObject);
        assertFalse(output.has("null"));
    }
}
