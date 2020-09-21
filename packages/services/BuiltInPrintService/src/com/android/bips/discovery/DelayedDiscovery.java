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

package com.android.bips.discovery;

import android.net.Uri;
import android.util.Log;

import com.android.bips.DelayedAction;

import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * A discovery that delays each discovery start and/or printer find by a fixed amount.
 */
public class DelayedDiscovery extends Discovery {
    private static final String TAG = DelayedDiscovery.class.getSimpleName();
    private static final boolean DEBUG = false;

    private final Discovery mChild;
    private final int mStartDelay;
    private final Listener mChildListener;
    private final Map<Uri, DiscoveredPrinter> mPending = new HashMap<>();
    private DelayedAction mDelayedStart;

    /**
     * Construct a Discovery that behaves like the wrapped discovery but with delays
     *
     * @param startDelay Milliseconds to delay the first start() command
     * @param findDelay  Milliseconds to delay each printer found
     */
    public DelayedDiscovery(Discovery wrapped, int startDelay, int findDelay) {
        super(wrapped.getPrintService());
        mChild = wrapped;
        mStartDelay = startDelay;
        mChildListener = new Listener() {
            @Override
            public void onPrinterFound(DiscoveredPrinter printer) {
                if (findDelay <= 0) {
                    printerFound(printer);
                    return;
                }

                DiscoveredPrinter old = mPending.put(printer.getUri(), printer);
                if (old == null) {
                    getHandler().postDelayed(() -> {
                        DiscoveredPrinter pending = mPending.remove(printer.getUri());
                        if (pending != null) {
                            if (DEBUG) Log.d(TAG, "Delay complete for " + pending);
                            printerFound(pending);
                        }
                    }, findDelay);
                }
            }

            @Override
            public void onPrinterLost(DiscoveredPrinter printer) {
                mPending.remove(printer.getUri());
                printerLost(printer.getUri());
            }
        };
    }

    @Override
    void onStart() {
        if (mStartDelay == 0) {
            mChild.start(mChildListener);
        } else {
            mDelayedStart = getPrintService().delay(mStartDelay, () -> {
                if (!isStarted()) {
                    return;
                }
                mChild.start(mChildListener);
            });
        }
    }

    @Override
    void onStop() {
        if (mDelayedStart != null) {
            mDelayedStart.cancel();
        }
        mChild.stop(mChildListener);
        mPending.clear();
    }

    @Override
    Collection<Discovery> getChildren() {
        return Collections.singleton(mChild);
    }
}
