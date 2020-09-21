/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package android.server.am;

import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;
import static android.hardware.display.DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC;
import static android.server.am.ActivityLauncher.KEY_DISPLAY_ID;
import static android.server.am.ActivityLauncher.KEY_LAUNCH_ACTIVITY;
import static android.server.am.ActivityLauncher.KEY_TARGET_COMPONENT;
import static android.server.am.ComponentNameUtils.getActivityName;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_CREATE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_DESTROY_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.COMMAND_RESIZE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.KEY_CAN_SHOW_WITH_INSECURE_KEYGUARD;
import static android.server.am.Components.VirtualDisplayActivity.KEY_COMMAND;
import static android.server.am.Components.VirtualDisplayActivity.KEY_COUNT;
import static android.server.am.Components.VirtualDisplayActivity.KEY_DENSITY_DPI;
import static android.server.am.Components.VirtualDisplayActivity.KEY_LAUNCH_TARGET_COMPONENT;
import static android.server.am.Components.VirtualDisplayActivity.KEY_PUBLIC_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.KEY_RESIZE_DISPLAY;
import static android.server.am.Components.VirtualDisplayActivity.VIRTUAL_DISPLAY_PREFIX;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Intent;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;

import java.util.HashMap;

/**
 * Activity that is able to create and destroy a virtual display.
 */
