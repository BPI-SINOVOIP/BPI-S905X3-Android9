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

package android.appsecurity.cts.orderedactivity;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

@RunWith(AndroidJUnit4.class)
public class PackageResolutionTest {
    @Test
    public void queryActivityOrdered() throws Exception {
        final PackageManager pm =
                InstrumentationRegistry.getInstrumentation().getContext().getPackageManager();
        final Intent intent = new Intent("android.cts.intent.action.ORDERED");
        intent.setData(Uri.parse("https://www.google.com/cts/packageresolution"));
        final List<ResolveInfo> resolve = pm.queryIntentActivities(intent, 0 /*flags*/);

        assertNotNull(resolve);
        assertEquals(4, resolve.size());
        assertEquals("android.appsecurity.cts.orderedactivity.OrderActivity3",
                resolve.get(0).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderActivity2",
                resolve.get(1).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderActivity1",
                resolve.get(2).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderActivityDefault",
                resolve.get(3).activityInfo.name);
    }

    @Test
    public void queryServiceOrdered() throws Exception {
        final PackageManager pm =
                InstrumentationRegistry.getInstrumentation().getContext().getPackageManager();
        final Intent intent = new Intent("android.cts.intent.action.ORDERED");
        intent.setData(Uri.parse("https://www.google.com/cts/packageresolution"));
        final List<ResolveInfo> resolve = pm.queryIntentServices(intent, 0 /*flags*/);
        
        assertNotNull(resolve);
        assertEquals(4, resolve.size());
        assertEquals("android.appsecurity.cts.orderedactivity.OrderService3",
                resolve.get(0).serviceInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderService2",
                resolve.get(1).serviceInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderService1",
                resolve.get(2).serviceInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderServiceDefault",
                resolve.get(3).serviceInfo.name);
    }

    @Test
    public void queryReceiverOrdered() throws Exception {
        final PackageManager pm =
                InstrumentationRegistry.getInstrumentation().getContext().getPackageManager();
        final Intent intent = new Intent("android.cts.intent.action.ORDERED");
        intent.setData(Uri.parse("https://www.google.com/cts/packageresolution"));
        final List<ResolveInfo> resolve = pm.queryBroadcastReceivers(intent, 0 /*flags*/);
        
        assertNotNull(resolve);
        assertEquals(4, resolve.size());
        assertEquals("android.appsecurity.cts.orderedactivity.OrderReceiver3",
                resolve.get(0).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderReceiver2",
                resolve.get(1).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderReceiver1",
                resolve.get(2).activityInfo.name);
        assertEquals("android.appsecurity.cts.orderedactivity.OrderReceiverDefault",
                resolve.get(3).activityInfo.name);
    }
}
