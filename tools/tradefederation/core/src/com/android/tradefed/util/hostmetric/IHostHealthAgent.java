/*
 * Copyright (C) 2015 The Android Open Source Project
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
package com.android.tradefed.util.hostmetric;

import java.util.Map;

/**
 * An interface to emit host or device metrics.
 */
public interface IHostHealthAgent {

    /**
     * Emits a value for a metric name. The value gets enqueued and uploaded when
     * {@link IHostHealthAgent#flush} gets called.
     *
     * @param name a metric name
     * @param value a metric value
     * @param data data associated with the metric value
     */
    public void emitValue(String name, long value, Map<String, String> data);

    /**
     * Flushes enqueued metrics.
     */
    public void flush();
}
