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
package com.android.media.tests;

import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.Pair;

import java.awt.image.BufferedImage;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;

import javax.imageio.ImageIO;

/**
 * Class that analyzes a screenshot captured from AudioLoopback test. There is a wave form in the
 * screenshot that has specific colors (TARGET_COLOR). This class extracts those colors and analyzes
 * wave amplitude, duration and form and make a decision if it's a legitimate wave form or not.
 */
public class AudioLoopbackImageAnalyzer {

    // General
    private static final int VERTICAL_THRESHOLD = 0;
    private static final int PRIMARY_WAVE_COLOR = 0xFF1E4A99;
    private static final int SECONDARY_WAVE_COLOR = 0xFF1D4998;
    private static final int[] TARGET_COLORS_TABLET =
            new int[] {PRIMARY_WAVE_COLOR, SECONDARY_WAVE_COLOR};
    private static final int[] TARGET_COLORS_PHONE =
            new int[] {PRIMARY_WAVE_COLOR, SECONDARY_WAVE_COLOR};

    private static final float EXPERIMENTAL_WAVE_MAX_TABLET = 69.0f; // In percent of image height
    private static final float EXPERIMENTAL_WAVE_MAX_PHONE = 32.0f; // In percent of image height

    // Image
    private static final int TABLET_SCREEN_MIN_WIDTH = 1700;
    private static final int TABLET_SCREEN_MIN_HEIGHT = 2300;

    // Duration parameters
    // Max duration should not span more than 2 of the 11 sections in the graph
    // Min duration should not be less than 1/4 of a section
    private static final float SECTION_WIDTH_IN_PERCENT = 100 * 1 / 11; // In percent of image width
    private static final float DURATION_MIN = SECTION_WIDTH_IN_PERCENT / 4;

    // Amplitude
    // Required numbers of column for a response
    private static final int MIN_NUMBER_OF_COLUMNS = 4;
    // The difference between two amplitude columns should not be more than this
    private static final float MAX_ALLOWED_COLUMN_DECREASE = 0.42f;
    // Only check MAX_ALLOWED_COLUMN_DECREASE up to this number
    private static final float MIN_NUMBER_OF_DECREASING_COLUMNS = 8;
    // Minimum space between two amplitude columns
    private static final int MIN_SPACE_BETWEEN_TWO_COLUMNS = 4;
    private static final int MIN_SPACE_BETWEEN_TWO_COLUMNS_TABLET = 5;

    enum Result {
        PASS,
        FAIL,
        UNKNOWN
    }

    private static class Amplitude {
        public int maxHeight = -1;
        public int zeroCounter = 0;
    }

