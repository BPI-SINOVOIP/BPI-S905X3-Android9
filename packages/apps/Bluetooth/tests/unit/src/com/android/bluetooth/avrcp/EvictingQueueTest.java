package com.android.bluetooth.avrcp;

import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 *  Unit tests for {@link EvictingQueue}.
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class EvictingQueueTest {
    @Test
    public void testEvictingQueue_canAddItems() {
        EvictingQueue<Integer> e = new EvictingQueue<Integer>(10);

        e.add(1);

        Assert.assertEquals((long) e.size(), (long) 1);
    }

    @Test
    public void testEvictingQueue_maxItems() {
        EvictingQueue<Integer> e = new EvictingQueue<Integer>(5);

        e.add(1);
        e.add(2);
        e.add(3);
        e.add(4);
        e.add(5);
        e.add(6);

        Assert.assertEquals((long) e.size(), (long) 5);
        // Items drop off the front
        Assert.assertEquals((long) e.peek(), (long) 2);
    }

    @Test
    public void testEvictingQueue_frontDrop() {
        EvictingQueue<Integer> e = new EvictingQueue<Integer>(5);

        e.add(1);
        e.add(2);
        e.add(3);
        e.add(4);
        e.add(5);

        Assert.assertEquals((long) e.size(), (long) 5);

        e.addFirst(6);

        Assert.assertEquals((long) e.size(), (long) 5);
        Assert.assertEquals((long) e.peek(), (long) 1);
    }
}
