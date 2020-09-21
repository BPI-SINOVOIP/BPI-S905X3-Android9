/*
 * Copyright (C) 2010 The Android Open Source Project
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

import java.util.ArrayList;
import java.util.Comparator;
import java.util.ConcurrentModificationException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.concurrent.PriorityBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.ReentrantLock;

/**
 * A thread-safe class with {@link PriorityBlockingQueue}-like operations that can retrieve objects
 * that match a certain condition.
 * <p/>
 * Iteration is also thread-safe, but not consistent. A copy of the queue is made at iterator
 * creation time, and that copy is used as the iteration target. If queue is modified during
 * iteration, a {@link ConcurrentModificationException} will not be thrown, but the iterator
 * will also not reflect the modified contents.
 * <p/>
 * @see PriorityBlockingQueue
 */
public class ConditionPriorityBlockingQueue<T> implements Iterable<T> {

    /**
     * An interface for determining if elements match some sort of condition.
     *
     * @param <T>
     */
    public static interface IMatcher<T> {
        /**
         * Determine if given <var>element</var> meets required condition
         *
         * @param element the object to match
         * @return <code>true</code> if condition was met. <code>false</code> otherwise.
         */
        boolean matches(T element);
    }

    /**
     * A {@link com.android.tradefed.util.ConditionPriorityBlockingQueue.IMatcher}
     * that matches any object.
     *
     * @param <T>
     */
    public static class AlwaysMatch<T> implements IMatcher<T> {

        /**
         * {@inheritDoc}
         */
        @Override
        public boolean matches(T element) {
            return true;
        }
    }

    private static class ConditionMatcherPair<T> {
        private final IMatcher<T> mMatcher;
        private final Condition mCondition;

        ConditionMatcherPair(IMatcher<T> m, Condition c) {
            mMatcher = m;
            mCondition = c;
        }
    }

    /** the list of current objects */
    private final List<T> mList;

    /** the global lock */
    private final ReentrantLock mLock = new ReentrantLock(true);
    /**
     * List of {@link IMatcher}'s that are waiting for an object to be added to queue that meets
     * their criteria
     */
    private final List<ConditionMatcherPair<T>> mWaitingMatcherList;

    private final Comparator<T> mComparator;

    /**
     * Creates a {@link ConditionPriorityBlockingQueue}
     * <p/>
     * Elements will be prioritized in FIFO order.
     */
    public ConditionPriorityBlockingQueue() {
        this(null);
    }

    /**
     * Creates a {@link ConditionPriorityBlockingQueue}
     *
     * @param c the {@link Comparator} used to prioritize the queue.
     */
    public ConditionPriorityBlockingQueue(Comparator<T> c) {
        mComparator = c;
        mList = new LinkedList<T>();
        mWaitingMatcherList = new LinkedList<ConditionMatcherPair<T>>();
    }

    /**
     * Retrieves and removes the head of this queue.
     *
     * @return the head of this queue, or <code>null</code> if the queue is empty
     */
    public T poll() {
        return poll(new AlwaysMatch<T>());
    }

