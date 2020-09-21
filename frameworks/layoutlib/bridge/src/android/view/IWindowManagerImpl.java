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

package android.view;

import android.app.IAssistDataReceiver;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.GraphicBuffer;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.Region;
import android.os.Bundle;
import android.os.IBinder;
import android.os.IRemoteCallback;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.util.DisplayMetrics;
import android.view.RemoteAnimationAdapter;

import com.android.internal.os.IResultReceiver;
import com.android.internal.policy.IKeyguardDismissCallback;
import com.android.internal.policy.IShortcutService;
import com.android.internal.view.IInputContext;
import com.android.internal.view.IInputMethodClient;

/**
 * Basic implementation of {@link IWindowManager} so that {@link Display} (and
 * {@link Display_Delegate}) can return a valid instance.
 */
public class IWindowManagerImpl implements IWindowManager {

    private final Configuration mConfig;
    private final DisplayMetrics mMetrics;
    private final int mRotation;
    private final boolean mHasNavigationBar;

    public IWindowManagerImpl(Configuration config, DisplayMetrics metrics, int rotation,
            boolean hasNavigationBar) {
        mConfig = config;
        mMetrics = metrics;
        mRotation = rotation;
        mHasNavigationBar = hasNavigationBar;
    }

    // custom API.

    public DisplayMetrics getMetrics() {
        return mMetrics;
    }

    // ---- implementation of IWindowManager that we care about ----

    @Override
    public int getDefaultDisplayRotation() throws RemoteException {
        return mRotation;
    }

    @Override
    public boolean hasNavigationBar() {
        return mHasNavigationBar;
    }

    // ---- unused implementation of IWindowManager ----

    @Override
    public int getNavBarPosition() throws RemoteException {
        return 0;
    }

    @Override
    public void addWindowToken(IBinder arg0, int arg1, int arg2) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void clearForcedDisplaySize(int displayId) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void clearForcedDisplayDensityForUser(int displayId, int userId) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void setOverscan(int displayId, int left, int top, int right, int bottom)
            throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void closeSystemDialogs(String arg0) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void startFreezingScreen(int exitAnim, int enterAnim) {
        // TODO Auto-generated method stub
    }

    @Override
    public void stopFreezingScreen() {
        // TODO Auto-generated method stub
    }