    public static Pair<Result, String> analyzeImage(String imgFile) {
        final String FN_TAG = "AudioLoopbackImageAnalyzer.analyzeImage";

        BufferedImage img = null;
        try {
            final File f = new File(imgFile);
            img = ImageIO.read(f);
        } catch (final IOException e) {
            CLog.e(e);
            throw new RuntimeException("Error loading image file '" + imgFile + "'");
        }

        final int width = img.getWidth();
        final int height = img.getHeight();

        CLog.i("image width=" + width + ", height=" + height);

        // Compute thresholds and min/max values based on image witdh, height
        final float waveMax;
        final int[] targetColors;
        final int amplitudeCenterMaxDiff;
        final float maxDuration;
        final int minNrOfZeroesBetweenAmplitudes;
        final int horizontalStart; //ignore anything left of this bound
        int horizontalThreshold = 10;

        if (width >= TABLET_SCREEN_MIN_WIDTH && height >= TABLET_SCREEN_MIN_HEIGHT) {
            CLog.i("Apply TABLET config values");
            waveMax = EXPERIMENTAL_WAVE_MAX_TABLET;
            amplitudeCenterMaxDiff = 40;
            maxDuration = 5 * SECTION_WIDTH_IN_PERCENT;
            targetColors = TARGET_COLORS_TABLET;
            horizontalStart = Math.round(1.7f * SECTION_WIDTH_IN_PERCENT * width / 100.0f);
            horizontalThreshold = 40;
            minNrOfZeroesBetweenAmplitudes = MIN_SPACE_BETWEEN_TWO_COLUMNS_TABLET;
        } else {
            waveMax = EXPERIMENTAL_WAVE_MAX_PHONE;
            amplitudeCenterMaxDiff = 20;
            maxDuration = 2.5f * SECTION_WIDTH_IN_PERCENT;
            targetColors = TARGET_COLORS_PHONE;
            horizontalStart = 0;
            minNrOfZeroesBetweenAmplitudes = MIN_SPACE_BETWEEN_TWO_COLUMNS;
        }

        // Amplitude
        // Max height should be about 80% of wave max.
        // Min height should be about 40% of wave max.
        final float AMPLITUDE_MAX_VALUE = waveMax * 0.8f;
        final float AMPLITUDE_MIN_VALUE = waveMax * 0.4f;

        final int[] vertical = new int[height];
        final int[] horizontal = new int[width];

        projectPixelsToXAxis(img, targetColors, horizontal, width, height);
        filter(horizontal, horizontalThreshold);
        final Pair<Integer, Integer> durationBounds = getBounds(horizontal, horizontalStart, -1);
        if (!boundsWithinRange(durationBounds, 0, width)) {
            final String fmt = "%1$s Upper/Lower bound along horizontal axis not found";
            final String err = String.format(fmt, FN_TAG);
            CLog.w(err);
            return new Pair<Result, String>(Result.FAIL, err);
        }

        projectPixelsToYAxis(img, targetColors, vertical, height, durationBounds);
        filter(vertical, VERTICAL_THRESHOLD);
        final Pair<Integer, Integer> amplitudeBounds = getBounds(vertical, -1, -1);
        if (!boundsWithinRange(durationBounds, 0, height)) {
            final String fmt = "%1$s: Upper/Lower bound along vertical axis not found";
            final String err = String.format(fmt, FN_TAG);
            CLog.w(err);
            return new Pair<Result, String>(Result.FAIL, err);
        }

        final int durationLeft = durationBounds.first.intValue();
        final int durationRight = durationBounds.second.intValue();
        final int amplitudeTop = amplitudeBounds.first.intValue();
        final int amplitudeBottom = amplitudeBounds.second.intValue();

        final float amplitude = (amplitudeBottom - amplitudeTop) * 100.0f / height;
        final float duration = (durationRight - durationLeft) * 100.0f / width;

        CLog.i("AudioLoopbackImageAnalyzer: Amplitude=" + amplitude + ", Duration=" + duration);

        Pair<Result, String> amplResult =
                analyzeAmplitude(
                        vertical,
                        amplitude,
                        amplitudeTop,
                        amplitudeBottom,
                        AMPLITUDE_MIN_VALUE,
                        AMPLITUDE_MAX_VALUE,
                        amplitudeCenterMaxDiff);
        if (amplResult.first != Result.PASS) {
            return amplResult;
        }

        amplResult =
                analyzeDuration(
                        horizontal,
                        duration,
                        durationLeft,
                        durationRight,
                        DURATION_MIN,
                        maxDuration,
                        MIN_NUMBER_OF_COLUMNS,
                        minNrOfZeroesBetweenAmplitudes);
        if (amplResult.first != Result.PASS) {
            return amplResult;
        }

        return new Pair<Result, String>(Result.PASS, "");
    }

