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

package com.android.tv.dvr.data;

import android.annotation.TargetApi;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.IntDef;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Range;
import com.android.tv.R;
import com.android.tv.TvSingletons;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.android.tv.dvr.DvrScheduleManager;
import com.android.tv.dvr.provider.DvrContract.Schedules;
import com.android.tv.util.CompositeComparator;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Collection;
import java.util.Comparator;
import java.util.Objects;

/** A data class for one recording contents. */
@TargetApi(Build.VERSION_CODES.N)
@SuppressWarnings("AndroidApiChecker") // TODO(b/32513850) remove when error prone is updated
public final class ScheduledRecording implements Parcelable {
    private static final String TAG = "ScheduledRecording";

    /** Indicates that the ID is not assigned yet. */
    public static final long ID_NOT_SET = 0;

    /** The default priority of the recording. */
    public static final long DEFAULT_PRIORITY = Long.MAX_VALUE >> 1;

    /** Compares the start time in ascending order. */
    public static final Comparator<ScheduledRecording> START_TIME_COMPARATOR =
            new Comparator<ScheduledRecording>() {
                @Override
                public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
                    return Long.compare(lhs.mStartTimeMs, rhs.mStartTimeMs);
                }
            };

    /** Compares the end time in ascending order. */
    public static final Comparator<ScheduledRecording> END_TIME_COMPARATOR =
            new Comparator<ScheduledRecording>() {
                @Override
                public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
                    return Long.compare(lhs.mEndTimeMs, rhs.mEndTimeMs);
                }
            };

    /** Compares ID in ascending order. The schedule with the larger ID was created later. */
    public static final Comparator<ScheduledRecording> ID_COMPARATOR =
            new Comparator<ScheduledRecording>() {
                @Override
                public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
                    return Long.compare(lhs.mId, rhs.mId);
                }
            };

    /** Compares the priority in ascending order. */
    public static final Comparator<ScheduledRecording> PRIORITY_COMPARATOR =
            new Comparator<ScheduledRecording>() {
                @Override
                public int compare(ScheduledRecording lhs, ScheduledRecording rhs) {
                    return Long.compare(lhs.mPriority, rhs.mPriority);
                }
            };

    /**
     * Compares start time in ascending order and then priority in descending order and then ID in
     * descending order.
     */
    public static final Comparator<ScheduledRecording> START_TIME_THEN_PRIORITY_THEN_ID_COMPARATOR =
            new CompositeComparator<>(
                    START_TIME_COMPARATOR,
                    PRIORITY_COMPARATOR.reversed(),
                    ID_COMPARATOR.reversed());

    /** Builds scheduled recordings from programs. */
    public static Builder builder(String inputId, Program p) {
        return new Builder()
                .setInputId(inputId)
                .setChannelId(p.getChannelId())
                .setStartTimeMs(p.getStartTimeUtcMillis())
                .setEndTimeMs(p.getEndTimeUtcMillis())
                .setProgramId(p.getId())
                .setProgramTitle(p.getTitle())
                .setSeasonNumber(p.getSeasonNumber())
                .setEpisodeNumber(p.getEpisodeNumber())
                .setEpisodeTitle(p.getEpisodeTitle())
                .setProgramDescription(p.getDescription())
                .setProgramLongDescription(p.getLongDescription())
                .setProgramPosterArtUri(p.getPosterArtUri())
                .setProgramThumbnailUri(p.getThumbnailUri())
                .setType(TYPE_PROGRAM);
    }

    /** Builds appointed watch from programs. */
    public static Builder builder(Program p) {
        return new Builder()
                .setChannelId(p.getChannelId())
                .setStartTimeMs(p.getStartTimeUtcMillis())
                .setEndTimeMs(p.getEndTimeUtcMillis())
                .setProgramId(p.getId())
                .setProgramTitle(p.getTitle())
                .setSeasonNumber(p.getSeasonNumber())
                .setEpisodeNumber(p.getEpisodeNumber())
                .setEpisodeTitle(p.getEpisodeTitle())
                .setProgramDescription(p.getDescription())
                .setProgramLongDescription(p.getLongDescription())
                .setProgramPosterArtUri(p.getPosterArtUri())
                .setProgramThumbnailUri(p.getThumbnailUri())
                .setType(TYPE_APPOINT);
    }

    public static Builder builder(String inputId, long channelId, long startTime, long endTime) {
        return new Builder()
                .setInputId(inputId)
                .setChannelId(channelId)
                .setStartTimeMs(startTime)
                .setEndTimeMs(endTime)
                .setType(TYPE_TIMED);
    }

    /** Creates a new Builder with the values set from the {@link RecordedProgram}. */
    public static Builder builder(RecordedProgram p) {
        boolean isProgramRecording = !TextUtils.isEmpty(p.getTitle());
        return new Builder()
                .setInputId(p.getInputId())
                .setChannelId(p.getChannelId())
                .setType(isProgramRecording ? TYPE_PROGRAM : TYPE_TIMED)
                .setStartTimeMs(p.getStartTimeUtcMillis())
                .setEndTimeMs(p.getEndTimeUtcMillis())
                .setProgramTitle(p.getTitle())
                .setSeasonNumber(p.getSeasonNumber())
                .setEpisodeNumber(p.getEpisodeNumber())
                .setEpisodeTitle(p.getEpisodeTitle())
                .setProgramDescription(p.getDescription())
                .setProgramLongDescription(p.getLongDescription())
                .setProgramPosterArtUri(p.getPosterArtUri())
                .setProgramThumbnailUri(p.getThumbnailUri())
                .setState(STATE_RECORDING_FINISHED)
                .setRecordedProgramId(p.getId());
    }

    public static final class Builder {
        private long mId = ID_NOT_SET;
        private long mPriority = DvrScheduleManager.DEFAULT_PRIORITY;
        private String mInputId;
        private long mChannelId;
        private long mProgramId = ID_NOT_SET;
        private String mProgramTitle;
        private @RecordingType int mType;
        private long mStartTimeMs;
        private long mEndTimeMs;
        private String mSeasonNumber;
        private String mEpisodeNumber;
        private String mEpisodeTitle;
        private String mProgramDescription;
        private String mProgramLongDescription;
        private String mProgramPosterArtUri;
        private String mProgramThumbnailUri;
        private @RecordingState int mState;
        private long mSeriesRecordingId = ID_NOT_SET;
        private Long mRecodedProgramId;
        private Integer mFailedReason;

        private Builder() {}

        public Builder setId(long id) {
            mId = id;
            return this;
        }

        public Builder setPriority(long priority) {
            mPriority = priority;
            return this;
        }

        public Builder setInputId(String inputId) {
            mInputId = inputId;
            return this;
        }

        public Builder setChannelId(long channelId) {
            mChannelId = channelId;
            return this;
        }

        public Builder setProgramId(long programId) {
            mProgramId = programId;
            return this;
        }

        public Builder setProgramTitle(String programTitle) {
            mProgramTitle = programTitle;
            return this;
        }

        private Builder setType(@RecordingType int type) {
            mType = type;
            return this;
        }

        public Builder setStartTimeMs(long startTimeMs) {
            mStartTimeMs = startTimeMs;
            return this;
        }

        public Builder setEndTimeMs(long endTimeMs) {
            mEndTimeMs = endTimeMs;
            return this;
        }

        public Builder setSeasonNumber(String seasonNumber) {
            mSeasonNumber = seasonNumber;
            return this;
        }

        public Builder setEpisodeNumber(String episodeNumber) {
            mEpisodeNumber = episodeNumber;
            return this;
        }

        public Builder setEpisodeTitle(String episodeTitle) {
            mEpisodeTitle = episodeTitle;
            return this;
        }

        public Builder setProgramDescription(String description) {
            mProgramDescription = description;
            return this;
        }

        public Builder setProgramLongDescription(String longDescription) {
            mProgramLongDescription = longDescription;
            return this;
        }

        public Builder setProgramPosterArtUri(String programPosterArtUri) {
            mProgramPosterArtUri = programPosterArtUri;
            return this;
        }

        public Builder setProgramThumbnailUri(String programThumbnailUri) {
            mProgramThumbnailUri = programThumbnailUri;
            return this;
        }

        public Builder setState(@RecordingState int state) {
            mState = state;
            return this;
        }

        public Builder setSeriesRecordingId(long seriesRecordingId) {
            mSeriesRecordingId = seriesRecordingId;
            return this;
        }

        public Builder setRecordedProgramId(Long recordedProgramId) {
            mRecodedProgramId = recordedProgramId;
            return this;
        }

        public Builder setFailedReason(Integer reason) {
            mFailedReason = reason;
            return this;
        }

        public ScheduledRecording build() {
            return new ScheduledRecording(
                    mId,
                    mPriority,
                    mInputId,
                    mChannelId,
                    mProgramId,
                    mProgramTitle,
                    mType,
                    mStartTimeMs,
                    mEndTimeMs,
                    mSeasonNumber,
                    mEpisodeNumber,
                    mEpisodeTitle,
                    mProgramDescription,
                    mProgramLongDescription,
                    mProgramPosterArtUri,
                    mProgramThumbnailUri,
                    mState,
                    mSeriesRecordingId,
                    mRecodedProgramId,
                    mFailedReason);
        }
    }

    /** Creates {@link Builder} object from the given original {@code Recording}. */
    public static Builder buildFrom(ScheduledRecording orig) {
        return new Builder()
                .setId(orig.mId)
                .setInputId(orig.mInputId)
                .setChannelId(orig.mChannelId)
                .setEndTimeMs(orig.mEndTimeMs)
                .setSeriesRecordingId(orig.mSeriesRecordingId)
                .setPriority(orig.mPriority)
                .setProgramId(orig.mProgramId)
                .setProgramTitle(orig.mProgramTitle)
                .setStartTimeMs(orig.mStartTimeMs)
                .setSeasonNumber(orig.getSeasonNumber())
                .setEpisodeNumber(orig.getEpisodeNumber())
                .setEpisodeTitle(orig.getEpisodeTitle())
                .setProgramDescription(orig.getProgramDescription())
                .setProgramLongDescription(orig.getProgramLongDescription())
                .setProgramPosterArtUri(orig.getProgramPosterArtUri())
                .setProgramThumbnailUri(orig.getProgramThumbnailUri())
                .setState(orig.mState)
                .setFailedReason(orig.getFailedReason())
                .setType(orig.mType)
                .setRecordedProgramId(orig.getRecordedProgramId());
    }

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        STATE_RECORDING_NOT_STARTED,
        STATE_RECORDING_IN_PROGRESS,
        STATE_RECORDING_FINISHED,
        STATE_RECORDING_FAILED,
        STATE_RECORDING_CLIPPED,
        STATE_RECORDING_DELETED,
        STATE_RECORDING_CANCELED
    })
    public @interface RecordingState {}

    public static final int STATE_RECORDING_NOT_STARTED = 0;
    public static final int STATE_RECORDING_IN_PROGRESS = 1;
    public static final int STATE_RECORDING_FINISHED = 2;
    public static final int STATE_RECORDING_FAILED = 3;
    public static final int STATE_RECORDING_CLIPPED = 4;
    public static final int STATE_RECORDING_DELETED = 5;
    public static final int STATE_RECORDING_CANCELED = 6;

    /** The reasons of failed recordings */
    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        FAILED_REASON_OTHER,
        FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED,
        FAILED_REASON_NOT_FINISHED,
        FAILED_REASON_SCHEDULER_STOPPED,
        FAILED_REASON_INVALID_CHANNEL,
        FAILED_REASON_MESSAGE_NOT_SENT,
        FAILED_REASON_CONNECTION_FAILED,
        FAILED_REASON_RESOURCE_BUSY,
        FAILED_REASON_INPUT_UNAVAILABLE,
        FAILED_REASON_INPUT_DVR_UNSUPPORTED,
        FAILED_REASON_INSUFFICIENT_SPACE
    })
    public @interface RecordingFailedReason {}

    public static final int FAILED_REASON_OTHER = 0;
    public static final int FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED = 1;
    public static final int FAILED_REASON_NOT_FINISHED = 2;
    public static final int FAILED_REASON_SCHEDULER_STOPPED = 3;
    public static final int FAILED_REASON_INVALID_CHANNEL = 4;
    public static final int FAILED_REASON_MESSAGE_NOT_SENT = 5;
    public static final int FAILED_REASON_CONNECTION_FAILED = 6;
    public static final int FAILED_REASON_RESOURCE_BUSY = 7;
    // For the following reasons, show advice to users
    public static final int FAILED_REASON_INPUT_UNAVAILABLE = 8;
    public static final int FAILED_REASON_INPUT_DVR_UNSUPPORTED = 9;
    public static final int FAILED_REASON_INSUFFICIENT_SPACE = 10;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({TYPE_TIMED, TYPE_PROGRAM, TYPE_APPOINT})
    public @interface RecordingType {}
    /** Record with given time range. */
    public static final int TYPE_TIMED = 1;
    /** Record with a given program. */
    public static final int TYPE_PROGRAM = 2;
    /** appoint watch with a given program. */
    public static final int TYPE_APPOINT = 3;

    @RecordingType private final int mType;

    /**
     * Use this projection if you want to create {@link ScheduledRecording} object using {@link
     * #fromCursor}.
     */
    public static final String[] PROJECTION = {
        // Columns must match what is read in #fromCursor
        Schedules._ID,
        Schedules.COLUMN_PRIORITY,
        Schedules.COLUMN_TYPE,
        Schedules.COLUMN_INPUT_ID,
        Schedules.COLUMN_CHANNEL_ID,
        Schedules.COLUMN_PROGRAM_ID,
        Schedules.COLUMN_PROGRAM_TITLE,
        Schedules.COLUMN_START_TIME_UTC_MILLIS,
        Schedules.COLUMN_END_TIME_UTC_MILLIS,
        Schedules.COLUMN_SEASON_NUMBER,
        Schedules.COLUMN_EPISODE_NUMBER,
        Schedules.COLUMN_EPISODE_TITLE,
        Schedules.COLUMN_PROGRAM_DESCRIPTION,
        Schedules.COLUMN_PROGRAM_LONG_DESCRIPTION,
        Schedules.COLUMN_PROGRAM_POST_ART_URI,
        Schedules.COLUMN_PROGRAM_THUMBNAIL_URI,
        Schedules.COLUMN_STATE,
        Schedules.COLUMN_FAILED_REASON,
        Schedules.COLUMN_SERIES_RECORDING_ID
    };

    /** Creates {@link ScheduledRecording} object from the given {@link Cursor}. */
    public static ScheduledRecording fromCursor(Cursor c) {
        int index = -1;
        return new Builder()
                .setId(c.getLong(++index))
                .setPriority(c.getLong(++index))
                .setType(recordingType(c.getString(++index)))
                .setInputId(c.getString(++index))
                .setChannelId(c.getLong(++index))
                .setProgramId(c.getLong(++index))
                .setProgramTitle(c.getString(++index))
                .setStartTimeMs(c.getLong(++index))
                .setEndTimeMs(c.getLong(++index))
                .setSeasonNumber(c.getString(++index))
                .setEpisodeNumber(c.getString(++index))
                .setEpisodeTitle(c.getString(++index))
                .setProgramDescription(c.getString(++index))
                .setProgramLongDescription(c.getString(++index))
                .setProgramPosterArtUri(c.getString(++index))
                .setProgramThumbnailUri(c.getString(++index))
                .setState(recordingState(c.getString(++index)))
                .setFailedReason(recordingFailedReason(c.getString(++index)))
                .setSeriesRecordingId(c.getLong(++index))
                .build();
    }

    public static ContentValues toContentValues(ScheduledRecording r) {
        ContentValues values = new ContentValues();
        if (r.getId() != ID_NOT_SET) {
            values.put(Schedules._ID, r.getId());
        }
        values.put(Schedules.COLUMN_INPUT_ID, r.getInputId());
        values.put(Schedules.COLUMN_CHANNEL_ID, r.getChannelId());
        values.put(Schedules.COLUMN_PROGRAM_ID, r.getProgramId());
        values.put(Schedules.COLUMN_PROGRAM_TITLE, r.getProgramTitle());
        values.put(Schedules.COLUMN_PRIORITY, r.getPriority());
        values.put(Schedules.COLUMN_START_TIME_UTC_MILLIS, r.getStartTimeMs());
        values.put(Schedules.COLUMN_END_TIME_UTC_MILLIS, r.getEndTimeMs());
        values.put(Schedules.COLUMN_SEASON_NUMBER, r.getSeasonNumber());
        values.put(Schedules.COLUMN_EPISODE_NUMBER, r.getEpisodeNumber());
        values.put(Schedules.COLUMN_EPISODE_TITLE, r.getEpisodeTitle());
        values.put(Schedules.COLUMN_PROGRAM_DESCRIPTION, r.getProgramDescription());
        values.put(Schedules.COLUMN_PROGRAM_LONG_DESCRIPTION, r.getProgramLongDescription());
        values.put(Schedules.COLUMN_PROGRAM_POST_ART_URI, r.getProgramPosterArtUri());
        values.put(Schedules.COLUMN_PROGRAM_THUMBNAIL_URI, r.getProgramThumbnailUri());
        values.put(Schedules.COLUMN_STATE, recordingState(r.getState()));
        values.put(Schedules.COLUMN_FAILED_REASON, recordingFailedReason(r.getFailedReason()));
        values.put(Schedules.COLUMN_TYPE, recordingType(r.getType()));
        if (r.getSeriesRecordingId() != ID_NOT_SET) {
            values.put(Schedules.COLUMN_SERIES_RECORDING_ID, r.getSeriesRecordingId());
        } else {
            values.putNull(Schedules.COLUMN_SERIES_RECORDING_ID);
        }
        return values;
    }

    public static ScheduledRecording fromParcel(Parcel in) {
        return new Builder()
                .setId(in.readLong())
                .setPriority(in.readLong())
                .setInputId(in.readString())
                .setChannelId(in.readLong())
                .setProgramId(in.readLong())
                .setProgramTitle(in.readString())
                .setType(in.readInt())
                .setStartTimeMs(in.readLong())
                .setEndTimeMs(in.readLong())
                .setSeasonNumber(in.readString())
                .setEpisodeNumber(in.readString())
                .setEpisodeTitle(in.readString())
                .setProgramDescription(in.readString())
                .setProgramLongDescription(in.readString())
                .setProgramPosterArtUri(in.readString())
                .setProgramThumbnailUri(in.readString())
                .setState(in.readInt())
                .setFailedReason(recordingFailedReason(in.readString()))
                .setSeriesRecordingId(in.readLong())
                .build();
    }

    public static final Parcelable.Creator<ScheduledRecording> CREATOR =
            new Parcelable.Creator<ScheduledRecording>() {
                @Override
                public ScheduledRecording createFromParcel(Parcel in) {
                    return ScheduledRecording.fromParcel(in);
                }

                @Override
                public ScheduledRecording[] newArray(int size) {
                    return new ScheduledRecording[size];
                }
            };

    /** The ID internal to Live TV */
    private long mId;

    /**
     * The priority of this recording.
     *
     * <p>The highest number is recorded first. If there is a tie in priority then the higher id
     * wins.
     */
    private final long mPriority;

    private final String mInputId;
    private final long mChannelId;
    /** Optional id of the associated program. */
    private final long mProgramId;

    private final String mProgramTitle;

    private final long mStartTimeMs;
    private final long mEndTimeMs;
    private final String mSeasonNumber;
    private final String mEpisodeNumber;
    private final String mEpisodeTitle;
    private final String mProgramDescription;
    private final String mProgramLongDescription;
    private final String mProgramPosterArtUri;
    private final String mProgramThumbnailUri;
    @RecordingState private final int mState;
    private final long mSeriesRecordingId;
    private final Long mRecordedProgramId;
    private final Integer mFailedReason;

    private ScheduledRecording(
            long id,
            long priority,
            String inputId,
            long channelId,
            long programId,
            String programTitle,
            @RecordingType int type,
            long startTime,
            long endTime,
            String seasonNumber,
            String episodeNumber,
            String episodeTitle,
            String programDescription,
            String programLongDescription,
            String programPosterArtUri,
            String programThumbnailUri,
            @RecordingState int state,
            long seriesRecordingId,
            Long recordedProgramId,
            Integer failedReason) {
        mId = id;
        mPriority = priority;
        mInputId = inputId;
        mChannelId = channelId;
        mProgramId = programId;
        mProgramTitle = programTitle;
        mType = type;
        mStartTimeMs = startTime;
        mEndTimeMs = endTime;
        mSeasonNumber = seasonNumber;
        mEpisodeNumber = episodeNumber;
        mEpisodeTitle = episodeTitle;
        mProgramDescription = programDescription;
        mProgramLongDescription = programLongDescription;
        mProgramPosterArtUri = programPosterArtUri;
        mProgramThumbnailUri = programThumbnailUri;
        mState = state;
        mSeriesRecordingId = seriesRecordingId;
        mRecordedProgramId = recordedProgramId;
        mFailedReason = failedReason;
    }

    /**
     * Returns recording schedule type. The possible types are {@link #TYPE_PROGRAM} and {@link
     * #TYPE_TIMED}.
     */
    @RecordingType
    public int getType() {
        return mType;
    }

    /** Returns schedules' input id. */
    public String getInputId() {
        return mInputId;
    }

    /** Returns recorded {@link Channel}. */
    public long getChannelId() {
        return mChannelId;
    }

    /** Return the optional program id */
    public long getProgramId() {
        return mProgramId;
    }

    /** Return the optional program Title */
    public String getProgramTitle() {
        return mProgramTitle;
    }

    /** Returns started time. */
    public long getStartTimeMs() {
        return mStartTimeMs;
    }

    /** Returns ended time. */
    public long getEndTimeMs() {
        return mEndTimeMs;
    }

    /** Returns the season number. */
    public String getSeasonNumber() {
        return mSeasonNumber;
    }

    /** Returns the episode number. */
    public String getEpisodeNumber() {
        return mEpisodeNumber;
    }

    /** Returns the episode title. */
    public String getEpisodeTitle() {
        return mEpisodeTitle;
    }

    /** Returns the description of program. */
    public String getProgramDescription() {
        return mProgramDescription;
    }

    /** Returns the long description of program. */
    public String getProgramLongDescription() {
        return mProgramLongDescription;
    }

    /** Returns the poster uri of program. */
    public String getProgramPosterArtUri() {
        return mProgramPosterArtUri;
    }

    /** Returns the thumb nail uri of program. */
    public String getProgramThumbnailUri() {
        return mProgramThumbnailUri;
    }

    /** Returns duration. */
    public long getDuration() {
        return mEndTimeMs - mStartTimeMs;
    }

    /**
     * Returns the state. The possible states are {@link #STATE_RECORDING_NOT_STARTED}, {@link
     * #STATE_RECORDING_IN_PROGRESS}, {@link #STATE_RECORDING_FINISHED}, {@link
     * #STATE_RECORDING_FAILED}, {@link #STATE_RECORDING_CLIPPED} and {@link
     * #STATE_RECORDING_DELETED}.
     */
    @RecordingState
    public int getState() {
        return mState;
    }

    /** Returns the ID of the {@link SeriesRecording} including this schedule. */
    public long getSeriesRecordingId() {
        return mSeriesRecordingId;
    }

    /** Returns the ID of the corresponding {@link RecordedProgram}. */
    @Nullable
    public Long getRecordedProgramId() {
        return mRecordedProgramId;
    }

    /** Returns the failed reason of the {@link ScheduledRecording}. */
    @Nullable @RecordingFailedReason
    public Integer getFailedReason() {
        return mFailedReason;
    }

    public long getId() {
        return mId;
    }

    /** Sets the ID; */
    public void setId(long id) {
        mId = id;
    }

    public long getPriority() {
        return mPriority;
    }

    /** Returns season number, episode number and episode title for display. */
    public String getEpisodeDisplayTitle(Context context) {
        if (!TextUtils.isEmpty(mEpisodeNumber)) {
            String episodeTitle = mEpisodeTitle == null ? "" : mEpisodeTitle;
            if (TextUtils.equals(mSeasonNumber, "0")) {
                // Do not show "S0: ".
                return String.format(
                        context.getResources()
                                .getString(R.string.display_episode_title_format_no_season_number),
                        mEpisodeNumber,
                        episodeTitle);
            } else {
                return String.format(
                        context.getResources().getString(R.string.display_episode_title_format),
                        mSeasonNumber,
                        mEpisodeNumber,
                        episodeTitle);
            }
        }
        return mEpisodeTitle;
    }

    /**
     * Returns the program's display title, if the program title is not null, returns program title.
     * Otherwise returns the channel name.
     */
    public String getProgramDisplayTitle(Context context) {
        if (!TextUtils.isEmpty(mProgramTitle)) {
            return mProgramTitle;
        }
        Channel channel =
                TvSingletons.getSingletons(context).getChannelDataManager().getChannel(mChannelId);
        return channel != null
                ? channel.getDisplayName()
                : context.getString(R.string.no_program_information);
    }

    /** Converts a string to a @RecordingType int, defaulting to {@link #TYPE_TIMED}. */
    private static @RecordingType int recordingType(String type) {
        switch (type) {
            case Schedules.TYPE_TIMED:
                return TYPE_TIMED;
            case Schedules.TYPE_PROGRAM:
                return TYPE_PROGRAM;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording type %s", type);
                return TYPE_TIMED;
        }
    }

    /** Converts a @RecordingType int to a string, defaulting to {@link Schedules#TYPE_TIMED}. */
    private static String recordingType(@RecordingType int type) {
        switch (type) {
            case TYPE_TIMED:
                return Schedules.TYPE_TIMED;
            case TYPE_PROGRAM:
                return Schedules.TYPE_PROGRAM;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording type %s", type);
                return Schedules.TYPE_TIMED;
        }
    }

    /**
     * Converts a string to a @RecordingState int, defaulting to {@link
     * #STATE_RECORDING_NOT_STARTED}.
     */
    private static @RecordingState int recordingState(String state) {
        switch (state) {
            case Schedules.STATE_RECORDING_NOT_STARTED:
                return STATE_RECORDING_NOT_STARTED;
            case Schedules.STATE_RECORDING_IN_PROGRESS:
                return STATE_RECORDING_IN_PROGRESS;
            case Schedules.STATE_RECORDING_FINISHED:
                return STATE_RECORDING_FINISHED;
            case Schedules.STATE_RECORDING_FAILED:
                return STATE_RECORDING_FAILED;
            case Schedules.STATE_RECORDING_CLIPPED:
                return STATE_RECORDING_CLIPPED;
            case Schedules.STATE_RECORDING_DELETED:
                return STATE_RECORDING_DELETED;
            case Schedules.STATE_RECORDING_CANCELED:
                return STATE_RECORDING_CANCELED;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording state %s", state);
                return STATE_RECORDING_NOT_STARTED;
        }
    }

    /**
     * Converts a @RecordingState int to string, defaulting to {@link
     * Schedules#STATE_RECORDING_NOT_STARTED}.
     */
    private static String recordingState(@RecordingState int state) {
        switch (state) {
            case STATE_RECORDING_NOT_STARTED:
                return Schedules.STATE_RECORDING_NOT_STARTED;
            case STATE_RECORDING_IN_PROGRESS:
                return Schedules.STATE_RECORDING_IN_PROGRESS;
            case STATE_RECORDING_FINISHED:
                return Schedules.STATE_RECORDING_FINISHED;
            case STATE_RECORDING_FAILED:
                return Schedules.STATE_RECORDING_FAILED;
            case STATE_RECORDING_CLIPPED:
                return Schedules.STATE_RECORDING_CLIPPED;
            case STATE_RECORDING_DELETED:
                return Schedules.STATE_RECORDING_DELETED;
            case STATE_RECORDING_CANCELED:
                return Schedules.STATE_RECORDING_CANCELED;
            default:
                SoftPreconditions.checkArgument(false, TAG, "Unknown recording state %s", state);
                return Schedules.STATE_RECORDING_NOT_STARTED;
        }
    }

    /**
     * Converts a string to a failed reason integer, defaulting to {@link
     * #FAILED_REASON_OTHER}.
     */
    private static Integer recordingFailedReason(String reason) {
        if (TextUtils.isEmpty(reason)) {
            return null;
        }
        switch (reason) {
            case Schedules.FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED:
                return FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED;
            case Schedules.FAILED_REASON_NOT_FINISHED:
                return FAILED_REASON_NOT_FINISHED;
            case Schedules.FAILED_REASON_SCHEDULER_STOPPED:
                return FAILED_REASON_SCHEDULER_STOPPED;
            case Schedules.FAILED_REASON_INVALID_CHANNEL:
                return FAILED_REASON_INVALID_CHANNEL;
            case Schedules.FAILED_REASON_MESSAGE_NOT_SENT:
                return FAILED_REASON_MESSAGE_NOT_SENT;
            case Schedules.FAILED_REASON_CONNECTION_FAILED:
                return FAILED_REASON_CONNECTION_FAILED;
            case Schedules.FAILED_REASON_RESOURCE_BUSY:
                return FAILED_REASON_RESOURCE_BUSY;
            case Schedules.FAILED_REASON_INPUT_UNAVAILABLE:
                return FAILED_REASON_INPUT_UNAVAILABLE;
            case Schedules.FAILED_REASON_INPUT_DVR_UNSUPPORTED:
                return FAILED_REASON_INPUT_DVR_UNSUPPORTED;
            case Schedules.FAILED_REASON_INSUFFICIENT_SPACE:
                return FAILED_REASON_INSUFFICIENT_SPACE;
            case Schedules.FAILED_REASON_OTHER:
            default:
                return FAILED_REASON_OTHER;
        }
    }

    /**
     * Converts a failed reason integer to string, defaulting to {@link
     * Schedules#FAILED_REASON_OTHER}.
     */
    private static String recordingFailedReason(Integer reason) {
        if (reason == null) {
            return null;
        }
        switch (reason) {
            case FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED:
                return Schedules.FAILED_REASON_PROGRAM_ENDED_BEFORE_RECORDING_STARTED;
            case FAILED_REASON_NOT_FINISHED:
                return Schedules.FAILED_REASON_NOT_FINISHED;
            case FAILED_REASON_SCHEDULER_STOPPED:
                return Schedules.FAILED_REASON_SCHEDULER_STOPPED;
            case FAILED_REASON_INVALID_CHANNEL:
                return Schedules.FAILED_REASON_INVALID_CHANNEL;
            case FAILED_REASON_MESSAGE_NOT_SENT:
                return Schedules.FAILED_REASON_MESSAGE_NOT_SENT;
            case FAILED_REASON_CONNECTION_FAILED:
                return Schedules.FAILED_REASON_CONNECTION_FAILED;
            case FAILED_REASON_RESOURCE_BUSY:
                return Schedules.FAILED_REASON_RESOURCE_BUSY;
            case FAILED_REASON_INPUT_UNAVAILABLE:
                return Schedules.FAILED_REASON_INPUT_UNAVAILABLE;
            case FAILED_REASON_INPUT_DVR_UNSUPPORTED:
                return Schedules.FAILED_REASON_INPUT_DVR_UNSUPPORTED;
            case FAILED_REASON_INSUFFICIENT_SPACE:
                return Schedules.FAILED_REASON_INSUFFICIENT_SPACE;
            case FAILED_REASON_OTHER: // fall through
            default:
                return Schedules.FAILED_REASON_OTHER;
        }
    }

    /** Checks if the {@code period} overlaps with the recording time. */
    public boolean isOverLapping(Range<Long> period) {
        return mStartTimeMs < period.getUpper() && mEndTimeMs > period.getLower();
    }

    /** Checks if the {@code schedule} overlaps with this schedule. */
    public boolean isOverLapping(ScheduledRecording schedule) {
        return mStartTimeMs < schedule.getEndTimeMs() && mEndTimeMs > schedule.getStartTimeMs();
    }

    @Override
    public String toString() {
        return "ScheduledRecording["
                + mId
                + "]"
                + "(inputId="
                + mInputId
                + ",channelId="
                + mChannelId
                + ",programId="
                + mProgramId
                + ",programTitle="
                + mProgramTitle
                + ",type="
                + mType
                + ",startTime="
                + CommonUtils.toIsoDateTimeString(mStartTimeMs)
                + "("
                + mStartTimeMs
                + ")"
                + ",endTime="
                + CommonUtils.toIsoDateTimeString(mEndTimeMs)
                + "("
                + mEndTimeMs
                + ")"
                + ",seasonNumber="
                + mSeasonNumber
                + ",episodeNumber="
                + mEpisodeNumber
                + ",episodeTitle="
                + mEpisodeTitle
                + ",programDescription="
                + mProgramDescription
                + ",programLongDescription="
                + mProgramLongDescription
                + ",programPosterArtUri="
                + mProgramPosterArtUri
                + ",programThumbnailUri="
                + mProgramThumbnailUri
                + ",state="
                + mState
                + ",failedReason="
                + mFailedReason
                + ",priority="
                + mPriority
                + ",seriesRecordingId="
                + mSeriesRecordingId
                + ",RecordedProgramId="
                + mRecordedProgramId
                + ")";
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int paramInt) {
        out.writeLong(mId);
        out.writeLong(mPriority);
        out.writeString(mInputId);
        out.writeLong(mChannelId);
        out.writeLong(mProgramId);
        out.writeString(mProgramTitle);
        out.writeInt(mType);
        out.writeLong(mStartTimeMs);
        out.writeLong(mEndTimeMs);
        out.writeString(mSeasonNumber);
        out.writeString(mEpisodeNumber);
        out.writeString(mEpisodeTitle);
        out.writeString(mProgramDescription);
        out.writeString(mProgramLongDescription);
        out.writeString(mProgramPosterArtUri);
        out.writeString(mProgramThumbnailUri);
        out.writeInt(mState);
        out.writeString(recordingFailedReason(mFailedReason));
        out.writeLong(mSeriesRecordingId);
    }

    /** Returns {@code true} if the recording is not started yet, otherwise @{code false}. */
    public boolean isNotStarted() {
        return mState == STATE_RECORDING_NOT_STARTED;
    }

    /** Returns {@code true} if the recording is in progress, otherwise @{code false}. */
    public boolean isInProgress() {
        return mState == STATE_RECORDING_IN_PROGRESS;
    }

    /** Returns {@code true} if the recording is finished, otherwise @{code false}. */
    public boolean isFinished() {
        return mState == STATE_RECORDING_FINISHED;
    }

    /** Returns {@code true} if the recording is appointed watch, otherwise @{code false}. */
    public boolean isAppointedWatchProgram() {
        return mType == TYPE_APPOINT;
    }

    @Override
    public boolean equals(Object obj) {
        if (!(obj instanceof ScheduledRecording)) {
            return false;
        }
        ScheduledRecording r = (ScheduledRecording) obj;
        return mId == r.mId
                && mPriority == r.mPriority
                && mChannelId == r.mChannelId
                && mProgramId == r.mProgramId
                && Objects.equals(mProgramTitle, r.mProgramTitle)
                && mType == r.mType
                && mStartTimeMs == r.mStartTimeMs
                && mEndTimeMs == r.mEndTimeMs
                && Objects.equals(mSeasonNumber, r.mSeasonNumber)
                && Objects.equals(mEpisodeNumber, r.mEpisodeNumber)
                && Objects.equals(mEpisodeTitle, r.mEpisodeTitle)
                && Objects.equals(mProgramDescription, r.getProgramDescription())
                && Objects.equals(mProgramLongDescription, r.getProgramLongDescription())
                && Objects.equals(mProgramPosterArtUri, r.getProgramPosterArtUri())
                && Objects.equals(mProgramThumbnailUri, r.getProgramThumbnailUri())
                && mState == r.mState
                && Objects.equals(mFailedReason, r.mFailedReason)
                && mSeriesRecordingId == r.mSeriesRecordingId;
    }

    @Override
    public int hashCode() {
        return Objects.hash(
                mId,
                mPriority,
                mChannelId,
                mProgramId,
                mProgramTitle,
                mType,
                mStartTimeMs,
                mEndTimeMs,
                mSeasonNumber,
                mEpisodeNumber,
                mEpisodeTitle,
                mProgramDescription,
                mProgramLongDescription,
                mProgramPosterArtUri,
                mProgramThumbnailUri,
                mState,
                mFailedReason,
                mSeriesRecordingId);
    }

    /** Returns an array containing all of the elements in the list. */
    public static ScheduledRecording[] toArray(Collection<ScheduledRecording> schedules) {
        return schedules.toArray(new ScheduledRecording[schedules.size()]);
    }
}
