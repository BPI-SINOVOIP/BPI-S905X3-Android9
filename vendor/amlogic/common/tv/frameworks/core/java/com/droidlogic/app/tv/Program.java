/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.content.ContentValues;
import android.database.Cursor;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.text.TextUtils;
import android.util.Log;

import java.util.Arrays;
import java.util.Objects;
import java.sql.Blob;
import java.sql.SQLException;

/**
 * A convenience class to create and insert program information into the database.
 */
public final class Program implements Comparable<Program> {
    private static final String TAG = "Program";
    private static final boolean DEBUG = false;
    public static final long INVALID_LONG_VALUE = -1;
    public static final int INVALID_INT_VALUE = -1;

    private long mId;
    private long mChannelId;
    private int mProgramId;
    private String mTitle;
    private String mEpisodeTitle;
    private int mSeasonNumber;
    private int mEpisodeNumber;
    private long mStartTimeUtcMillis;
    private long mEndTimeUtcMillis;
    private String mDescription;
    private String mLongDescription;
    private int mVideoWidth;
    private int mVideoHeight;
    private String mPosterArtUri;
    private String mThumbnailUri;
    private String[] mCanonicalGenres;
    private TvContentRating[] mContentRatings;
    private String mInternalProviderData;
    private boolean mIsAppointed;
    private int mScheduledRecordStatus;
    private String mVersion;
    private String mEitExt;

    public static final int RECORD_STATUS_NOT_STARTED = 0;
    public static final int RECORD_STATUS_APPOINTED = 1;
    public static final int RECORD_STATUS_IN_PROGRESS = 2;

    private Program() {
        mId = INVALID_LONG_VALUE;
        mChannelId = INVALID_LONG_VALUE;
        mProgramId = INVALID_INT_VALUE;
        mSeasonNumber = INVALID_INT_VALUE;
        mEpisodeNumber = INVALID_INT_VALUE;
        mStartTimeUtcMillis = INVALID_LONG_VALUE;
        mEndTimeUtcMillis = INVALID_LONG_VALUE;
        mVideoWidth = INVALID_INT_VALUE;
        mVideoHeight = INVALID_INT_VALUE;
        mIsAppointed = false;
        mScheduledRecordStatus = 0;
        mVersion = null;
        mEitExt = null;
    }

    public long getId() {
        return mId;
    }

    public long getProgramId() {
        return mProgramId;
    }

    public long getChannelId() {
        return mChannelId;
    }

    public String getTitle() {
        return mTitle;
    }

    public String getEpisodeTitle() {
        return mEpisodeTitle;
    }

    public int getSeasonNumber() {
        return mSeasonNumber;
    }

    public int getEpisodeNumber() {
        return mEpisodeNumber;
    }

    public long getStartTimeUtcMillis() {
        return mStartTimeUtcMillis;
    }

    public long getEndTimeUtcMillis() {
        return mEndTimeUtcMillis;
    }

    public String getDescription() {
        return mDescription;
    }

    public String getLongDescription() {
        return mLongDescription;
    }

    public int getVideoWidth() {
        return mVideoWidth;
    }

    public int getVideoHeight() {
        return mVideoHeight;
    }

    public String[] getCanonicalGenres() {
        return mCanonicalGenres;
    }

    public TvContentRating[] getContentRatings() {
        return mContentRatings;
    }

    public void setContentRatings(TvContentRating[] rating) {
        mContentRatings = rating;
    }

    public String getPosterArtUri() {
        return mPosterArtUri;
    }

    public String getThumbnailUri() {
        return mThumbnailUri;
    }

    public String getInternalProviderData() {
        return mInternalProviderData;
    }

    public void setDescription(String description) {
        this.mDescription = description;
        return;
    }

    public boolean isAppointed() {
        return mIsAppointed;
    }

    public int getScheduledRecordStatus() {
        return mScheduledRecordStatus;
    }

    public void setIsAppointed(boolean isAppointed) {
        mIsAppointed = isAppointed;
    }

    public void setScheduledRecordStatus(int scheduledStatus) {
        mScheduledRecordStatus = scheduledStatus;
    }

    public String getVersion() {
        return mVersion;
    }