public class VirtualDisplayActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = VirtualDisplayActivity.class.getSimpleName();

    private static final int DEFAULT_DENSITY_DPI = 160;

    // Container for details about a pending virtual display creation request.
    private static class VirtualDisplayRequest {
        final SurfaceView surfaceView;
        final Bundle extras;

        VirtualDisplayRequest(SurfaceView surfaceView, Bundle extras) {
            this.surfaceView = surfaceView;
            this.extras = extras;
        }
    }

    // Container to hold association between an active virtual display and surface view.
    private static class VirtualDisplayEntry {
        final VirtualDisplay display;
        final SurfaceView surfaceView;
        final boolean resizeDisplay;
        final int density;

        VirtualDisplayEntry(VirtualDisplay display, SurfaceView surfaceView, int density,
                boolean resizeDisplay) {
            this.display = display;
            this.surfaceView = surfaceView;
            this.density = density;
            this.resizeDisplay = resizeDisplay;
        }
    }

    private final HashMap<Surface, VirtualDisplayRequest> mPendingDisplayRequests = new HashMap<>();
    private final HashMap<Surface, VirtualDisplayEntry> mVirtualDisplays = new HashMap<>();
    private DisplayManager mDisplayManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.virtual_display_layout);

        mDisplayManager = (DisplayManager) getSystemService(DISPLAY_SERVICE);
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        final Bundle extras = intent.getExtras();
        if (extras == null) {
            return;
        }

        String command = extras.getString(KEY_COMMAND);
        switch (command) {
            case COMMAND_CREATE_DISPLAY:
                createVirtualDisplay(extras);
                break;
            case COMMAND_DESTROY_DISPLAY:
                destroyVirtualDisplays();
                break;
            case COMMAND_RESIZE_DISPLAY:
                resizeDisplay();
                break;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        destroyVirtualDisplays();
    }

    private void createVirtualDisplay(Bundle extras) {
        final int requestedCount = extras.getInt(KEY_COUNT, 1);
        Log.d(TAG, "createVirtualDisplays. requested count:" + requestedCount);

        for (int displayCount = 0; displayCount < requestedCount; ++displayCount) {
            final ViewGroup root = findViewById(android.R.id.content);
            final SurfaceView surfaceView = new SurfaceView(this);
            surfaceView.setLayoutParams(new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
            surfaceView.getHolder().addCallback(this);
            mPendingDisplayRequests.put(surfaceView.getHolder().getSurface(),
                    new VirtualDisplayRequest(surfaceView, extras));
            root.addView(surfaceView);
        }
    }

    private void destroyVirtualDisplays() {
        Log.d(TAG, "destroyVirtualDisplays");
        final ViewGroup root = findViewById(android.R.id.content);

        for (VirtualDisplayEntry entry : mVirtualDisplays.values()) {
            Log.d(TAG, "destroying:" + entry.display);
            entry.display.release();
            root.removeView(entry.surfaceView);
        }

        mPendingDisplayRequests.clear();
        mVirtualDisplays.clear();
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        final VirtualDisplayRequest entry =
                mPendingDisplayRequests.remove(surfaceHolder.getSurface());

        if (entry == null) {
            return;
        }

        final int densityDpi = entry.extras.getInt(KEY_DENSITY_DPI, DEFAULT_DENSITY_DPI);
        final boolean resizeDisplay = entry.extras.getBoolean(KEY_RESIZE_DISPLAY);
        final String launchComponentName = entry.extras.getString(KEY_LAUNCH_TARGET_COMPONENT);
        final Surface surface = surfaceHolder.getSurface();

        // Initially, the surface will not have a set width or height so rely on the parent.
        // This should be accurate with match parent on both params.
        final int width = surfaceHolder.getSurfaceFrame().width();
        final int height = surfaceHolder.getSurfaceFrame().height();

        int flags = 0;

        final boolean canShowWithInsecureKeyguard
                = entry.extras.getBoolean(KEY_CAN_SHOW_WITH_INSECURE_KEYGUARD);
        if (canShowWithInsecureKeyguard) {
            flags |= 1 << 5; // VIRTUAL_DISPLAY_FLAG_CAN_SHOW_WITH_INSECURE_KEYGUARD
        }

        final boolean publicDisplay = entry.extras.getBoolean(KEY_PUBLIC_DISPLAY);
        if (publicDisplay) {
            flags |= VIRTUAL_DISPLAY_FLAG_PUBLIC | VIRTUAL_DISPLAY_FLAG_OWN_CONTENT_ONLY;
        }

        Log.d(TAG, "createVirtualDisplay: " + width + "x" + height + ", dpi: "
                + densityDpi + ", canShowWithInsecureKeyguard=" + canShowWithInsecureKeyguard
                + ", publicDisplay=" + publicDisplay);
        try {
            VirtualDisplay virtualDisplay = mDisplayManager.createVirtualDisplay(
                    VIRTUAL_DISPLAY_PREFIX + mVirtualDisplays.size(), width,
                    height, densityDpi, surface, flags);
            mVirtualDisplays.put(surface,
                    new VirtualDisplayEntry(virtualDisplay, entry.surfaceView, densityDpi,
                            resizeDisplay));
            if (launchComponentName != null) {
                final ComponentName targetActivity =
                        ComponentName.unflattenFromString(launchComponentName);
                final int displayId = virtualDisplay.getDisplay().getDisplayId();
                Log.d(TAG, "Launch activity after display created: activityName="
                        + getActivityName(targetActivity) + ", displayId=" + displayId);
                launchActivity(targetActivity, displayId);
            }
        } catch (IllegalArgumentException e) {
            final ViewGroup root = findViewById(android.R.id.content);
            // This is expected when trying to create show-when-locked public display.
            root.removeView(entry.surfaceView);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int format, int width, int height) {
        final VirtualDisplayEntry entry = mVirtualDisplays.get(surfaceHolder.getSurface());

        if (entry != null && entry.resizeDisplay) {
            entry.display.resize(width, height, entry.density);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
    }

    /** Resize virtual display to half of the surface frame size. */
    private void resizeDisplay() {
        final VirtualDisplayEntry vd = (VirtualDisplayEntry) mVirtualDisplays.values().toArray()[0];
        final SurfaceHolder surfaceHolder = vd.surfaceView.getHolder();
        vd.display.resize(surfaceHolder.getSurfaceFrame().width() / 2,
                surfaceHolder.getSurfaceFrame().height() / 2, vd.density);
    }

    private void launchActivity(ComponentName activityName, int displayId) {
        final Bundle extras = new Bundle();
        extras.putBoolean(KEY_LAUNCH_ACTIVITY, true);
        extras.putString(KEY_TARGET_COMPONENT, getActivityName(activityName));
        extras.putInt(KEY_DISPLAY_ID, displayId);
        ActivityLauncher.launchActivityFromExtras(this, extras);
    }
}