    /**
     * Function to analyze the waveforms duration (how wide it stretches along x-axis) and to make
     * sure the waveform degrades nicely, i.e. the amplitude columns becomes smaller and smaller
     * over time.
     *
     * @param horizontal - int array with waveforms amplitude values
     * @param duration - calculated length of duration in percent of screen width
     * @param durationLeft - index for "horizontal" where waveform starts
     * @param durationRight - index for "horizontal" where waveform ends
     * @param durationMin - if duration is below this value, return FAIL and failure reason
     * @param durationMax - if duration exceed this value, return FAIL and failure reason
     * @param minNumberOfAmplitudes - min number of amplitudes (columns) in waveform to pass test
     * @param minNrOfZeroesBetweenAmplitudes - min number of required zeroes between amplitudes
     * @return - returns result status and failure reason, if any
     */
    private static Pair<Result, String> analyzeDuration(
            int[] horizontal,
            float duration,
            int durationLeft,
            int durationRight,
            final float durationMin,
            final float durationMax,
            final int minNumberOfAmplitudes,
            final int minNrOfZeroesBetweenAmplitudes) {
        // This is the tricky one; basically, there should be "columns" that starts
        // at "durationLeft", with the tallest column to the left and then column
        // height will drop until it fades completely after "durationRight".
        final String FN_TAG = "AudioLoopbackImageAnalyzer.analyzeDuration";

        if (duration < durationMin || duration > durationMax) {
            final String fmt = "%1$s: Duration outside range, value=%2$f, range=(%3$f,%4$f)";
            return handleError(fmt, FN_TAG, duration, durationMin, durationMax);
        }

        final ArrayList<Amplitude> amplitudes = new ArrayList<Amplitude>();
        Amplitude currentAmplitude = new Amplitude();
        amplitudes.add(currentAmplitude);
        int zeroCounter = 0;

        // Create amplitude objects that track largest amplitude within a "group" in the array.
        // Example array:
        //      [ 202, 530, 420, 12, 0, 0, 0, 0, 0, 0, 0, 236, 423, 262, 0, 0, 0, 0, 0, 0, 0, 0 ]
        // We would get two amplitude objects with amplitude 530 and 423. Each amplitude object
        // will also get the number of zeroes to the next amplitude, i.e. 7 and 8 respectively.
        for (int i = durationLeft; i < durationRight; i++) {
            final int v = horizontal[i];
            if (v == 0) {
                // Count how many consecutive zeroes we have
                zeroCounter++;
                continue;
            }

            CLog.i("index=" + i + ", v=" + v);

            if (zeroCounter >= minNrOfZeroesBetweenAmplitudes) {
                // Found a new amplitude; update old amplitude
                // with the "gap" count - i.e. nr of zeroes between the amplitudes
                if (currentAmplitude != null) {
                    currentAmplitude.zeroCounter = zeroCounter;
                }

                // Create new Amplitude object
                currentAmplitude = new Amplitude();
                amplitudes.add(currentAmplitude);
            }

            // Reset counter
            zeroCounter = 0;

            if (currentAmplitude != null && v > currentAmplitude.maxHeight) {
                currentAmplitude.maxHeight = v;
            }
        }

        StringBuilder sb = new StringBuilder(128);
        int counter = 0;
        for (final Amplitude a : amplitudes) {
            CLog.i(
                    sb.append("Amplitude=")
                            .append(counter)
                            .append(", MaxHeight=")
                            .append(a.maxHeight)
                            .append(", ZeroesToNextColumn=")
                            .append(a.zeroCounter)
                            .toString());
            counter++;
            sb.setLength(0);
        }

        if (amplitudes.size() < minNumberOfAmplitudes) {
            final String fmt = "%1$s: Not enough amplitude columns, value=%2$d";
            return handleError(fmt, FN_TAG, amplitudes.size());
        }

        int currentColumnHeight = -1;
        int oldColumnHeight = -1;
        for (int i = 0; i < amplitudes.size(); i++) {
            if (i == 0) {
                oldColumnHeight = amplitudes.get(i).maxHeight;
                continue;
            }

            currentColumnHeight = amplitudes.get(i).maxHeight;
            if (oldColumnHeight > currentColumnHeight) {
                // We want at least a good number of columns that declines nicely.
                // After MIN_NUMBER_OF_DECREASING_COLUMNS, we don't really care that much
                if (i < MIN_NUMBER_OF_DECREASING_COLUMNS
                        && currentColumnHeight < (oldColumnHeight * MAX_ALLOWED_COLUMN_DECREASE)) {
                    final String fmt =
                            "%1$s: Amplitude column heights declined too much, "
                                    + "old=%2$d, new=%3$d, column=%4$d";
                    return handleError(fmt, FN_TAG, oldColumnHeight, currentColumnHeight, i);
                }
                oldColumnHeight = currentColumnHeight;
            } else if (oldColumnHeight == currentColumnHeight) {
                if (i < MIN_NUMBER_OF_DECREASING_COLUMNS) {
                    final String fmt =
                            "%1$s: Amplitude column heights are same, "
                                    + "old=%2$d, new=%3$d, column=%4$d";
                    return handleError(fmt, FN_TAG, oldColumnHeight, currentColumnHeight, i);
                }
            } else {
                final String fmt =
                        "%1$s: Amplitude column heights don't decline, "
                                + "old=%2$d, new=%3$d, column=%4$d";
                return handleError(fmt, FN_TAG, oldColumnHeight, currentColumnHeight, i);
            }
        }

        return new Pair<Result, String>(Result.PASS, "");
    }

    /**
     * Function to analyze the waveforms duration (how wide it stretches along x-axis) and to make
     * sure the waveform degrades nicely, i.e. the amplitude columns becomes smaller and smaller
     * over time.
     *
     * @param vertical - integer array with waveforms amplitude accumulated values
     * @param amplitude - calculated height of amplitude in percent of screen height
     * @param amplitudeTop - index in "vertical" array where waveform starts
     * @param amplitudeBottom - index in "vertical" array where waveform ends
     * @param amplitudeMin - if amplitude is below this value, return FAIL and failure reason
     * @param amplitudeMax - if amplitude exceed this value, return FAIL and failure reason
     * @param amplitudeCenterDiffThreshold - threshold to check that waveform is centered
     * @return - returns result status and failure reason, if any
     */
    private static Pair<Result, String> analyzeAmplitude(
            int[] vertical,
            float amplitude,
            int amplitudeTop,
            int amplitudeBottom,
            final float amplitudeMin,
            final float amplitudeMax,
            final int amplitudeCenterDiffThreshold) {
        final String FN_TAG = "AudioLoopbackImageAnalyzer.analyzeAmplitude";

        if (amplitude < amplitudeMin || amplitude > amplitudeMax) {
            final String fmt = "%1$s: Amplitude outside range, value=%2$f, range=(%3$f,%4$f)";
            final String err = String.format(fmt, FN_TAG, amplitude, amplitudeMin, amplitudeMax);
            CLog.w(err);
            return new Pair<Result, String>(Result.FAIL, err);
        }

        // Are the amplitude top/bottom centered around the centerline?
        final int amplitudeCenter = getAmplitudeCenter(vertical, amplitudeTop, amplitudeBottom);
        final int topDiff = amplitudeCenter - amplitudeTop;
        final int bottomDiff = amplitudeBottom - amplitudeCenter;
        final int diff = Math.abs(topDiff - bottomDiff);

        if (diff < amplitudeCenterDiffThreshold) {
            return new Pair<Result, String>(Result.PASS, "");
        }

        final String fmt =
                "%1$s: Amplitude not centered topDiff=%2$d, bottomDiff=%3$d, "
                        + "center=%4$d, diff=%5$d";
        final String err = String.format(fmt, FN_TAG, topDiff, bottomDiff, amplitudeCenter, diff);
        CLog.w(err);
        return new Pair<Result, String>(Result.FAIL, err);
    }

