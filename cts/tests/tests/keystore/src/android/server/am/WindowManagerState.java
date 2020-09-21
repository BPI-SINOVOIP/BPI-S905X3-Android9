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

import static android.app.WindowConfiguration.ACTIVITY_TYPE_STANDARD;
import static android.app.WindowConfiguration.ACTIVITY_TYPE_UNDEFINED;
import static android.app.WindowConfiguration.WINDOWING_MODE_UNDEFINED;
import static android.server.am.ProtoExtractors.extract;
import static android.server.am.StateLogger.log;
import static android.server.am.StateLogger.logE;
import static android.view.Display.DEFAULT_DISPLAY;

import static org.junit.Assert.fail;

import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.support.test.InstrumentationRegistry;
import android.view.nano.DisplayInfoProto;

import com.android.server.wm.nano.AppTransitionProto;
import com.android.server.wm.nano.AppWindowTokenProto;
import com.android.server.wm.nano.ConfigurationContainerProto;
import com.android.server.wm.nano.DisplayFramesProto;
import com.android.server.wm.nano.DisplayProto;
import com.android.server.wm.nano.IdentifierProto;
import com.android.server.wm.nano.PinnedStackControllerProto;
import com.android.server.wm.nano.StackProto;
import com.android.server.wm.nano.TaskProto;
import com.android.server.wm.nano.WindowContainerProto;
import com.android.server.wm.nano.WindowManagerServiceDumpProto;
import com.android.server.wm.nano.WindowStateAnimatorProto;
import com.android.server.wm.nano.WindowStateProto;
import com.android.server.wm.nano.WindowSurfaceControllerProto;
import com.android.server.wm.nano.WindowTokenProto;

import com.google.protobuf.nano.InvalidProtocolBufferNanoException;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

public class WindowManagerState {
    private static final String DUMPSYS_WINDOW = "dumpsys window -a --proto";

    private static final String STARTING_WINDOW_PREFIX = "Starting ";
    private static final String DEBUGGER_WINDOW_PREFIX = "Waiting For Debugger: ";

    // Windows in z-order with the top most at the front of the list.
    private List<WindowState> mWindowStates = new ArrayList();
    // Stacks in z-order with the top most at the front of the list, starting with primary display.
    private final List<WindowStack> mStacks = new ArrayList();
    // Stacks on all attached displays, in z-order with the top most at the front of the list.
    private final Map<Integer, List<WindowStack>> mDisplayStacks
            = new HashMap<>();
    private List<Display> mDisplays = new ArrayList();
    private String mFocusedWindow = null;
    private String mFocusedApp = null;
    private Rect mDefaultPinnedStackBounds = new Rect();
    private Rect mPinnedStackMovementBounds = new Rect();
    private final LinkedList<String> mSysDump = new LinkedList();
    private int mRotation;
    private boolean mIsDockedStackMinimized;

    public void computeState() {
        // It is possible the system is in the middle of transition to the right state when we get
        // the dump. We try a few times to get the information we need before giving up.
        int retriesLeft = 3;
        boolean retry = false;
        byte[] dump = null;

        log("==============================");
        log("      WindowManagerState      ");
        log("==============================");
        do {
            if (retry) {
                log("***Incomplete WM state. Retrying...");
                // Wait half a second between retries for window manager to finish transitioning...
                SystemClock.sleep(500);
            }

            dump = executeShellCommand(DUMPSYS_WINDOW);
            try {
                parseSysDumpProto(dump);
            } catch (InvalidProtocolBufferNanoException ex) {
                throw new RuntimeException("Failed to parse dumpsys:\n"
                        + new String(dump, StandardCharsets.UTF_8), ex);
            }

            retry = mWindowStates.isEmpty() || mFocusedApp == null;
        } while (retry && retriesLeft-- > 0);

        if (mWindowStates.isEmpty()) {
            logE("No Windows found...");
        }
        if (mFocusedWindow == null) {
            logE("No Focused Window...");
        }
        if (mFocusedApp == null) {
            logE("No Focused App...");
        }
    }

