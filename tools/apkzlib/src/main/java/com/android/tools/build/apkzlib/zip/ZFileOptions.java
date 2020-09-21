/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tools.build.apkzlib.zip;

import com.android.tools.build.apkzlib.zip.compress.DeflateExecutionCompressor;
import com.android.tools.build.apkzlib.zip.utils.ByteTracker;
import java.util.function.Supplier;
import java.util.zip.Deflater;
import javax.annotation.Nonnull;

/**
 * Options to create a {@link ZFile}.
 */
public class ZFileOptions {

    /**
     * The byte tracker.
     */
    @Nonnull
    private ByteTracker tracker;

    /**
     * The compressor to use.
     */
    @Nonnull
    private Compressor compressor;

    /**
     * Should timestamps be zeroed?
     */
    private boolean noTimestamps;

    /**
     * The alignment rule to use.
     */
    @Nonnull
    private AlignmentRule alignmentRule;

    /**
     * Should the extra field be used to cover empty space?
     */
    private boolean coverEmptySpaceUsingExtraField;

    /**
     * Should files be automatically sorted before update?
     */
    private boolean autoSortFiles;

    /**
     * Factory creating verification logs to use.
     */
    @Nonnull
    private Supplier<VerifyLog> verifyLogFactory;

    /**
     * Creates a new options object. All options are set to their defaults.
     */
    public ZFileOptions() {
        tracker = new ByteTracker();
        compressor =
                new DeflateExecutionCompressor(
                        Runnable::run,
                        tracker,
                        Deflater.DEFAULT_COMPRESSION);
        alignmentRule = AlignmentRules.compose();
        verifyLogFactory = VerifyLogs::devNull;
    }

    /**
     * Obtains the ZFile's byte tracker.
     *
     * @return the byte tracker
     */
    @Nonnull
    public ByteTracker getTracker() {
        return tracker;
    }

    /**
     * Obtains the compressor to use.
     *
     * @return the compressor
     */
    @Nonnull
    public Compressor getCompressor() {
        return compressor;
    }

    /**
     * Sets the compressor to use.
     *
     * @param compressor the compressor
     */
    public ZFileOptions setCompressor(@Nonnull Compressor compressor) {
        this.compressor = compressor;
        return this;
    }

    /**
     * Obtains whether timestamps should be zeroed.
     *
     * @return should timestamps be zeroed?
     */
    public boolean getNoTimestamps() {
        return noTimestamps;
    }

    /**
     * Sets whether timestamps should be zeroed.
     *
     * @param noTimestamps should timestamps be zeroed?
     */
    public ZFileOptions setNoTimestamps(boolean noTimestamps) {
        this.noTimestamps = noTimestamps;
        return this;
    }

    /**
     * Obtains the alignment rule.
     *
     * @return the alignment rule
     */
    @Nonnull
    public AlignmentRule getAlignmentRule() {
        return alignmentRule;
    }

    /**
     * Sets the alignment rule.
     *
     * @param alignmentRule the alignment rule
     */
    public ZFileOptions setAlignmentRule(@Nonnull AlignmentRule alignmentRule) {
        this.alignmentRule = alignmentRule;
        return this;
    }

    /**
     * Obtains whether the extra field should be used to cover empty spaces. See {@link ZFile} for
     * an explanation on using the extra field for covering empty spaces.
     *
     * @return should the extra field be used to cover empty spaces?
     */
    public boolean getCoverEmptySpaceUsingExtraField() {
        return coverEmptySpaceUsingExtraField;
    }

    /**
     * Sets whether the extra field should be used to cover empty spaces. See {@link ZFile} for an
     * explanation on using the extra field for covering empty spaces.
     *
     * @param coverEmptySpaceUsingExtraField should the extra field be used to cover empty spaces?
     */
    public ZFileOptions setCoverEmptySpaceUsingExtraField(boolean coverEmptySpaceUsingExtraField) {
        this.coverEmptySpaceUsingExtraField = coverEmptySpaceUsingExtraField;
        return this;
    }

    /**
     * Obtains whether files should be automatically sorted before updating the zip file. See
     * {@link ZFile} for an explanation on automatic sorting.
     *
     * @return should the file be automatically sorted?
     */
    public boolean getAutoSortFiles() {
        return autoSortFiles;
    }

    /**
     * Sets whether files should be automatically sorted before updating the zip file. See {@link
     * ZFile} for an explanation on automatic sorting.
     *
     * @param autoSortFiles should the file be automatically sorted?
     */
    public ZFileOptions setAutoSortFiles(boolean autoSortFiles) {
        this.autoSortFiles = autoSortFiles;
        return this;
    }

    /**
     * Sets the verification log factory.
     *
     * @param verifyLogFactory verification log factory
     */
    public ZFileOptions setVerifyLogFactory(@Nonnull Supplier<VerifyLog> verifyLogFactory) {
        this.verifyLogFactory = verifyLogFactory;
        return this;
    }

    /**
     * Obtains the verification log factory. By default, the verification log doesn't store
     * anything and will always return an empty log.
     *
     * @return the verification log factory
     */
    @Nonnull
    public Supplier<VerifyLog> getVerifyLogFactory() {
        return verifyLogFactory;
    }
}
