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

import com.android.tradefed.util.ConditionPriorityBlockingQueue.IMatcher;

import junit.framework.TestCase;

import java.util.Comparator;
import java.util.concurrent.TimeUnit;

/**
 * Unit tests for {@link ConditionPriorityBlockingQueue}.
 */
public class ConditionPriorityBlockingQueueTest extends TestCase {

    private ConditionPriorityBlockingQueue<Integer> mQueue;

    /**
     * {@inheritDoc}
     */
    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mQueue = new ConditionPriorityBlockingQueue<Integer>(new IntCompare());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll()} when queue is empty.
     */
    public void testPoll_empty() {
        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#take()} when a single object is in queue.
     */
    public void testTake() throws InterruptedException {
        Integer one = Integer.valueOf(1);
        mQueue.add(one);
        assertEquals(one, mQueue.take());
        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#take()} when multiple objects are in queue, and
     * verify objects are returned in expected order.
     */
    public void testTake_priority() throws InterruptedException {
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        mQueue.add(two);
        mQueue.add(one);
        assertEquals(one, mQueue.take());
        assertEquals(two, mQueue.take());
        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll()} when using FIFO ordering.
     */
    public void testTake_fifo() throws InterruptedException {
        ConditionPriorityBlockingQueue<Integer> fifoQueue =
            new ConditionPriorityBlockingQueue<Integer>();
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        fifoQueue.add(two);
        fifoQueue.add(one);
        assertEquals(two, fifoQueue.take());
        assertEquals(one, fifoQueue.take());
        assertNull(fifoQueue.poll());
    }

    /**
     * Same as {@link ConditionPriorityBlockingQueueTest#testTake_priority()}, but add the test
     * objects in inverse order.
     */
    public void testTake_priorityReverse() throws InterruptedException {
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        mQueue.add(one);
        mQueue.add(two);
        assertEquals(one, mQueue.take());
        assertEquals(two, mQueue.take());
        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#take()} when object is not initially present.
     */
    public void testTake_delayedAdd() throws InterruptedException {
        final Integer one = Integer.valueOf(1);
        Thread delayedAdd = new Thread() {
            @Override
            public void run() {
                try {
                    sleep(200);
                } catch (InterruptedException e) {
                }
                mQueue.add(one);
            }
        };
        delayedAdd.setName(getClass().getCanonicalName());
        delayedAdd.start();
        assertEquals(one, mQueue.take());
        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#take(IMatcher)} when object that matches is not
     * initially present.
     */
    public void testTake_matcher_delayedAdd() throws InterruptedException {
        final Integer one = Integer.valueOf(1);
        final Integer two = Integer.valueOf(2);
        mQueue.add(two);
        Thread delayedAdd = new Thread() {
            @Override
            public void run() {
                try {
                    sleep(200);
                } catch (InterruptedException e) {
                }
                mQueue.add(one);
            }
        };
        delayedAdd.setName(getClass().getCanonicalName());
        delayedAdd.start();
        assertEquals(one, mQueue.take(new OneMatcher()));
        assertNull(mQueue.poll(new OneMatcher()));
        assertEquals(two, mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#take(IMatcher)} when multiple threads are waiting
     */
    public void testTake_multiple_matchers() throws InterruptedException {
        final Integer one = Integer.valueOf(1);
        final Integer second_one = Integer.valueOf(1);

        Thread waiter = new Thread() {
            @Override
            public void run() {
                try {
                    mQueue.take(new OneMatcher());
                } catch (InterruptedException e) {

                }
            }
        };
        waiter.setName(getClass().getCanonicalName() + "#testTake_multiple_matchers");
        waiter.start();
        Thread waiter2 = new Thread() {
            @Override
            public void run() {
                try {
                    mQueue.take(new OneMatcher());
                } catch (InterruptedException e) {
                }
            }
        };
        waiter2.setName(getClass().getCanonicalName() + "#testTake_multiple_matchers");
        waiter2.start();

        Thread delayedAdd = new Thread() {
            @Override
            public void run() {
                try {
                    sleep(200);
                } catch (InterruptedException e) {
                }
                mQueue.add(one);
            }
        };
        delayedAdd.setName(getClass().getCanonicalName() + "#testTake_multiple_matchers");
        delayedAdd.start();
        Thread delayedAdd2 = new Thread() {
            @Override
            public void run() {
                try {
                    sleep(300);
                } catch (InterruptedException e) {
                }
                mQueue.add(second_one);
            }
        };
        delayedAdd2.setName(getClass().getCanonicalName() + "#testTake_multiple_matchers");
        delayedAdd2.start();

        // wait for blocked threads to die. This test will deadlock if failed
        waiter.join();
        waiter2.join();

        assertNull(mQueue.poll());
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll(IMatcher)} when queue is empty.
     */
    public void testPoll_condition_empty() {
        assertNull(mQueue.poll(new OneMatcher()));
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll(long, TimeUnit, IMatcher)} when queue is
     * empty.
     */
    public void testPoll_time_empty() throws InterruptedException {
        assertNull(mQueue.poll(100, TimeUnit.MILLISECONDS, new OneMatcher()));
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll(IMatcher)} when object matches, and one
     * doesn't.
     */
    public void testPoll_condition() {
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        mQueue.add(one);
        mQueue.add(two);
        assertEquals(one, mQueue.poll(new OneMatcher()));
        assertNull(mQueue.poll(new OneMatcher()));
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll(long, TimeUnit, IMatcher)} when object
     * matches, and one doesn't.
     */
    public void testPoll_time_condition() throws InterruptedException {
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        mQueue.add(one);
        mQueue.add(two);
        assertEquals(one, mQueue.poll(100, TimeUnit.MILLISECONDS, new OneMatcher()));
        assertNull(mQueue.poll(100, TimeUnit.MILLISECONDS, new OneMatcher()));
    }

    /**
     * Test {@link ConditionPriorityBlockingQueue#poll(IMatcher)} when object matches, and one
     * doesn't, using FIFO ordering.
     */
    public void testPoll_fifo_condition() {
        ConditionPriorityBlockingQueue<Integer> fifoQueue =
            new ConditionPriorityBlockingQueue<Integer>();
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        fifoQueue.add(two);
        fifoQueue.add(one);
        assertEquals(one, fifoQueue.poll(new OneMatcher()));
        assertNull(fifoQueue.poll(new OneMatcher()));
    }

    /**
     * Same as {@link ConditionPriorityBlockingQueueTest#testPoll_condition()}, but objects are
     * added in inverse order.
     */
    public void testPoll_condition_reverse() {
        Integer one = Integer.valueOf(1);
        Integer two = Integer.valueOf(2);
        mQueue.add(two);
        mQueue.add(one);
        assertEquals(one, mQueue.poll(new OneMatcher()));
        assertNull(mQueue.poll(new OneMatcher()));
    }

    /**
     * Test behavior when queue is modified during iteration
     * @throws Throwable
     */
    public void testModificationOnIterating() throws Throwable {
        final ConditionPriorityBlockingQueue<Integer> queue =
                new ConditionPriorityBlockingQueue<Integer>(new IntCompare());
        for (int i = 0; i < 10; i++) {
            queue.add(i);
        }
        final Throwable[] throwables = new Throwable[1];
        Thread iterator = new Thread() {
            @Override
            public void run() {
                try {
                    for (@SuppressWarnings("unused") Integer i : queue) {
                        Thread.sleep(10);
                    }
                } catch (Throwable t) {
                    throwables[0] = t;
                }
            }
        };
        iterator.setName(getClass().getCanonicalName() + "#testModificationOnIterating");
        iterator.start();
        for (int i = 0; i < 10 && throwables[0] == null; i++) {
            queue.add(i);
            Thread.sleep(10);
        }
        if (throwables[0] != null) {
            throw throwables[0];
        }
    }

    /**
     * A {@link Comparator} for {@link Integer}
     */
    private static class IntCompare implements Comparator<Integer> {

        @Override
        public int compare(Integer o1, Integer o2) {
            return o1.compareTo(o2);
        }

    }

    private static class OneMatcher implements IMatcher<Integer> {

        @Override
        public boolean matches(Integer element) {
            return element == 1;
        }
    }
}
