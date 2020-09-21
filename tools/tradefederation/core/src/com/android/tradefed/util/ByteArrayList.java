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

package com.android.tradefed.util;

import java.util.Arrays;

/**
 * A class to represent a lightweight byte array.  The goal of this class is to avoid the
 * significant space overhead of using Java classes.  See, for instance:
 * http://benjchristensen.com/2008/05/27/java-memory-usage-ints/
 */
public class ByteArrayList {
    private byte[] mStorage;
    /** arbitrarily choose 128 bytes as the default size */
    private int mMaxBytes = 128;
    private int mCurBytes = 0;
    private float mGrowthFactor = 2.0f;

    /**
     * Constructs an empty list with an initial capacity of 128 bytes and growth factor of 2.0
     */
    public ByteArrayList() {
        mStorage = new byte[mMaxBytes];
    }

    /**
     * Constructs an empty list with the specified initial capacity, and with a growth factor of 2.0
     *
     * @param defaultSize The initial capacity of the list, in bytes
     */
    public ByteArrayList(int defaultSize) {
        this();
        setSize(defaultSize);
    }

    /**
     * Constructs an empty list with the specified initial capacity and growth factor
     *
     * @param defaultSize The initial capacity of the list, in bytes
     * @param growthFactor The factor by which the capacity is multiplied when the list needs to
     *        auto-resize.  Must be {@code >= 1.1f}.
     */
    public ByteArrayList(int defaultSize, float growthFactor) {
        this(defaultSize);

        if (growthFactor < 1.1f) {
            throw new IllegalArgumentException("Growth factor must be at least 1.1");
        }
        mGrowthFactor = growthFactor;
    }

    // Methods involved in managing size
    /**
     * Trims the capacity of this {@code ByteArrayList} instance to be the list's current size.
     */
    public void trimToSize() {
        setSize(size());
    }

    /**
     * Increases the capacity of this {@code ByteArrayList} instance, if necessary, to ensure that
     * it can hold at least the number of bytes specified by the minimum capacity argument.
     *
     * @param minCapacity The minimum capacity to ensure storage for, in bytes
     */
    public void ensureCapacity(int minCapacity) {
        if (minCapacity < 0) {
            throw new IllegalArgumentException("Minimum capacity must be non-negative");
        } else if (minCapacity <= mMaxBytes) {
            return;
        }

        int curSize = mMaxBytes;
        if (curSize == 0) {
            curSize = 1;
        }
        /**
         * Need to grow the array
         * Want: size * (growthFactor * k) >= minCapacity
         *   So: k >= minCapacity / (size * growthFactor)
         *       k = ceil(minCapacity / size / growthFactor)
         */
        int growthFactorMultiples = (int)Math.ceil(minCapacity / mGrowthFactor / curSize);
        // newSize = oldSize * (growthFactor * k) >= minCapacity, from above
        float newSize = curSize * mGrowthFactor * growthFactorMultiples;
        setSize((int)Math.ceil(newSize));
    }

    /**
     * Sets the storage capacity of the internal storage array to the size specified, truncating the
     * internal byte array if needed.
     *
     * @param size The new capacity of the list, in bytes
     */
    void setSize(int size) {
        if (size < 0) {
            throw new IllegalArgumentException("New size must be non-negative");
        } else if (size < mCurBytes) {
            mCurBytes = size;
        }
        mMaxBytes = size;

        byte[] newStorage = new byte[size];
        System.arraycopy(mStorage, 0, newStorage, 0, mCurBytes);
        mStorage = newStorage;
    }

    // Extra methods specific to this class
    /**
     * Returns a copy of the contents of this {@code ByteArrayList} as a {@code byte[]}.
     *
     * @return A {@code byte[]} copy of the list contents
     */
    public byte[] getContents() {
        byte[] contents = new byte[mCurBytes];
        System.arraycopy(mStorage, 0, contents, 0, mCurBytes);
        return contents;
    }

    // Implement useful methods from the List interface
    /**
     * Appends the specified element to the end of this list
     *
     * @param b The {@code byte} to append to the list
     * @return {@code true}
     */
    public boolean add(byte b) {
        ensureCapacity(mCurBytes + 1);
        mStorage[mCurBytes] = b;
        mCurBytes++;
        return true;
    }

    /**
     * Appends the full contents of the supplied {@code byte[]} to the list.
     *
     * @param src The {@code byte[]} to append contents from
     * @return {@code true}
     */
    public boolean addAll(byte[] src) {
        if (src == null) {
            throw new NullPointerException();
        }

        return addAll(src, 0, src.length);
    }

    /**
     * Appends the specified contents of the supplied {@code byte[]} to the list.
     *
     * @param src The {@code byte[]} to append contents from
     * @param srcOffset The index of first element of {@code src} to append
     * @param length The quantity of bytes to append to the list
     * @return {@code true}
     */
    public boolean addAll(byte[] src, int srcOffset, int length) {
        if (src == null) {
            throw new NullPointerException();
        }

        ensureCapacity(mCurBytes + length);
        System.arraycopy(src, srcOffset, mStorage, mCurBytes, length);
        mCurBytes += length;
        return true;
    }

    /**
     * Appends the full contents of the supplied {@link ByteArrayList} to the list.
     *
     * @param src The {@link ByteArrayList} to append contents from
     * @return {@code true}
     */
    public boolean addall(ByteArrayList src) {
        if (src == null) {
            throw new NullPointerException();
        }

        return addAll(src.getContents());
    }

    /**
     * Removes all of the elements from this list.
     */
    public void clear() {
        mCurBytes = 0;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean equals(Object other) {
        if (!(other instanceof ByteArrayList)) {
            return false;
        }
        ByteArrayList otherList = (ByteArrayList) other;

        if (otherList.size() != size()) {
            return false;
        }
        for (int i = 0; i < size(); i++) {
            if (otherList.get(i) != get(i)) {
                return false;
            }
        }
        return true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public int hashCode() {
        return Arrays.hashCode(mStorage);
    }

    /**
     * Returns {@code true} if this list contains no bytes
     */
    public boolean isEmpty() {
        return size() == 0;
    }

    /**
     * Returns the element at the specified position in this list
     *
     * @param idx The index to return
     */
    public byte get(int idx) {
        if (idx < 0 || idx >= size()) {
            throw new IndexOutOfBoundsException();
        }
        return mStorage[idx];
    }

    /**
     * Replaces the element at the specified position in this list with the specified element
     *
     * @param idx The index to replace
     * @param b The {@code byte} to replace at that index
     */
    public byte set(int idx, byte b) {
        if (idx < 0 || idx >= size()) {
            throw new IndexOutOfBoundsException();
        }
        byte curVal = mStorage[idx];
        mStorage[idx] = b;
        return curVal;
    }

    /**
     * Returns the number of bytes in this list
     */
    public int size() {
        return mCurBytes;
    }

    // methods to facilitate testing
    int getMaxSize() {
        return mMaxBytes;
    }
}

