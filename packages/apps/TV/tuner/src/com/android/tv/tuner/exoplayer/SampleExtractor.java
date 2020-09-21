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
package com.android.tv.tuner.exoplayer;

import android.os.Handler;
import com.google.android.exoplayer.MediaFormat;
import com.google.android.exoplayer.MediaFormatHolder;
import com.google.android.exoplayer.SampleHolder;
import com.google.android.exoplayer.SampleSource;
import com.google.android.exoplayer.TrackRenderer;
import java.io.IOException;
import java.util.List;

/**
 * Extractor for reading track metadata and samples stored in tracks.
 *
 * <p>Call {@link #prepare} until it returns {@code true}, then access track metadata via {@link
 * #getTrackFormats} and {@link #getTrackMediaFormat}.
 *
 * <p>Pass indices of tracks to read from to {@link #selectTrack}. A track can later be deselected
 * by calling {@link #deselectTrack}. It is safe to select/deselect tracks after reading sample data
 * or seeking. Initially, all tracks are deselected.
 *
 * <p>Call {@link #release()} when the extractor is no longer needed to free resources.
 */
public interface SampleExtractor {

    /**
     * If the extractor is currently having difficulty preparing or loading samples, then this
     * method throws the underlying error. Otherwise does nothing.
     *
     * @throws IOException The underlying error.
     */
    void maybeThrowError() throws IOException;

    /**
     * Prepares the extractor for reading track metadata and samples.
     *
     * @return whether the source is ready; if {@code false}, this method must be called again.
     * @throws IOException thrown if the source can't be read
     */
    boolean prepare() throws IOException;

    /** Returns track information about all tracks that can be selected. */
    List<MediaFormat> getTrackFormats();

    /** Selects the track at {@code index} for reading sample data. */
    void selectTrack(int index);

    /** Deselects the track at {@code index}, so no more samples will be read from that track. */
    void deselectTrack(int index);

    /**
     * Returns an estimate of the position up to which data is buffered.
     *
     * <p>This method should not be called until after the extractor has been successfully prepared.
     *
     * @return an estimate of the absolute position in microseconds up to which data is buffered, or
     *     {@link TrackRenderer#END_OF_TRACK_US} if data is buffered to the end of the stream, or
     *     {@link TrackRenderer#UNKNOWN_TIME_US} if no estimate is available.
     */
    long getBufferedPositionUs();

    /**
     * Seeks to the specified time in microseconds.
     *
     * <p>This method should not be called until after the extractor has been successfully prepared.
     *
     * @param positionUs the seek position in microseconds
     */
    void seekTo(long positionUs);

    /** Stores the {@link MediaFormat} of {@code track}. */
    void getTrackMediaFormat(int track, MediaFormatHolder outMediaFormatHolder);

    /**
     * Reads the next sample in the track at index {@code track} into {@code sampleHolder},
     * returning {@link SampleSource#SAMPLE_READ} if it is available.
     *
     * <p>Advances to the next sample if a sample was read.
     *
     * @param track the index of the track from which to read a sample
     * @param sampleHolder the holder for read sample data, if {@link SampleSource#SAMPLE_READ} is
     *     returned
     * @return {@link SampleSource#SAMPLE_READ} if a sample was read into {@code sampleHolder}, or
     *     {@link SampleSource#END_OF_STREAM} if the last samples in all tracks have been read, or
     *     {@link SampleSource#NOTHING_READ} if the sample cannot be read immediately as it is not
     *     loaded.
     */
    int readSample(int track, SampleHolder sampleHolder);

    /** Releases resources associated with this extractor. */
    void release();

    /** Indicates to the source that it should still be buffering data. */
    boolean continueBuffering(long positionUs);

    /**
     * Sets OnCompletionListener for notifying the completion of SampleExtractor.
     *
     * @param listener the OnCompletionListener
     * @param handler the {@link Handler} for {@link Handler#post(Runnable)} of OnCompletionListener
     */
    void setOnCompletionListener(OnCompletionListener listener, Handler handler);

    /** The listener for SampleExtractor being completed. */
    interface OnCompletionListener {

        /**
         * Called when sample extraction is completed.
         *
         * @param result {@code true} when the extractor is finished without an error, {@code false}
         *     otherwise (storage error, weak signal, being reached at EoS prematurely, etc.)
         * @param lastExtractedPositionUs the last extracted position when extractor is completed
         */
        void onCompletion(boolean result, long lastExtractedPositionUs);
    }
}
