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
package android.longevity.platform;

import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.longevity.platform.listener.BatteryTerminator;
import android.longevity.platform.listener.ErrorTerminator;
import android.longevity.platform.listener.TimeoutTerminator;
import android.os.BatteryManager;
import android.os.Bundle;
import android.support.annotation.VisibleForTesting;
import android.support.test.InstrumentationRegistry;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;

import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.RunnerBuilder;

/**
 * {@inheritDoc}
 *
 * This class is used for constructing longevity suites that run on an Android device.
 */
public final class LongevitySuite extends android.longevity.core.LongevitySuite {
    private static final String LOG_TAG = LongevitySuite.class.getSimpleName();

    private Instrumentation mInstrumentation;
    private Context mContext;

    /**
     * Takes a {@link Bundle} and maps all String K/V pairs into a {@link Map<String, String>}.
     *
     * @param bundle the input arguments to return in a {@link Map}
     * @return Map<String, String> all String-to-String key, value pairs in the {@link Bundle}
     */
    private static final Map<String, String> toMap(Bundle bundle) {
        Map<String, String> result = new HashMap<>();
        for (String key : bundle.keySet()) {
            if (!bundle.containsKey(key)) {
                Log.w(LOG_TAG, String.format("Couldn't find value for option: %s", key));
            } else {
                // Arguments are assumed String <-> String
                result.put(key, bundle.getString(key));
            }
        }
        return result;
    }

    /**
     * Called reflectively on classes annotated with {@code @RunWith(LongevitySuite.class)}
     */
    public LongevitySuite(Class<?> klass, RunnerBuilder builder)
            throws InitializationError {
        this(klass, builder, InstrumentationRegistry.getInstrumentation(),
                InstrumentationRegistry.getContext(), InstrumentationRegistry.getArguments());
    }

    /**
     * Used to pass in mock-able Android features for testing.
     */
    @VisibleForTesting
    public LongevitySuite(Class<?> klass, RunnerBuilder builder,
            Instrumentation instrumentation, Context context, Bundle arguments)
            throws InitializationError {
        super(klass, builder, toMap(arguments));
        mInstrumentation = instrumentation;
        mContext = context;
    }

    @Override
    public void run(final RunNotifier notifier) {
        // Register the battery terminator available only on the platform library, if present.
        if (hasBattery()) {
            notifier.addListener(new BatteryTerminator(notifier, mArguments, mContext));
        }
        // Register other listeners and continue with standard longevity run.
        super.run(notifier);
    }

    /**
     * Returns the platform-specific {@link TimeoutTerminator} for Android devices.
     */
    @Override
    public android.longevity.core.listener.ErrorTerminator getErrorTerminator(
            final RunNotifier notifier) {
        return new ErrorTerminator(notifier);
    }

    /**
     * Returns the platform-specific {@link TimeoutTerminator} for Android devices.
     */
    @Override
    public android.longevity.core.listener.TimeoutTerminator getTimeoutTerminator(
            final RunNotifier notifier) {
        return new TimeoutTerminator(notifier, mArguments);
    }

    /**
     * Determines if the device has a battery attached.
     */
    private boolean hasBattery () {
        final Intent batteryInfo =
                mContext.registerReceiver(null, new IntentFilter(Intent.ACTION_BATTERY_CHANGED));
        return batteryInfo.getBooleanExtra(BatteryManager.EXTRA_PRESENT, true);
    }
}
