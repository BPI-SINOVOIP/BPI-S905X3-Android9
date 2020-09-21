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
package android.device.collectors;

import android.os.Bundle;
import android.support.annotation.VisibleForTesting;

import java.io.File;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Object to hold all the data collected by metric collectors.
 */
public class DataRecord {
    // TODO: expend type supports to more complex type: Object,etc.
    private LinkedHashMap<String, String> mCurrentStringMetrics = new LinkedHashMap<>();
    private LinkedHashMap<String, File> mCurrentFileMetrics = new LinkedHashMap<>();
    private LinkedHashMap<String, byte[]> mCurrentBinaryMetrics = new LinkedHashMap<>();

    /**
     * Add a metric to be tracked by a key.
     *
     * @param key the key under which to find the metric
     * @param value the value associated with the key
     */
    public void addStringMetric(String key, String value) {
        mCurrentStringMetrics.put(key, value);
    }

    /**
     * Add a metric file to be tracked under a key. It will be reported in the instrumentation
     * results as the key and absolute path to the file.
     *
     * @param fileKey the key under which the file will be found.
     * @param value the {@link File} associated to the key.
     */
    public void addFileMetric(String fileKey, File value) {
        mCurrentFileMetrics.put(fileKey, value);
    }

    /**
     * Add a byte[] metric to be tracked by a key.
     *
     * @param key the key under which to find the metric
     * @param value the byte[] value associated with the key
     */
    public void addBinaryMetric(String key, byte[] value) {
        mCurrentBinaryMetrics.put(key, value);
    }

    /**
     * Returns True if the {@link DataRecord} already contains some metrics, False otherwise.
     */
    public boolean hasMetrics() {
        return (mCurrentStringMetrics.size() + mCurrentFileMetrics.size()
                + mCurrentBinaryMetrics.size()) > 0;
    }

    /**
     * Returns all the string and file data received so far to the map of metrics that will be
     * reported.
     */
    private Map<String, String> getStringMetrics() {
        Map<String, String> res = new LinkedHashMap<>();
        for (Map.Entry<String, File> entry : mCurrentFileMetrics.entrySet()) {
            res.put(entry.getKey(), entry.getValue().getAbsolutePath());
        }
        res.putAll(mCurrentStringMetrics);
        return res;
    }

    /**
     * Create a {@link Bundle} and populate it with the metrics, or return null if no metrics are
     * available.
     */
    final Bundle createBundleFromMetrics() {
        Map<String, String> map = getStringMetrics();
        Bundle b = createBundle();
        for (String key : map.keySet()) {
            b.putString(key, map.get(key));
        }
        for (String key : mCurrentBinaryMetrics.keySet()) {
            b.putByteArray(key, mCurrentBinaryMetrics.get(key));
        }
        return b;
    }

    /**
     * Create a {@link Bundle} that will hold the metrics. Exposed for testing.
     */
    @VisibleForTesting
    Bundle createBundle() {
        return new Bundle();
    }
}
