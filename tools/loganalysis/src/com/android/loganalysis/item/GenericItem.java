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

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 * An implementation of the {@link IItem} interface which implements helper methods.
 */
public class GenericItem implements IItem {
    private Map<String, Object> mAttributes = new HashMap<String, Object>();
    private Set<String> mAllowedAttributes;

    protected GenericItem(Set<String> allowedAttributes) {
        mAllowedAttributes = new HashSet<String>();
        mAllowedAttributes.addAll(allowedAttributes);
    }

    protected GenericItem(Set<String> allowedAttributes, Map<String, Object> attributes) {
        this(allowedAttributes);

        for (Map.Entry<String, Object> entry : attributes.entrySet()) {
            setAttribute(entry.getKey(), entry.getValue());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IItem merge(IItem other) throws ConflictingItemException {
        if (this == other) {
            return this;
        }
        if (other == null || getClass() != other.getClass()) {
            throw new ConflictingItemException("Conflicting class types");
        }

        return new GenericItem(mAllowedAttributes, mergeAttributes(other, mAllowedAttributes));
    }

    /**
     * Merges the attributes from the item and another and returns a Map of the merged attributes.
     * <p>
     * Goes through each field in the item preferring non-null attributes over null attributes.
     * </p>
     *
     * @param other The other item
     * @return A Map from Strings to Objects containing merged attributes.
     * @throws ConflictingItemException If the two items are not consistent.
     */
    protected Map<String, Object> mergeAttributes(IItem other, Set<String> attributes)
            throws ConflictingItemException {
        if (this == other) {
            return mAttributes;
        }
        if (other == null || getClass() != other.getClass()) {
            throw new ConflictingItemException("Conflicting class types");
        }

        GenericItem item = (GenericItem) other;
        Map<String, Object> mergedAttributes = new HashMap<String, Object>();
        for (String attribute : attributes) {
            mergedAttributes.put(attribute,
                    mergeObjects(getAttribute(attribute), item.getAttribute(attribute)));
        }
        return mergedAttributes;
    }

    /** {@inheritDoc} */
    @Override
    public boolean isConsistent(IItem other) {
        if (this == other) {
            return true;
        }
        if (other == null || getClass() != other.getClass()) {
            return false;
        }

        GenericItem item = (GenericItem) other;
        for (String attribute : mAllowedAttributes) {
            if (!areConsistent(getAttribute(attribute), item.getAttribute(attribute))) {
                return false;
            }
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (other == null || getClass() != other.getClass()) {
            return false;
        }

        GenericItem item = (GenericItem) other;
        for (String attribute : mAllowedAttributes) {
            if (!areEqual(getAttribute(attribute), item.getAttribute(attribute))) {
                return false;
            }
        }
        return true;
    }

    /** {@inheritDoc} */
    @Override
    public int hashCode() {
        int result = 13;
        for (String attribute : mAllowedAttributes) {
            Object val = getAttribute(attribute);
            result += 37 * (val == null ? 0 : val.hashCode());
        }
        return result;
    }

    /**
     * {@inheritDoc}
     * <p>
     * This method will only return a JSON object with attributes that the JSON library knows how to
     * encode, along with attributes which implement the {@link IItem} interface.  If objects are
     * stored in this class that fall outside this category, they must be encoded outside of this
     * method.
     * </p>
     */
    @Override
    public JSONObject toJson() {
        JSONObject object = new JSONObject();
        for (Map.Entry<String, Object> entry : mAttributes.entrySet()) {
            final String key = entry.getKey();
            final Object attribute = entry.getValue();
            try {
                if (attribute != null && attribute instanceof IItem) {
                    object.put(key, ((IItem) attribute).toJson());
                } else {
                    object.put(key, attribute);
                }
            } catch (JSONException e) {
                // Ignore
            }
        }
        return object;
    }

    /**
     * Set an attribute to a value.
     *
     * @param attribute The name of the attribute.
     * @param value The value.
     * @throws IllegalArgumentException If the attribute is not in allowedAttributes.
     */
    protected void setAttribute(String attribute, Object value) throws IllegalArgumentException {
        if (!mAllowedAttributes.contains(attribute)) {
            throw new IllegalArgumentException();
        }
        mAttributes.put(attribute, value);
    }

    /**
     * Get the value of an attribute.
     *
     * @param attribute The name of the attribute.
     * @return The value or null if the attribute has not been set.
     * @throws IllegalArgumentException If the attribute is not in allowedAttributes.
     */
    protected Object getAttribute(String attribute) throws IllegalArgumentException {
        if (!mAllowedAttributes.contains(attribute)) {
            throw new IllegalArgumentException();
        }
        return mAttributes.get(attribute);
    }

    /**
     * Helper method to return if two objects are equal.
     *
     * @param object1 The first object
     * @param object2 The second object
     * @return True if object1 and object2 are both null or if object1 is equal to object2, false
     * otherwise.
     */
    static protected boolean areEqual(Object object1, Object object2) {
        return object1 == null ? object2 == null : object1.equals(object2);
    }

    /**
     * Helper method to return if two objects are consistent.
     *
     * @param object1 The first object
     * @param object2 The second object
     * @return True if either object1 or object2 is null or if object1 is equal to object2, false if
     * both objects are not null and not equal.
     */
    static protected boolean areConsistent(Object object1, Object object2) {
        return object1 == null || object2 == null ? true : object1.equals(object2);
    }

    /**
     * Helper method used for merging two objects.
     *
     * @param object1 The first object
     * @param object2 The second object
     * @return If both objects are null, then null, else the non-null item if both items are equal.
     * @throws ConflictingItemException If both objects are not null and they are not equal.
     */
    static protected Object mergeObjects(Object object1, Object object2)
            throws ConflictingItemException {
        if (!areConsistent(object1, object2)) {
            throw new ConflictingItemException(String.format("%s conflicts with %s", object1,
                    object2));
        }
        return object1 == null ? object2 : object1;
    }
}