    private byte[] executeShellCommand(String cmd) {
        try {
            ParcelFileDescriptor pfd =
                    InstrumentationRegistry.getInstrumentation().getUiAutomation()
                            .executeShellCommand(cmd);
            byte[] buf = new byte[512];
            int bytesRead;
            FileInputStream fis = new ParcelFileDescriptor.AutoCloseInputStream(pfd);
            ByteArrayOutputStream stdout = new ByteArrayOutputStream();
            while ((bytesRead = fis.read(buf)) != -1) {
                stdout.write(buf, 0, bytesRead);
            }
            fis.close();
            return stdout.toByteArray();
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }


    private void parseSysDumpProto(byte[] sysDump) throws InvalidProtocolBufferNanoException {
        reset();
        WindowManagerServiceDumpProto state = WindowManagerServiceDumpProto.parseFrom(sysDump);
        List<WindowState> allWindows = new ArrayList<>();
        Map<String, WindowState> windowMap = new HashMap<>();
        if (state.focusedWindow != null) {
            mFocusedWindow = state.focusedWindow.title;
        }
        mFocusedApp = state.focusedApp;
        for (int i = 0; i < state.rootWindowContainer.displays.length; i++) {
            DisplayProto displayProto = state.rootWindowContainer.displays[i];
            final Display display = new Display(displayProto);
            mDisplays.add(display);
            allWindows.addAll(display.getWindows());
            List<WindowStack> stacks = new ArrayList<>();
            for (int j = 0; j < displayProto.stacks.length; j++) {
                StackProto stackProto = displayProto.stacks[j];
                final WindowStack stack = new WindowStack(stackProto);
                mStacks.add(stack);
                stacks.add(stack);
                allWindows.addAll(stack.getWindows());
            }
            mDisplayStacks.put(display.mDisplayId, stacks);

            // use properties from the default display only
            if (display.getDisplayId() == DEFAULT_DISPLAY) {
                if (displayProto.dockedStackDividerController != null) {
                    mIsDockedStackMinimized =
                            displayProto.dockedStackDividerController.minimizedDock;
                }
                PinnedStackControllerProto pinnedStackProto = displayProto.pinnedStackController;
                if (pinnedStackProto != null) {
                    mDefaultPinnedStackBounds = extract(pinnedStackProto.defaultBounds);
                    mPinnedStackMovementBounds = extract(pinnedStackProto.movementBounds);
                }
            }
        }
        for (WindowState w : allWindows) {
            windowMap.put(w.getToken(), w);
        }
        for (int i = 0; i < state.rootWindowContainer.windows.length; i++) {
            IdentifierProto identifierProto = state.rootWindowContainer.windows[i];
            String hash_code = Integer.toHexString(identifierProto.hashCode);
            mWindowStates.add(windowMap.get(hash_code));
        }
        mRotation = state.rotation;
        AppTransitionProto appTransitionProto = state.appTransition;
    }

    public void getMatchingVisibleWindowState(final String windowName, List<WindowState> windowList) {
        windowList.clear();
        for (WindowState ws : mWindowStates) {
            if (ws.isShown() && windowName.equals(ws.getName())) {
                windowList.add(ws);
            }
        }
    }

    public WindowState getWindowByPackageName(String packageName, int windowType) {
        for (WindowState ws : mWindowStates) {
            final String name = ws.getName();
            if (name == null || !name.contains(packageName)) {
                continue;
            }
            if (windowType != ws.getType()) {
                continue;
            }
            return ws;
        }

        return null;
    }

    public void getWindowsByPackageName(String packageName, List<Integer> restrictToTypeList,
            List<WindowState> outWindowList) {
        outWindowList.clear();
        for (WindowState ws : mWindowStates) {
            final String name = ws.getName();
            if (name == null || !name.contains(packageName)) {
                continue;
            }
            if (restrictToTypeList != null && !restrictToTypeList.contains(ws.getType())) {
                continue;
            }
            outWindowList.add(ws);
        }
    }

    Display getDisplay(int displayId) {
        for (Display display : mDisplays) {
            if (displayId == display.getDisplayId()) {
                return display;
            }
        }
        return null;
    }

    List<Display> getDisplays() {
        return mDisplays;
    }

    String getFrontWindow() {
        if (mWindowStates == null || mWindowStates.isEmpty()) {
            return null;
        }
        return mWindowStates.get(0).getName();
    }

    public String getFocusedWindow() {
        return mFocusedWindow;
    }

    public String getFocusedApp() {
        return mFocusedApp;
    }

    public int getRotation() {
        return mRotation;
    }

    /**
     * Check if at least one window which matches provided window name is visible.
     */
    boolean isWindowVisible(String windowName) {
        for (WindowState window : mWindowStates) {
            if (window.getName().equals(windowName)) {
                if (window.isShown()) {
                    return true;
                }
            }
        }
        return false;
    }

    WindowStack getStack(int stackId) {
        for (WindowStack stack : mStacks) {
            if (stackId == stack.mStackId) {
                return stack;
            }
        }
        return null;
    }

    public boolean isDockedStackMinimized() {
        return mIsDockedStackMinimized;
    }

    public int getZOrder(WindowState w) {
        return mWindowStates.size() - mWindowStates.indexOf(w);
    }

    private void reset() {
        mSysDump.clear();
        mStacks.clear();
        mDisplays.clear();
        mWindowStates.clear();
        mDisplayStacks.clear();
        mFocusedWindow = null;
        mFocusedApp = null;
        mIsDockedStackMinimized = false;
        mDefaultPinnedStackBounds.setEmpty();
        mPinnedStackMovementBounds.setEmpty();
        mRotation = 0;
    }

    static class WindowStack extends WindowContainer {

        int mStackId;
        ArrayList<WindowTask> mTasks = new ArrayList<>();
        boolean mWindowAnimationBackgroundSurfaceShowing;

        WindowStack(StackProto proto) {
            super(proto.windowContainer);
            mStackId = proto.id;
            mFullscreen = proto.fillsParent;
            mBounds = extract(proto.bounds);
            for (int i = 0; i < proto.tasks.length; i++) {
                TaskProto taskProto = proto.tasks[i];
                WindowTask task = new WindowTask(taskProto);
                mTasks.add(task);
                mSubWindows.addAll(task.getWindows());
            }
            mWindowAnimationBackgroundSurfaceShowing = proto.animationBackgroundSurfaceIsDimming;
        }

        WindowTask getTask(int taskId) {
            for (WindowTask task : mTasks) {
                if (taskId == task.mTaskId) {
                    return task;
                }
            }
            return null;
        }
    }

    static class WindowTask extends WindowContainer {

        int mTaskId;
        Rect mTempInsetBounds;
        List<String> mAppTokens = new ArrayList<>();

        WindowTask(TaskProto proto) {
            super(proto.windowContainer);
            mTaskId = proto.id;
            mFullscreen = proto.fillsParent;
            mBounds = extract(proto.bounds);
            for (int i = 0; i < proto.appWindowTokens.length; i++) {
                AppWindowTokenProto appWindowTokenProto = proto.appWindowTokens[i];
                mAppTokens.add(appWindowTokenProto.name);
                WindowTokenProto windowTokenProto = appWindowTokenProto.windowToken;
                for (int j = 0; j < windowTokenProto.windows.length; j++) {
                    WindowStateProto windowProto = windowTokenProto.windows[j];
                    WindowState window = new WindowState(windowProto);
                    mSubWindows.add(window);
                    mSubWindows.addAll(window.getWindows());
                }
            }
            mTempInsetBounds = extract(proto.tempInsetBounds);
        }
    }

    static class ConfigurationContainer {
        final Configuration mOverrideConfiguration = new Configuration();
        final Configuration mFullConfiguration = new Configuration();
        final Configuration mMergedOverrideConfiguration = new Configuration();

        ConfigurationContainer(ConfigurationContainerProto proto) {
            if (proto == null) {
                return;
            }
            mOverrideConfiguration.setTo(extract(proto.overrideConfiguration));
            mFullConfiguration.setTo(extract(proto.fullConfiguration));
            mMergedOverrideConfiguration.setTo(extract(proto.mergedOverrideConfiguration));
        }

        int getWindowingMode() {
            if (mFullConfiguration == null) {
                return WINDOWING_MODE_UNDEFINED;
            }
            return mFullConfiguration.windowConfiguration.getWindowingMode();
        }

        int getActivityType() {
            if (mFullConfiguration == null) {
                return ACTIVITY_TYPE_UNDEFINED;
            }
            return mFullConfiguration.windowConfiguration.getActivityType();
        }
    }

    static abstract class WindowContainer extends ConfigurationContainer {

        protected boolean mFullscreen;
        protected Rect mBounds;
        protected int mOrientation;
        protected List<WindowState> mSubWindows = new ArrayList<>();

        WindowContainer(WindowContainerProto proto) {
            super(proto.configurationContainer);
            mOrientation = proto.orientation;
        }

        Rect getBounds() {
            return mBounds;
        }

        boolean isFullscreen() {
            return mFullscreen;
        }

        List<WindowState> getWindows() {
            return mSubWindows;
        }
    }

    static class Display extends WindowContainer {

        private final int mDisplayId;
        private Rect mDisplayRect = new Rect();
        private Rect mAppRect = new Rect();
        private int mDpi;
        private String mName;

        public Display(DisplayProto proto) {
            super(proto.windowContainer);
            mDisplayId = proto.id;
            for (int i = 0; i < proto.aboveAppWindows.length; i++) {
                addWindowsFromTokenProto(proto.aboveAppWindows[i]);
            }
            for (int i = 0; i < proto.belowAppWindows.length; i++) {
                addWindowsFromTokenProto(proto.belowAppWindows[i]);
            }
            for (int i = 0; i < proto.imeWindows.length; i++) {
                addWindowsFromTokenProto(proto.imeWindows[i]);
            }
            mDpi = proto.dpi;
            DisplayInfoProto infoProto = proto.displayInfo;
            if (infoProto != null) {
                mDisplayRect.set(0, 0, infoProto.logicalWidth, infoProto.logicalHeight);
                mAppRect.set(0, 0, infoProto.appWidth, infoProto.appHeight);
                mName = infoProto.name;
            }
        }

        private void addWindowsFromTokenProto(WindowTokenProto proto) {
            for (int j = 0; j < proto.windows.length; j++) {
                WindowStateProto windowProto = proto.windows[j];
                WindowState childWindow = new WindowState(windowProto);
                mSubWindows.add(childWindow);
                mSubWindows.addAll(childWindow.getWindows());
            }
        }

        int getDisplayId() {
            return mDisplayId;
        }

        int getDpi() {
            return mDpi;
        }

        Rect getDisplayRect() {
            return mDisplayRect;
        }

        String getName() {
            return mName;
        }

        @Override
        public String toString() {
            return "Display #" + mDisplayId + ": name=" + mName + " mDisplayRect=" + mDisplayRect
                    + " mAppRect=" + mAppRect;
        }
    }

    public static class WindowState extends WindowContainer {

        private static final int WINDOW_TYPE_STARTING = 1;
        private static final int WINDOW_TYPE_EXITING = 2;
        private static final int WINDOW_TYPE_DEBUGGER = 3;

        private String mName;
        private final String mAppToken;
        private final int mWindowType;
        private int mType = 0;
        private int mDisplayId;
        private int mStackId;
        private boolean mShown;
        private Rect mContainingFrame = new Rect();
        private Rect mParentFrame = new Rect();
        private Rect mContentFrame = new Rect();
        private Rect mFrame = new Rect();
        private Rect mCrop = new Rect();

        WindowState(WindowStateProto proto) {
            super(proto.windowContainer);
            IdentifierProto identifierProto = proto.identifier;
            mName = identifierProto.title;
            mAppToken = Integer.toHexString(identifierProto.hashCode);
            mDisplayId = proto.displayId;
            mStackId = proto.stackId;
            if (proto.attributes != null) {
                mType = proto.attributes.type;
            }
            WindowStateAnimatorProto animatorProto = proto.animator;
            if (animatorProto != null) {
                if (animatorProto.surface != null) {
                    WindowSurfaceControllerProto surfaceProto = animatorProto.surface;
                    mShown = surfaceProto.shown;
                }
                mCrop = extract(animatorProto.lastClipRect);
            }
            mFrame = extract(proto.frame);
            mContainingFrame = extract(proto.containingFrame);
            mParentFrame = extract(proto.parentFrame);
            mContentFrame = extract(proto.contentFrame);
            if (mName.startsWith(STARTING_WINDOW_PREFIX)) {
                mWindowType = WINDOW_TYPE_STARTING;
                // Existing code depends on the prefix being removed
                mName = mName.substring(STARTING_WINDOW_PREFIX.length());
            } else if (proto.animatingExit) {
                mWindowType = WINDOW_TYPE_EXITING;
            } else if (mName.startsWith(DEBUGGER_WINDOW_PREFIX)) {
                mWindowType = WINDOW_TYPE_STARTING;
                mName = mName.substring(DEBUGGER_WINDOW_PREFIX.length());
            } else {
                mWindowType = 0;
            }
            for (int i = 0; i < proto.childWindows.length; i++) {
                WindowStateProto childProto = proto.childWindows[i];
                WindowState childWindow = new WindowState(childProto);
                mSubWindows.add(childWindow);
                mSubWindows.addAll(childWindow.getWindows());
            }
        }

        public String getName() {
            return mName;
        }

        String getToken() {
            return mAppToken;
        }

        int getDisplayId() {
            return mDisplayId;
        }

        int getStackId() {
            return mStackId;
        }

        public Rect getFrame() {
            return mFrame;
        }

        public Rect getContentFrame() {
            return mContentFrame;
        }

        Rect getCrop() {
            return mCrop;
        }

        public boolean isShown() {
            return mShown;
        }

        public int getType() {
            return mType;
        }

        private String getWindowTypeSuffix(int windowType) {
            switch (windowType) {
                case WINDOW_TYPE_STARTING:
                    return " STARTING";
                case WINDOW_TYPE_EXITING:
                    return " EXITING";
                case WINDOW_TYPE_DEBUGGER:
                    return " DEBUGGER";
                default:
                    break;
            }
            return "";
        }

        @Override
        public String toString() {
            return "WindowState: {" + mAppToken + " " + mName
                    + getWindowTypeSuffix(mWindowType) + "}" + " type=" + mType
                    + " cf=" + mContainingFrame + " pf=" + mParentFrame;
        }
    }
}
