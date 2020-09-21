/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package android.app.componentfactory.cts;

import static org.junit.Assert.assertTrue;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class AppComponentFactoryTest {

    private final Context mContext = InstrumentationRegistry.getContext();

    @Test
    public void testApplication() {
        assertTrue(mContext.getApplicationContext() instanceof MyApplication);
    }

    @Test
    public void testActivity() {
        mContext.startActivity(new Intent()
                .setComponent(new ComponentName(mContext.getPackageName(),
                        mContext.getPackageName() + ".inject.MyActivity"))
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
    }

    @Test
    public void testReceiver() {
        mContext.sendBroadcast(new Intent()
                .setComponent(new ComponentName(mContext.getPackageName(),
                        mContext.getPackageName() + ".inject.MyReceiver")));
    }

    @Test
    public void testService() {
        mContext.startService(new Intent()
                .setComponent(new ComponentName(mContext.getPackageName(),
                        mContext.getPackageName() + ".inject.MyService")));
    }

    // No provider test needed because of their weird lifecycle.
}
