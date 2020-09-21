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

/**
 * Timeouts for common tasks.
 */
final class Timeouts {

    /**
     * Timeout until framework binds / unbinds from service.
     */
    static final Timeout CONNECTION_TIMEOUT = new Timeout("CONNECTION_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout until framework unbinds from a service.
     */
    // TODO: must be higher than RemoteFillService.TIMEOUT_IDLE_BIND_MILLIS, so we should use a
    // @hidden @Testing constants instead...
    static final Timeout IDLE_UNBIND_TIMEOUT = new Timeout("IDLE_UNBIND_TIMEOUT", 10000, 2F, 10000);

    /**
     * Timeout to get the expected number of fill events.
     */
    static final Timeout FILL_EVENTS_TIMEOUT = new Timeout("FILL_EVENTS_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout for expected autofill requests.
     */
    static final Timeout FILL_TIMEOUT = new Timeout("FILL_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout for expected save requests.
     */
    static final Timeout SAVE_TIMEOUT = new Timeout("SAVE_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout used when save is not expected to be shown - test will sleep for that amount of time
     * as there is no callback that be received to assert it's not shown.
     */
    static final long SAVE_NOT_SHOWN_NAPTIME_MS = 5000;

    /**
     * Timeout for UI operations. Typically used by {@link UiBot}.
     */
    static final Timeout UI_TIMEOUT = new Timeout("UI_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout for webview operations. Typically used by {@link UiBot}.
     */
    static final Timeout WEBVIEW_TIMEOUT = new Timeout("WEBVIEW_TIMEOUT", 8000, 2F, 16000);

    /**
     * Timeout for showing the autofill dataset picker UI.
     *
     * <p>The value is usually higher than {@link #UI_TIMEOUT} because the performance of the
     * dataset picker UI can be affect by external factors in some low-level devices.
     *
     * <p>Typically used by {@link UiBot}.
     */
    static final Timeout UI_DATASET_PICKER_TIMEOUT =
            new Timeout("UI_DATASET_PICKER_TIMEOUT", 5000, 2F, 10000);

    /**
     * Timeout used when the dataset picker is not expected to be shown - test will sleep for that
     * amount of time as there is no callback that be received to assert it's not shown.
     */
    static final long DATASET_PICKER_NOT_SHOWN_NAPTIME_MS = 5000;

    /**
     * Timeout (in milliseconds) for an activity to be brought out to top.
     */
    static final Timeout ACTIVITY_RESURRECTION =
            new Timeout("ACTIVITY_RESURRECTION", 6000, 3F, 20000);

    /**
     * Timeout for changing the screen orientation.
     */
    static final Timeout UI_SCREEN_ORIENTATION_TIMEOUT =
            new Timeout("UI_SCREEN_ORIENTATION_TIMEOUT", 5000, 2F, 10000);

    private Timeouts() {
        throw new UnsupportedOperationException("contain static methods only");
    }
}