    private static int getAmplitudeCenter(int[] vertical, int amplitudeTop, int amplitudeBottom) {
        int max = -1;
        int center = -1;
        for (int i = amplitudeTop; i < amplitudeBottom; i++) {
            if (vertical[i] > max) {
                max = vertical[i];
                center = i;
            }
        }

        return center;
    }

    private static void projectPixelsToXAxis(
            BufferedImage img,
            final int[] targetColors,
            int[] horizontal,
            final int width,
            final int height) {
        // "Flatten image" by projecting target colors horizontally,
        // counting number of found pixels in each column
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                final int color = img.getRGB(x, y);
                for (final int targetColor : targetColors) {
                    if (color == targetColor) {
                        horizontal[x]++;
                        break;
                    }
                }
            }
        }
    }

    private static void projectPixelsToYAxis(
            BufferedImage img,
            final int[] targetColors,
            int[] vertical,
            int height,
            Pair<Integer, Integer> horizontalMinMax) {

        final int min = horizontalMinMax.first.intValue();
        final int max = horizontalMinMax.second.intValue();

        // "Flatten image" by projecting target colors (between min/max) vertically,
        // counting number of found pixels in each row

        // Pass over y-axis, restricted to horizontalMin, horizontalMax
        for (int y = 0; y < height; y++) {
            for (int x = min; x <= max; x++) {
                final int color = img.getRGB(x, y);
                for (final int targetColor : targetColors) {
                    if (color == targetColor) {
                        vertical[y]++;
                        break;
                    }
                }
            }
        }
    }

    private static Pair<Integer, Integer> getBounds(int[] array, int lowerBound, int upperBound) {
        // Determine min, max
        if (lowerBound == -1) {
            lowerBound = 0;
        }

        if (upperBound == -1) {
            upperBound = array.length - 1;
        }

        int min = -1;
        for (int i = lowerBound; i <= upperBound; i++) {
            if (array[i] > 0) {
                min = i;
                break;
            }
        }

        int max = -1;
        for (int i = upperBound; i >= lowerBound; i--) {
            if (array[i] > 0) {
                max = i;
                break;
            }
        }

        return new Pair<Integer, Integer>(Integer.valueOf(min), Integer.valueOf(max));
    }

    private static void filter(int[] array, final int threshold) {
        // Filter horizontal array; set all values < threshold to 0
        for (int i = 0; i < array.length; i++) {
            final int v = array[i];
            if (v != 0 && v <= threshold) {
                array[i] = 0;
            }
        }
    }

    private static boolean boundsWithinRange(Pair<Integer, Integer> bounds, int low, int high) {
        return low <= bounds.first.intValue()
                && bounds.first.intValue() < high
                && low <= bounds.second.intValue()
                && bounds.second.intValue() < high;
    }

    private static Pair<Result, String> handleError(String fmt, String tag, int arg1) {
        final String err = String.format(fmt, tag, arg1);
        CLog.w(err);
        return new Pair<Result, String>(Result.FAIL, err);
    }

    private static Pair<Result, String> handleError(
            String fmt, String tag, int arg1, int arg2, int arg3) {
        final String err = String.format(fmt, tag, arg1, arg2, arg3);
        CLog.w(err);
        return new Pair<Result, String>(Result.FAIL, err);
    }

    private static Pair<Result, String> handleError(
            String fmt, String tag, float arg1, float arg2, float arg3) {
        final String err = String.format(fmt, tag, arg1, arg2, arg3);
        CLog.w(err);
        return new Pair<Result, String>(Result.FAIL, err);
    }
}