    public void setVersion(String version) {
        mVersion = version;
    }

    public String getEitExt() {
        return mEitExt;
    }

    public void setEitExt(String ext) {
        mEitExt = ext;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mChannelId, mStartTimeUtcMillis, mEndTimeUtcMillis,
                mTitle, mEpisodeTitle, mDescription, mLongDescription, mVideoWidth, mVideoHeight,
                mPosterArtUri, mThumbnailUri, mContentRatings, mCanonicalGenres, mSeasonNumber,
                mEpisodeNumber);
    }

    @Override
    public boolean equals(Object other) {
        if (!(other instanceof Program)) {
            return false;
        }
        Program program = (Program) other;
        return mChannelId == program.mChannelId
                && mStartTimeUtcMillis == program.mStartTimeUtcMillis
                && mEndTimeUtcMillis == program.mEndTimeUtcMillis
                && Objects.equals(mTitle, program.mTitle)
                && Objects.equals(mEpisodeTitle, program.mEpisodeTitle)
                && Objects.equals(mDescription, program.mDescription)
                && Objects.equals(mLongDescription, program.mLongDescription)
                && mVideoWidth == program.mVideoWidth
                && mVideoHeight == program.mVideoHeight
                && Objects.equals(mPosterArtUri, program.mPosterArtUri)
                && Objects.equals(mThumbnailUri, program.mThumbnailUri)
                && Arrays.equals(mContentRatings, program.mContentRatings)
                && Arrays.equals(mCanonicalGenres, program.mCanonicalGenres)
                && mSeasonNumber == program.mSeasonNumber
                && mEpisodeNumber == program.mEpisodeNumber
                && TextUtils.equals(mVersion, program.mVersion)
                && TextUtils.equals(mEitExt, program.mEitExt);
    }

    public boolean matchsWithoutDescription(Program program) {
         return mChannelId == program.mChannelId
                && mStartTimeUtcMillis == program.mStartTimeUtcMillis
                && mEndTimeUtcMillis == program.mEndTimeUtcMillis
                && Objects.equals(mTitle, program.mTitle)
                && Objects.equals(mEpisodeTitle, program.mEpisodeTitle)
                && mVideoWidth == program.mVideoWidth
                && mVideoHeight == program.mVideoHeight
                && Objects.equals(mPosterArtUri, program.mPosterArtUri)
                && Objects.equals(mThumbnailUri, program.mThumbnailUri)
                && Arrays.equals(mContentRatings, program.mContentRatings)
                && Arrays.equals(mCanonicalGenres, program.mCanonicalGenres)
                && mSeasonNumber == program.mSeasonNumber
                && mEpisodeNumber == program.mEpisodeNumber
                && Objects.equals(contentRatingsToString(mContentRatings),
                    contentRatingsToString(program.mContentRatings));
    }

    @Override
    public int compareTo(Program other) {
        return Long.compare(mStartTimeUtcMillis, other.mStartTimeUtcMillis);
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("Program{")
                .append("id=").append(mId)
                .append("programId=").append(mProgramId)
                .append(", channelId=").append(mChannelId)
                .append(", title=").append(mTitle)
                .append(", episodeTitle=").append(mEpisodeTitle)
                .append(", seasonNumber=").append(mSeasonNumber)
                .append(", episodeNumber=").append(mEpisodeNumber)
                .append(", startTimeUtcSec=").append(mStartTimeUtcMillis/1000)
                .append(", endTimeUtcSec=").append(mEndTimeUtcMillis/1000)
                .append(", videoWidth=").append(mVideoWidth)
                .append(", videoHeight=").append(mVideoHeight)
                .append(", contentRatings=").append(mContentRatings)
                .append(", posterArtUri=").append(mPosterArtUri)
                .append(", thumbnailUri=").append(mThumbnailUri)
                .append(", contentRatings=").append(mContentRatings)
                .append(", genres=").append(mCanonicalGenres)
                .append(", isAppointed=").append(mIsAppointed)
                .append(", scheduledRecordStatus=").append(mScheduledRecordStatus)
                .append(", ext=").append(mVersion).append(":"+mEitExt);
        return builder.append("}").toString();
    }

    public void copyFrom(Program other) {
        if (this == other) {
            return;
        }

        mProgramId = other.mProgramId;
        mChannelId = other.mChannelId;
        mTitle = other.mTitle;
        mEpisodeTitle = other.mEpisodeTitle;
        mSeasonNumber = other.mSeasonNumber;
        mEpisodeNumber = other.mEpisodeNumber;
        mStartTimeUtcMillis = other.mStartTimeUtcMillis;
        mEndTimeUtcMillis = other.mEndTimeUtcMillis;
        mDescription = other.mDescription;
        mLongDescription = other.mLongDescription;
        mVideoWidth = other.mVideoWidth;
        mVideoHeight = other.mVideoHeight;
        mPosterArtUri = other.mPosterArtUri;
        mThumbnailUri = other.mThumbnailUri;
        mCanonicalGenres = other.mCanonicalGenres;
        mContentRatings = other.mContentRatings;
        mIsAppointed = other.mIsAppointed;
        mScheduledRecordStatus = other.mScheduledRecordStatus;
        mVersion = other.mVersion;
        mEitExt = other.mEitExt;
    }

    public ContentValues toContentValues() {
        ContentValues values = new ContentValues();
        if (mChannelId != INVALID_LONG_VALUE) {
            values.put(TvContract.Programs.COLUMN_CHANNEL_ID, mChannelId);
        } else {
            values.putNull(TvContract.Programs.COLUMN_CHANNEL_ID);
        }
        if (!TextUtils.isEmpty(mTitle)) {
            values.put(TvContract.Programs.COLUMN_TITLE, mTitle);
        } else {
            values.putNull(TvContract.Programs.COLUMN_TITLE);
        }
        if (!TextUtils.isEmpty(mEpisodeTitle)) {
            values.put(TvContract.Programs.COLUMN_EPISODE_TITLE, mEpisodeTitle);
        } else {
            values.putNull(TvContract.Programs.COLUMN_EPISODE_TITLE);
        }
        if (mSeasonNumber != INVALID_INT_VALUE) {
            values.put(TvContract.Programs.COLUMN_SEASON_NUMBER, mSeasonNumber);
        } else {
            values.putNull(TvContract.Programs.COLUMN_SEASON_NUMBER);
        }
        if (mEpisodeNumber != INVALID_INT_VALUE) {
            values.put(TvContract.Programs.COLUMN_EPISODE_NUMBER, mEpisodeNumber);
        } else {
            values.putNull(TvContract.Programs.COLUMN_EPISODE_NUMBER);
        }
        if (!TextUtils.isEmpty(mDescription)) {
            values.put(TvContract.Programs.COLUMN_SHORT_DESCRIPTION, mDescription);
        } else {
            values.putNull(TvContract.Programs.COLUMN_SHORT_DESCRIPTION);
        }
        if (!TextUtils.isEmpty(mPosterArtUri)) {
            values.put(TvContract.Programs.COLUMN_POSTER_ART_URI, mPosterArtUri);
        } else {
            values.putNull(TvContract.Programs.COLUMN_POSTER_ART_URI);
        }
        if (!TextUtils.isEmpty(mThumbnailUri)) {
            values.put(TvContract.Programs.COLUMN_THUMBNAIL_URI, mThumbnailUri);
        } else {
            values.putNull(TvContract.Programs.COLUMN_THUMBNAIL_URI);
        }
        if (mCanonicalGenres != null && mCanonicalGenres.length > 0) {
            values.put(TvContract.Programs.COLUMN_CANONICAL_GENRE,
                    TvContract.Programs.Genres.encode(mCanonicalGenres));
        } else {
            values.putNull(TvContract.Programs.COLUMN_CANONICAL_GENRE);
        }
        if (mContentRatings != null && mContentRatings.length > 0) {
            values.put(TvContract.Programs.COLUMN_CONTENT_RATING,
                    contentRatingsToString(mContentRatings));
        } else {
            values.putNull(TvContract.Programs.COLUMN_CONTENT_RATING);
        }
        if (mStartTimeUtcMillis != INVALID_LONG_VALUE) {
            values.put(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS, mStartTimeUtcMillis);
        } else {
            values.putNull(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS);
        }
        if (mEndTimeUtcMillis != INVALID_LONG_VALUE) {
            values.put(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS, mEndTimeUtcMillis);
        } else {
            values.putNull(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS);
        }
        if (mVideoWidth != INVALID_INT_VALUE) {
            values.put(TvContract.Programs.COLUMN_VIDEO_WIDTH, mVideoWidth);
        } else {
            values.putNull(TvContract.Programs.COLUMN_VIDEO_WIDTH);
        }
        if (mVideoHeight != INVALID_INT_VALUE) {
            values.put(TvContract.Programs.COLUMN_VIDEO_HEIGHT, mVideoHeight);
        } else {
            values.putNull(TvContract.Programs.COLUMN_VIDEO_HEIGHT);
        }
        if (!TextUtils.isEmpty(mInternalProviderData)) {
            values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA, mInternalProviderData.getBytes());
        } else {
            values.putNull(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA);
        }
        values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG1, mIsAppointed ? 1 : 0);
        values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG2, mProgramId);
        values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG4, mScheduledRecordStatus);
        if (!TextUtils.isEmpty(mVersion)) {
            values.put(TvContract.Programs.COLUMN_VERSION_NUMBER, mVersion);
        } else {
            values.putNull(TvContract.Programs.COLUMN_VERSION_NUMBER);
        }
        if (!TextUtils.isEmpty(mEitExt)) {
            values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG3, mEitExt);
        } else {
            values.putNull(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG3);
        }
        return values;
    }

    public static Program fromCursor(Cursor cursor) {
        Builder builder = new Builder();
        int index = cursor.getColumnIndex(TvContract.Programs._ID);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setId(cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_CHANNEL_ID);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setChannelId(cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_TITLE);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setTitle(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_EPISODE_TITLE);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setEpisodeTitle(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_SEASON_NUMBER);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setSeasonNumber(cursor.getInt(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_EPISODE_NUMBER);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setEpisodeNumber(cursor.getInt(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_SHORT_DESCRIPTION);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setDescription(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_LONG_DESCRIPTION);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setLongDescription(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_POSTER_ART_URI);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setPosterArtUri(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_THUMBNAIL_URI);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setThumbnailUri(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_CANONICAL_GENRE);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setCanonicalGenres(TvContract.Programs.Genres.decode(cursor.getString(index)));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_CONTENT_RATING);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setContentRatings(stringToContentRatings(cursor.getString(
                    index)));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setStartTimeUtcMillis(cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setEndTimeUtcMillis(cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_VIDEO_WIDTH);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setVideoWidth((int) cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_VIDEO_HEIGHT);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setVideoHeight((int) cursor.getLong(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA);
        if (index >= 0 && !cursor.isNull(index)) {
            String strValue = null;
            if (cursor.getType(index) == Cursor.FIELD_TYPE_STRING) {
                strValue = cursor.getString(index);
            } else if (cursor.getType(index) == Cursor.FIELD_TYPE_BLOB) {
                byte[] blobValue = cursor.getBlob(index);
                if (blobValue != null) {
                    try {
                        strValue = new String(blobValue);
                    } catch (Exception e) {
                        // TODO Auto-generated catch block
                        if (DEBUG) Log.e(TAG, "blob to String Exception = " + e.getMessage());
                        e.printStackTrace();
                    }
                }
                if (DEBUG) Log.d(TAG, "blob to String = " + strValue);
            } else {
                if (DEBUG) Log.d(TAG, "COLUMN_INTERNAL_PROVIDER_DATA other type = " + cursor.getType(index));
            }
            builder.setInternalProviderData(strValue);
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG1);
        if (index >= 0) {
            builder.setIsAppointed(cursor.getInt(index) == 1 ? true : false);
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG2);
        if (index >= 0) {
            builder.setProgramId(cursor.getInt(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG4);
        if (index >= 0) {
            builder.setScheduledRecordStatus(cursor.getInt(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_VERSION_NUMBER);
        if (index >= 0 && !cursor.isNull(index)) {
            builder.setVersion(cursor.getString(index));
        }
        index = cursor.getColumnIndex(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_FLAG3);
        if (index >=0 && !cursor.isNull(index)) {
            builder.setEitExt(cursor.getString(index));
        }
        return builder.build();
    }

    public static final class Builder {
        private final Program mProgram;

        public Builder() {
            mProgram = new Program();
        }

        public Builder(Program other) {
            mProgram = new Program();
            mProgram.copyFrom(other);
        }

        public Builder setId(long id) {
            mProgram.mId = id;
            return this;
        }

        public Builder setProgramId(int programId) {
            mProgram.mProgramId = programId;
            return this;
        }

        public Builder setChannelId(long channelId) {
            mProgram.mChannelId = channelId;
            return this;
        }

        public Builder setTitle(String title) {
            mProgram.mTitle = title;
            return this;
        }

        public Builder setEpisodeTitle(String episodeTitle) {
            mProgram.mEpisodeTitle = episodeTitle;
            return this;
        }

        public Builder setSeasonNumber(int seasonNumber) {
            mProgram.mSeasonNumber = seasonNumber;
            return this;
        }

        public Builder setEpisodeNumber(int episodeNumber) {
            mProgram.mEpisodeNumber = episodeNumber;
            return this;
        }

        public Builder setStartTimeUtcMillis(long startTimeUtcMillis) {
            mProgram.mStartTimeUtcMillis = startTimeUtcMillis;
            return this;
        }

        public Builder setEndTimeUtcMillis(long endTimeUtcMillis) {
            mProgram.mEndTimeUtcMillis = endTimeUtcMillis;
            return this;
        }

        public Builder setDescription(String description) {
            mProgram.mDescription = description;
            return this;
        }

        public Builder setLongDescription(String longDescription) {
            mProgram.mLongDescription = longDescription;
            return this;
        }

        public Builder setVideoWidth(int width) {
            mProgram.mVideoWidth = width;
            return this;
        }

        public Builder setVideoHeight(int height) {
            mProgram.mVideoHeight = height;
            return this;
        }

        public Builder setContentRatings(TvContentRating[] contentRatings) {
            mProgram.mContentRatings = contentRatings;
            return this;
        }

        public Builder setPosterArtUri(String posterArtUri) {
            mProgram.mPosterArtUri = posterArtUri;
            return this;
        }

        public Builder setThumbnailUri(String thumbnailUri) {
            mProgram.mThumbnailUri = thumbnailUri;
            return this;
        }

        public Builder setCanonicalGenres(String[] genres) {
            mProgram.mCanonicalGenres = genres;
            return this;
        }

        public Builder setInternalProviderData(String data) {
            mProgram.mInternalProviderData = data;
            return this;
        }

        public Builder setIsAppointed(boolean isAppointed) {
            mProgram.mIsAppointed = isAppointed;
            return this;
        }

        public Builder setScheduledRecordStatus(int recordStatus) {
            mProgram.mScheduledRecordStatus = recordStatus;
            return this;
        }

        public Builder setVersion(String version) {
            mProgram.mVersion = version;
            return this;
        }

        public Builder setEitExt(String ext) {
            mProgram.mEitExt = ext;
            return this;
        }

        public Program build() {
            return mProgram;
        }
    }

    public static TvContentRating[] stringToContentRatings(String commaSeparatedRatings) {
        if (TextUtils.isEmpty(commaSeparatedRatings)) {
            return null;
        }
        String[] ratings = commaSeparatedRatings.split("\\s*,\\s*");
        TvContentRating[] contentRatings = new TvContentRating[ratings.length];
        for (int i = 0; i < contentRatings.length; ++i) {
            contentRatings[i] = TvContentRating.unflattenFromString(ratings[i]);
        }
        return contentRatings;
    }

    public static String contentRatingsToString(TvContentRating[] contentRatings) {
        if (contentRatings == null || contentRatings.length == 0) {
            return null;
        }
        final String DELIMITER = ",";
        StringBuilder ratings = new StringBuilder(contentRatings[0].flattenToString());
        for (int i = 1; i < contentRatings.length; ++i) {
            ratings.append(DELIMITER);
            ratings.append(contentRatings[i].flattenToString());
        }
        return ratings.toString();
    }
}
