/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.autofillservice.cts;

import static com.google.common.truth.Truth.assertThat;
import static com.google.common.truth.Truth.assertWithMessage;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.view.PixelCopy;
import android.view.View;
import android.view.autofill.AutofillManager;

import com.android.compatibility.common.util.SynchronousPixelCopy;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
  * Base class for all activities in this test suite
  */
abstract class AbstractAutoFillActivity extends Activity {

    private MyAutofillCallback mCallback;

    /**
     * Run an action in the UI thread, and blocks caller until the action is finished.
     */
    public final void syncRunOnUiThread(Runnable action) {
        syncRunOnUiThread(action, Timeouts.UI_TIMEOUT.ms());
    }

    /**
     * Run an action in the UI thread, and blocks caller until the action is finished or it times
     * out.
     */
    public final void syncRunOnUiThread(Runnable action, long timeoutMs) {
        final CountDownLatch latch = new CountDownLatch(1);
        runOnUiThread(() -> {
            action.run();
            latch.countDown();
        });
        try {
            if (!latch.await(timeoutMs, TimeUnit.MILLISECONDS)) {
                throw new RetryableException("action on UI thread timed out after %d ms",
                        timeoutMs);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new RuntimeException("Interrupted", e);
        }
    }

    protected AutofillManager getAutofillManager() {
        return getSystemService(AutofillManager.class);
    }

    /**
     * Takes a screenshot from the whole activity.
     *
     * <p><b>Note:</b> this screenshot only contains the contents of the activity, it doesn't
     * include the autofill UIs; if you need to check that, please use
     * {@link UiBot#takeScreenshot()} instead.
     */
    public Bitmap takeScreenshot() {
        return takeScreenshot(findViewById(android.R.id.content).getRootView());
    }

    /**
     * Takes a screenshot from the a view.
     */
    public Bitmap takeScreenshot(View view) {
        final Rect srcRect = new Rect();
        syncRunOnUiThread(() -> view.getGlobalVisibleRect(srcRect));
        final Bitmap dest = Bitmap.createBitmap(
                srcRect.width(), srcRect.height(), Bitmap.Config.ARGB_8888);

        final SynchronousPixelCopy copy = new SynchronousPixelCopy();
        final int copyResult = copy.request(getWindow(), srcRect, dest);
        assertThat(copyResult).isEqualTo(PixelCopy.SUCCESS);

        return dest;
    }

    /**
     * Registers and returns a custom callback for autofill events.
     *
     * <p>Note: caller doesn't need to call {@link #unregisterCallback()}, it will be automatically
     * unregistered on {@link #finish()}.
     */
    protected MyAutofillCallback registerCallback() {
        assertWithMessage("already registered").that(mCallback).isNull();
        mCallback = new MyAutofillCallback();
        getAutofillManager().registerCallback(mCallback);
        return mCallback;
    }

    /**
     * Unregister the callback from the {@link AutofillManager}.
     *
     * <p>This method just neeed to be called when a test case wants to explicitly test the behavior
     * of the activity when the callback is unregistered.
     */
    protected void unregisterCallback() {
        assertWithMessage("not registered").that(mCallback).isNotNull();
        unregisterNonNullCallback();
    }

    private void unregisterNonNullCallback() {
        getAutofillManager().unregisterCallback(mCallback);
        mCallback = null;
    }

    @Override
    public void finish() {
        if (mCallback != null) {
            unregisterNonNullCallback();
        }
        super.finish();
    }
}