    /**
     * Retrieves and removes the minimum (as judged by the provided {@link Comparator} element T in
     * the queue where <var>matcher.matches(T)</var> is <code>true</code>.
     *
     * @param matcher the {@link IMatcher} to use to evaluate elements
     * @return the minimum matched element or <code>null</code> if there are no matching elements
     */
    public T poll(IMatcher<T> matcher) {
        mLock.lock();
        try {
            // reference to the current min object
            T minObject = null;
            ListIterator<T> iter = mList.listIterator();
            while (iter.hasNext()) {
                T obj = iter.next();
                if (matcher.matches(obj) && compareObjects(obj, minObject) < 0) {
                    minObject = obj;
                }
            }
            if (minObject != null) {
                mList.remove(minObject);
            }
            return minObject;
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Retrieves and removes the minimum (as judged by the provided {@link Comparator} element T in
     * the queue.
     * <p/>
     * Blocks up to <var>timeout</var> time for an element to become available.
     *
     * @param timeout the amount of time to wait for an element to become available
     * @param unit the {@link TimeUnit} of timeout
     * @return the minimum matched element or <code>null</code> if there are no matching elements
     */
    public T poll(long timeout, TimeUnit unit) throws InterruptedException {
        return poll(timeout, unit, new AlwaysMatch<T>());
    }

    /**
     * Retrieves and removes the minimum (as judged by the provided {@link Comparator} element T in
     * the queue where <var>matcher.matches(T)</var> is <code>true</code>.
     * <p/>
     * Blocks up to <var>timeout</var> time for an element to become available.
     *
     * @param timeout the amount of time to wait for an element to become available
     * @param unit the {@link TimeUnit} of timeout
     * @param matcher the {@link IMatcher} to use to evaluate elements
     * @return the minimum matched element or <code>null</code> if there are no matching elements
     */
    public T poll(long timeout, TimeUnit unit, IMatcher<T> matcher) throws InterruptedException {
        Long nanos = unit.toNanos(timeout);
        return blockingPoll(nanos, matcher);
    }

    /**
     * Retrieves and removes the minimum (as judged by the provided {@link Comparator} element T in
     * the queue where <var>matcher.matches(T)</var> is <code>true</code>.
     * <p/>
     * Blocks up to <var>nanos</var> ns time for an element to become available. If <var>nanos</var>
     * is <code>null</code> will block indefinitely.
     *
     * @param nanos the amount of time in ns to wait for an element to become available. If
     *            <code>null</code> will wait indefinitely
     * @param matcher the {@link IMatcher} to use to evaluate elements
     * @return the minimum matched element or <code>null</code> if there are no matching elements
     * @throws InterruptedException
     */
    private T blockingPoll(Long nanos, IMatcher<T> matcher) throws InterruptedException {
        mLock.lockInterruptibly();
        try {
            T matchedObj = null;
            Condition myCondition = mLock.newCondition();
            ConditionMatcherPair<T> myMatcherPair = new ConditionMatcherPair<T>(matcher,
                    myCondition);
            mWaitingMatcherList.add(myMatcherPair);
            try {
                while ((matchedObj = poll(matcher)) == null && (nanos == null || nanos > 0)) {
                    if (nanos != null) {
                        nanos = myCondition.awaitNanos(nanos);
                    } else {
                        myCondition.await();
                    }
                }
            } catch (InterruptedException ie) {
                // TODO: do we need to propagate to non-interrupted thread?
                throw ie;
            } finally {
                mWaitingMatcherList.remove(myMatcherPair);
            }

            return matchedObj;
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Compare given <var>object</var> against given <var>minObject</var> using this class'
     * {@link Comparator}.
     *
     * @param object the object to compare
     * @param minObject the current minimum object to use as basis for comparison
     * @return -1 if <var>object</var> is less than <var>minObject</var> or <var>minObject</var> is
     *         null.<br/>
     *         0 if the two objects are equal.<br/>
     *         1 if <var>object</var> is greater than <var>minObject</var>. Note that 1 will always
     *         be returned if FIFO prioritization is used (ie the comparator is null) and
     *         <var>minObject</var> is not null, because <var>minObject</var> represents an
     *         <var>object</var> that is earlier in the queue.
     */
    private int compareObjects(T object, T minObject) {
        if (minObject == null) {
            return -1;
        } else if (mComparator == null) {
            return 1;
        } else {
            return mComparator.compare(object, minObject);
        }
    }

    /**
     * Retrieves and removes the minimum (as judged by the provided {@link Comparator} element T in
     * the queue.
     * <p/>
     * Blocks indefinitely for an element to become available.
     *
     * @return the head of this queue
     * @throws InterruptedException if interrupted while waiting
     */
    public T take() throws InterruptedException {
        return take(new AlwaysMatch<T>());
    }

    /**
     * Retrieves and removes the first element T in the queue where <var>matcher.matches(T)</var> is
     * <code>true</code>, waiting if necessary until such an element becomes available.
     *
     * @param matcher the {@link IMatcher} to use to evaluate elements
     * @return the matched element
     * @throws InterruptedException if interrupted while waiting
     */
    public T take(IMatcher<T> matcher) throws InterruptedException {
        return blockingPoll(null, matcher);
    }

    /**
     * Inserts the specified element into this queue. As the queue is unbounded this method will
     * never block.
     *
     * @param addedElement the element to add
     * @return <code>true</code>
     * @throws ClassCastException if the specified element cannot be compared with elements
     *             currently in the priority queue according to the priority queue's ordering
     * @throws NullPointerException if the specified element is null
     */
    public boolean add(T addedElement) {
        mLock.lock();
        try {
            boolean ok = mList.add(addedElement);
            assert ok;

            for (ConditionMatcherPair<T> matcherPair : mWaitingMatcherList) {
                if (matcherPair.mMatcher.matches(addedElement)) {
                    matcherPair.mCondition.signal();
                    break;
                }
            }
            return true;
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Removes all elements from this queue.
     */
    public void clear() {
        mLock.lock();
        try {
            mList.clear();
        } finally {
            mLock.unlock();
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public Iterator<T> iterator() {
        return getCopy().iterator();
    }

    /**
     * Get a copy of the contents of the queue.
     */
    public List<T> getCopy() {
        mLock.lock();
        try {
            List<T> l = new ArrayList<T>(size());
            l.addAll(mList);
            return l;
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Determine if an object is currently contained in this queue.
     *
     * @param object the object to find
     * @return <code>true</code> if given object is contained in queue. <code>false></code>
     *         otherwise.
     */
    public boolean contains(T object) {
        mLock.lock();
        try {
            return mList.contains(object);
        } finally {
            mLock.unlock();
        }
    }

    /**
     * @return the number of elements in queue
     */
    public int size() {
        mLock.lock();
        try {
            return mList.size();
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Removes an item from this queue.
     *
     * @param object the object to remove
     * @return <code>true</code> if given object was removed from queue. <code>false></code>
     *         otherwise.
     */
    public boolean remove(T object) {
        mLock.lock();
        try {
            return mList.remove(object);
        } finally {
            mLock.unlock();
        }
    }

    /**
     * Adds a item to this queue, replacing any existing object that matches given condition
     *
     * @param matcher the matcher to evaluate existing objects
     * @param object the object to add
     * @return the replaced object or <code>null</code> if none exist
     */
    public T addUnique(IMatcher<T> matcher, T object) {
        mLock.lock();
        try {
            T removedObj = poll(matcher);
            add(object);
            return removedObj;
        } finally {
            mLock.unlock();
        }
    }
}