    @Override
    public void disableKeyguard(IBinder arg0, String arg1) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void executeAppTransition() throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void exitKeyguardSecurely(IOnKeyguardExitResult arg0) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void freezeRotation(int arg0) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public float getAnimationScale(int arg0) throws RemoteException {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public float[] getAnimationScales() throws RemoteException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int getPendingAppTransition() throws RemoteException {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public boolean inputMethodClientHasFocus(IInputMethodClient arg0) throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isKeyguardLocked() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isKeyguardSecure() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isViewServerRunning() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public IWindowSession openSession(IWindowSessionCallback argn1, IInputMethodClient arg0,
            IInputContext arg1) throws RemoteException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void overridePendingAppTransition(String arg0, int arg1, int arg2,
            IRemoteCallback startedCallback) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void overridePendingAppTransitionScaleUp(int startX, int startY, int startWidth,
            int startHeight) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionClipReveal(int startX, int startY,
            int startWidth, int startHeight) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionThumb(GraphicBuffer srcThumb, int startX, int startY,
            IRemoteCallback startedCallback, boolean scaleUp) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionAspectScaledThumb(GraphicBuffer srcThumb, int startX,
            int startY, int targetWidth, int targetHeight, IRemoteCallback startedCallback,
            boolean scaleUp) {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionInPlace(String packageName, int anim) {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionMultiThumbFuture(
            IAppTransitionAnimationSpecsFuture specsFuture, IRemoteCallback startedCallback,
            boolean scaleUp) throws RemoteException {

    }

    @Override
    public void overridePendingAppTransitionMultiThumb(AppTransitionAnimationSpec[] specs,
            IRemoteCallback callback0, IRemoteCallback callback1, boolean scaleUp) {
        // TODO Auto-generated method stub
    }

    @Override
    public void overridePendingAppTransitionRemote(RemoteAnimationAdapter adapter) {
    }

    @Override
    public void prepareAppTransition(int arg0, boolean arg1) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void reenableKeyguard(IBinder arg0) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void removeWindowToken(IBinder arg0, int arg1) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public boolean requestAssistScreenshot(IAssistDataReceiver receiver)
            throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void setAnimationScale(int arg0, float arg1) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public void setAnimationScales(float[] arg0) throws RemoteException {
        // TODO Auto-generated method stub

    }

    @Override
    public float getCurrentAnimatorScale() throws RemoteException {
        return 0;
    }

    @Override
    public void setEventDispatching(boolean arg0) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void setFocusedApp(IBinder arg0, boolean arg1) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void getInitialDisplaySize(int displayId, Point size) {
        // TODO Auto-generated method stub
    }

    @Override
    public void getBaseDisplaySize(int displayId, Point size) {
        // TODO Auto-generated method stub
    }

    @Override
    public void setForcedDisplaySize(int displayId, int arg0, int arg1) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public int getInitialDisplayDensity(int displayId) {
        return -1;
    }

    @Override
    public int getBaseDisplayDensity(int displayId) {
        return -1;
    }

    @Override
    public void setForcedDisplayDensityForUser(int displayId, int density, int userId)
            throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void setForcedDisplayScalingMode(int displayId, int mode) {
    }

    @Override
    public void setInTouchMode(boolean arg0) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public int[] setNewDisplayOverrideConfiguration(Configuration arg0, int displayId)
            throws RemoteException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void refreshScreenCaptureDisabled(int userId) {
        // TODO Auto-generated method stub
    }

    @Override
    public void updateRotation(boolean arg0, boolean arg1) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void setStrictModeVisualIndicatorPreference(String arg0) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void showStrictModeViolation(boolean arg0) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public boolean startViewServer(int arg0) throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void statusBarVisibilityChanged(int arg0) throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public void setRecentsVisibility(boolean visible) {
        // TODO Auto-generated method stub
    }

    @Override
    public void setPipVisibility(boolean visible) {
        // TODO Auto-generated method stub
    }

    @Override
    public void setShelfHeight(boolean visible, int shelfHeight) {
        // TODO Auto-generated method stub
    }

    @Override
    public void setNavBarVirtualKeyHapticFeedbackEnabled(boolean enabled) {
    }

    @Override
    public boolean stopViewServer() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void thawRotation() throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public Configuration updateOrientationFromAppTokens(Configuration arg0, IBinder arg1, int arg2)
            throws RemoteException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int watchRotation(IRotationWatcher arg0, int arg1) throws RemoteException {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public void removeRotationWatcher(IRotationWatcher arg0) throws RemoteException {
    }

    @Override
    public IBinder asBinder() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int getPreferredOptionsPanelGravity() throws RemoteException {
        return Gravity.CENTER_HORIZONTAL | Gravity.BOTTOM;
    }

    @Override
    public void dismissKeyguard(IKeyguardDismissCallback callback, CharSequence message)
            throws RemoteException {
    }

    @Override
    public void setSwitchingUser(boolean switching) throws RemoteException {
    }

    @Override
    public void lockNow(Bundle options) {
        // TODO Auto-generated method stub
    }

    @Override
    public boolean isSafeModeEnabled() {
        return false;
    }

    @Override
    public boolean isRotationFrozen() throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public void enableScreenIfNeeded() throws RemoteException {
        // TODO Auto-generated method stub
    }

    @Override
    public boolean clearWindowContentFrameStats(IBinder token) throws RemoteException {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public WindowContentFrameStats getWindowContentFrameStats(IBinder token)
            throws RemoteException {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public int getDockedStackSide() throws RemoteException {
        return 0;
    }

    @Override
    public void endProlongedAnimations() {
    }

    @Override
    public void registerDockedStackListener(IDockedStackListener listener) throws RemoteException {
    }

    @Override
    public void registerPinnedStackListener(int displayId, IPinnedStackListener listener) throws RemoteException {
    }

    @Override
    public void setResizeDimLayer(boolean visible, int targetStackId, float alpha)
            throws RemoteException {
    }

    @Override
    public void setDockedStackDividerTouchRegion(Rect touchableRegion) throws RemoteException {
    }

    @Override
    public void requestAppKeyboardShortcuts(
            IResultReceiver receiver, int deviceId) throws RemoteException {
    }

    @Override
    public void getStableInsets(int displayId, Rect outInsets) throws RemoteException {
    }

    @Override
    public void registerShortcutKey(long shortcutCode, IShortcutService service)
        throws RemoteException {}

    @Override
    public void createInputConsumer(IBinder token, String name, InputChannel inputChannel)
            throws RemoteException {}

    @Override
    public boolean destroyInputConsumer(String name) throws RemoteException {
        return false;
    }

    @Override
    public Bitmap screenshotWallpaper() throws RemoteException {
        return null;
    }

    @Override
    public Region getCurrentImeTouchRegion() throws RemoteException {
        return null;
    }

    @Override
    public boolean registerWallpaperVisibilityListener(IWallpaperVisibilityListener listener,
            int displayId) throws RemoteException {
        return false;
    }

    @Override
    public void unregisterWallpaperVisibilityListener(IWallpaperVisibilityListener listener,
            int displayId) throws RemoteException {
    }

    @Override
    public void startWindowTrace() throws RemoteException {
    }

    @Override
    public void stopWindowTrace() throws RemoteException {
    }

    @Override
    public boolean isWindowTraceEnabled() throws RemoteException {
        return false;
    }

    @Override
    public void requestUserActivityNotification() throws RemoteException {
    }

    @Override
    public void dontOverrideDisplayInfo(int displayId) throws RemoteException {
    }
}
