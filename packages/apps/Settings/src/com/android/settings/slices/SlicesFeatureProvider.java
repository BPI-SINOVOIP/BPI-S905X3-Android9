package com.android.settings.slices;

import android.content.Context;

import com.android.settings.wifi.calling.WifiCallingSliceHelper;

/**
 * Manages Slices in Settings.
 */
public interface SlicesFeatureProvider {

    boolean DEBUG = false;

    SlicesIndexer getSliceIndexer(Context context);

    SliceDataConverter getSliceDataConverter(Context context);

    /**
     * Asynchronous call to index the data used to build Slices.
     * If the data is already indexed, the data will not change.
     */
    void indexSliceDataAsync(Context context);

    /**
     * Indexes the data used to build Slices.
     * If the data is already indexed, the data will not change.
     */
    void indexSliceData(Context context);

    /**
     * Gets new WifiCallingSliceHelper object
     */
    WifiCallingSliceHelper getNewWifiCallingSliceHelper(Context context);
}
