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

package com.android.camera.functional;

import com.android.camera.Camera;
import com.android.camera.R;
import com.android.camera.VideoCamera;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;
import android.os.Process;
import android.provider.MediaStore;
import android.test.InstrumentationTestCase;
import android.test.suitebuilder.annotation.LargeTest;
import android.util.Log;
import android.view.KeyEvent;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

public class CameraTest extends InstrumentationTestCase {
    private static final String TAG = "CameraTest";

    @LargeTest
    public void testVideoCaptureIntentFdLeak() throws Exception {
        Intent intent = new Intent(MediaStore.ACTION_VIDEO_CAPTURE);
        intent.setClass(getInstrumentation().getTargetContext(), VideoCamera.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.putExtra(MediaStore.EXTRA_OUTPUT, Uri.parse("file://"
                + Environment.getExternalStorageDirectory().toString()
                + "test_fd_leak.3gp"));
        getInstrumentation().startActivitySync(intent).finish();
        // Test if the fd is closed.
        for (File f: new File("/proc/" + Process.myPid() + "/fd").listFiles()) {
            assertEquals(-1, f.getCanonicalPath().indexOf("test_fd_leak.3gp"));
        }
    }

    @LargeTest
    public void testActivityLeak() throws Exception {
        checkActivityLeak(Camera.class);
        checkActivityLeak(VideoCamera.class);
    }

    private void checkActivityLeak(Class<?> cls) throws Exception {
        final int TEST_COUNT = 5;
        Intent intent = new Intent();
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setClass(getInstrumentation().getTargetContext(), cls);
        ArrayList<WeakReference<Activity>> refs =
                new ArrayList<WeakReference<Activity>>();
        for (int i = 0; i < TEST_COUNT; i++) {
            Activity activity = getInstrumentation().startActivitySync(intent);
            refs.add(new WeakReference<Activity>(activity));
            activity.finish();
            getInstrumentation().waitForIdleSync();
            activity = null;
        }
        Runtime.getRuntime().gc();
        Runtime.getRuntime().runFinalization();
        Runtime.getRuntime().gc();
        int refCount = 0;
        for (WeakReference<Activity> c: refs) {
            if (c.get() != null) refCount++;
        }
        // If applications are leaking activity, every reference is reachable.
        assertTrue(refCount != TEST_COUNT);
    }
}
