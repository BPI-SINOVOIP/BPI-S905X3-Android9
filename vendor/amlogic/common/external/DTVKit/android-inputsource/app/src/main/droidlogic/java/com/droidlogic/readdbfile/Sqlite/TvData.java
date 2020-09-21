package com.droidlogic.readdbfile.Sqlite;

import android.content.ContentProviderOperation;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

public class TvData {
    private final static String TAG = TvData.class.getSimpleName();

    private static final char DOUBLE_QUOTE = '"';
    private static final char COMMA = ',';
    private static final String DELIMITER = ",";

    //channel
    public static final String PATH_CHANNEL = "channels";
    public static final String[] CHANNEL_BASE_COLUMNS = new String[] {
            TvContract.Channels._ID,
            TvContract.Channels.COLUMN_DESCRIPTION,
            TvContract.Channels.COLUMN_DISPLAY_NAME,
            TvContract.Channels.COLUMN_DISPLAY_NUMBER,
            TvContract.Channels.COLUMN_INPUT_ID,
            TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA,
            TvContract.Channels.COLUMN_NETWORK_AFFILIATION,
            TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID,
            TvContract.Channels.COLUMN_PACKAGE_NAME,
            TvContract.Channels.COLUMN_SEARCHABLE,
            TvContract.Channels.COLUMN_SERVICE_ID,
            TvContract.Channels.COLUMN_SERVICE_TYPE,
            TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID,
            TvContract.Channels.COLUMN_TYPE,
            TvContract.Channels.COLUMN_VIDEO_FORMAT,
    };

    //program
    public static final String PATH_PROGRAM = "programs";
    public static final String[] PROGRAM_BASE_COLUMNS = new String[] {
            TvContract.Programs._ID,
            TvContract.Programs.COLUMN_CHANNEL_ID,
            TvContract.Programs.COLUMN_TITLE,
            TvContract.Programs.COLUMN_EPISODE_TITLE,
            TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER,
            TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER,
            TvContract.Programs.COLUMN_SHORT_DESCRIPTION,
            TvContract.Programs.COLUMN_LONG_DESCRIPTION,
            TvContract.Programs.COLUMN_POSTER_ART_URI,
            TvContract.Programs.COLUMN_THUMBNAIL_URI,
            TvContract.Programs.COLUMN_AUDIO_LANGUAGE,
            TvContract.Programs.COLUMN_BROADCAST_GENRE,
            TvContract.Programs.COLUMN_CANONICAL_GENRE,
            TvContract.Programs.COLUMN_CONTENT_RATING,
            TvContract.Programs.COLUMN_START_TIME_UTC_MILLIS,
            TvContract.Programs.COLUMN_END_TIME_UTC_MILLIS,
            TvContract.Programs.COLUMN_VIDEO_WIDTH,
            TvContract.Programs.COLUMN_VIDEO_HEIGHT,
            TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA
    };

    private Context mContext;

    public TvData(Context context) {
        mContext = context;
    }

    public void syncTvChannels(List<TvChannel> channels) {
        if (mContext != null) {
            ArrayList<ContentProviderOperation> ops = new ArrayList<>();
            for (TvChannel channel : channels) {
                Log.d(TAG, "syncTvChannels " + channel.mDisplayNumber + " " + channel.mPackageName);
                ops.add(ContentProviderOperation.newInsert(
                        TvContract.Channels.CONTENT_URI)
                        .withValues(channel.toContentValues())
                        .build());
                if (ops.size() >= 500) {
                    try {
                        mContext.getContentResolver().applyBatch(TvContract.AUTHORITY, ops);
                    } catch (Exception e) {
                        Log.e(TAG, "syncTvChannel Failed1 = " + e.getMessage());
                    }
                    ops.clear();
                }
            }
            if (ops.size() > 0) {
                try {
                    mContext.getContentResolver().applyBatch(TvContract.AUTHORITY, ops);
                } catch (Exception e) {
                    Log.e(TAG, "syncTvChannel Failed2 = " + e.getMessage());
                }
            }
        }
    }

    public void deleteTvChannels() {
        if (mContext != null) {
            int num = mContext.getContentResolver().delete(TvContract.Channels.CONTENT_URI, null/*TvContract.Channels._ID + "!=-1"*/, null);
            Log.d(TAG, "deleteTvChannels  num = " + num);
        }
    }

    public void syncTvPrograms(List<TvProgram> programs) {
        if (mContext != null) {
            ArrayList<ContentProviderOperation> ops = new ArrayList<>();
            for (TvProgram program : programs) {
                ops.add(ContentProviderOperation.newInsert(
                        TvContract.Programs.CONTENT_URI)
                        .withValues(program.toContentValues())
                        .build());
                if (ops.size() >= 500) {
                    try {
                        mContext.getContentResolver().applyBatch(TvContract.AUTHORITY, ops);
                    } catch (Exception e) {
                        Log.e(TAG, "syncTvProgram Failed1 = " + e.getMessage());
                    }
                    ops.clear();
                }
            }
            if (ops.size() > 0) {
                try {
                    mContext.getContentResolver().applyBatch(TvContract.AUTHORITY, ops);
                } catch (Exception e) {
                    Log.e(TAG, "syncTvChannel Failed2 = " + e.getMessage());
                }
            }
        }
    }

    public void deleteTvPrograms() {
        if (mContext != null) {
            int num = mContext.getContentResolver().delete(TvContract.Programs.CONTENT_URI, null/*TvContract.Programs._ID + "!=-1"*/, null);
            Log.d(TAG, "deleteTvPrograms  num = " + num);
        }
    }

    public TvChannel getTvChannel() {
        return new TvChannel();
    }

    public TvProgram getTvProgram() {
        return new TvProgram();
    }

    /**
     * Flattens an array of {@link TvContentRating} into a String to be inserted into a database.
     *
     * @param contentRatings An array of TvContentRatings.
     * @return A comma-separated String of ratings.
     */
    public  String contentRatingsToString(TvContentRating[] contentRatings) {
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

    /**
     * Parses a string of comma-separated ratings into an array of {@link TvContentRating}.
     *
     * @param commaSeparatedRatings String containing various ratings, separated by commas.
     * @return An array of TvContentRatings.
     */
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

    /**
     * Encodes genre strings to a text that can be put into the database.
     *
     * @param genres Genre strings.
     * @return an encoded genre string that can be inserted into the
     *         {@link # TvContract.Programs.COLUMN_BROADCAST_GENRE} or {@link # TvContract.Programs.COLUMN_CANONICAL_GENRE} column.
     */
    public static String encode(String... genres) {
        if (genres == null) {
            // MNC and before will throw a NPE.
            return null;
        }
        StringBuilder sb = new StringBuilder();
        String separator = "";
        for (String genre : genres) {
            sb.append(separator).append(encodeToCsv(genre));
            separator = DELIMITER;
        }
        return sb.toString();
    }

    private static String encodeToCsv(String genre) {
        StringBuilder sb = new StringBuilder();
        int length = genre.length();
        for (int i = 0; i < length; ++i) {
            char c = genre.charAt(i);
            switch (c) {
                case DOUBLE_QUOTE:
                    sb.append(DOUBLE_QUOTE);
                    break;
                case COMMA:
                    sb.append(DOUBLE_QUOTE);
                    break;
            }
            sb.append(c);
        }
        return sb.toString();
    }

    public class TvChannel {
        private static final long INVALID_CHANNEL_ID = -1;
        private static final int INVALID_INTEGER_VALUE = -1;
        private static final int IS_SEARCHABLE = 1;

        private long mId;
        private String mPackageName;
        private String mInputId;
        private String mType;
        private String mDisplayNumber;
        private String mDisplayName;
        private String mDescription;
        private String mChannelLogo;
        private String mVideoFormat;
        private int mOriginalNetworkId;
        private int mTransportStreamId;
        private int mServiceId;
        private String mAppLinkText;
        private int mAppLinkColor;
        private String mAppLinkIconUri;
        private String mAppLinkPosterArtUri;
        private String mAppLinkIntentUri;
        private byte[] mInternalProviderData;
        private String mNetworkAffiliation;
        private int mSearchable;
        private String mServiceType;

        public TvChannel() {

        }

        public TvChannel fromCursor(Cursor cursor) {
            Builder builder = new Builder();
            int index = 0;
            if (!cursor.isNull(index)) {
                builder.setId(cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setDescription(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setDisplayName(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setDisplayNumber(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setInputId(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setInternalProviderData(cursor.getBlob(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setNetworkAffiliation(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setOriginalNetworkId(cursor.getInt(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setPackageName(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setSearchable(cursor.getInt(index) == IS_SEARCHABLE);
            }
            if (!cursor.isNull(++index)) {
                builder.setServiceId(cursor.getInt(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setServiceType(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setTransportStreamId(cursor.getInt(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setType(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setVideoFormat(cursor.getString(index));
            }
            return builder.build();
        }

        private void copyFrom(TvChannel other) {
            if (this == other) {
                return;
            }
            mId = other.mId;
            mPackageName = other.mPackageName;
            mInputId = other.mInputId;
            mType = other.mType;
            mDisplayNumber = other.mDisplayNumber;
            mDisplayName = other.mDisplayName;
            mDescription = other.mDescription;
            mVideoFormat = other.mVideoFormat;
            mOriginalNetworkId = other.mOriginalNetworkId;
            mTransportStreamId = other.mTransportStreamId;
            mServiceId = other.mServiceId;
            mAppLinkText = other.mAppLinkText;
            mAppLinkColor = other.mAppLinkColor;
            mAppLinkIconUri = other.mAppLinkIconUri;
            mAppLinkPosterArtUri = other.mAppLinkPosterArtUri;
            mAppLinkIntentUri = other.mAppLinkIntentUri;
            mChannelLogo = other.mChannelLogo;
            mInternalProviderData = other.mInternalProviderData;
            mNetworkAffiliation = other.mNetworkAffiliation;
            mSearchable = other.mSearchable;
            mServiceType = other.mServiceType;
        }

        /**
         * @return The fields of the Channel in the ContentValues format to be easily inserted into the
         * TV Input Framework database.
         */
        public ContentValues toContentValues() {
            ContentValues values = new ContentValues();
            if (mId != INVALID_CHANNEL_ID) {
                values.put(TvContract.Channels._ID, mId);
            }
            if (!TextUtils.isEmpty(mPackageName)) {
                values.put(TvContract.Channels.COLUMN_PACKAGE_NAME, mPackageName);
            } else {
                values.putNull(TvContract.Channels.COLUMN_PACKAGE_NAME);
            }
            if (!TextUtils.isEmpty(mInputId)) {
                values.put(TvContract.Channels.COLUMN_INPUT_ID, mInputId);
            } else {
                values.putNull(TvContract.Channels.COLUMN_INPUT_ID);
            }
            if (!TextUtils.isEmpty(mType)) {
                values.put(TvContract.Channels.COLUMN_TYPE, mType);
            } else {
                values.putNull(TvContract.Channels.COLUMN_TYPE);
            }
            if (!TextUtils.isEmpty(mDisplayNumber)) {
                values.put(TvContract.Channels.COLUMN_DISPLAY_NUMBER, mDisplayNumber);
            } else {
                values.putNull(TvContract.Channels.COLUMN_DISPLAY_NUMBER);
            }
            if (!TextUtils.isEmpty(mDisplayName)) {
                values.put(TvContract.Channels.COLUMN_DISPLAY_NAME, mDisplayName);
            } else {
                values.putNull(TvContract.Channels.COLUMN_DISPLAY_NAME);
            }
            if (!TextUtils.isEmpty(mDescription)) {
                values.put(TvContract.Channels.COLUMN_DESCRIPTION, mDescription);
            } else {
                values.putNull(TvContract.Channels.COLUMN_DESCRIPTION);
            }
            if (!TextUtils.isEmpty(mVideoFormat)) {
                values.put(TvContract.Channels.COLUMN_VIDEO_FORMAT, mVideoFormat);
            } else {
                values.putNull(TvContract.Channels.COLUMN_VIDEO_FORMAT);
            }
            if (mInternalProviderData != null && mInternalProviderData.length > 0) {
                values.put(TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA, mInternalProviderData);
            } else {
                values.putNull(TvContract.Channels.COLUMN_INTERNAL_PROVIDER_DATA);
            }
            values.put(TvContract.Channels.COLUMN_ORIGINAL_NETWORK_ID, mOriginalNetworkId);
            values.put(TvContract.Channels.COLUMN_TRANSPORT_STREAM_ID, mTransportStreamId);
            values.put(TvContract.Channels.COLUMN_SERVICE_ID, mServiceId);
            values.put(TvContract.Channels.COLUMN_NETWORK_AFFILIATION, mNetworkAffiliation);
            values.put(TvContract.Channels.COLUMN_SEARCHABLE, mSearchable);
            values.put(TvContract.Channels.COLUMN_SERVICE_TYPE, mServiceType);
            return values;
        }

        public final class Builder {
            private final TvChannel mChannel;

            public Builder() {
                mChannel = new TvChannel();
            }

            /**
             * Sets the id of the Channel.
             *
             * @param id The value of {@link TvContract.Channels#_ID} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            private Builder setId(long id) {
                mChannel.mId = id;
                return this;
            }

            /**
             * Sets the package name of the Channel.
             *
             * @param packageName The value of {@link TvContract.Channels#COLUMN_PACKAGE_NAME} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setPackageName(String packageName) {
                mChannel.mPackageName = packageName;
                return this;
            }

            /**
             * Sets the input id of the Channel.
             *
             * @param inputId The value of {@link TvContract.Channels#COLUMN_INPUT_ID} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setInputId(String inputId) {
                mChannel.mInputId = inputId;
                return this;
            }

            /**
             * Sets the broadcast standard of the Channel.
             *
             * @param type The value of {@link TvContract.Channels#COLUMN_TYPE} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setType(String type) {
                mChannel.mType = type;
                return this;
            }

            /**
             * Sets the display number of the Channel.
             *
             * @param displayNumber The value of {@link TvContract.Channels#COLUMN_DISPLAY_NUMBER} for
             * the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setDisplayNumber(String displayNumber) {
                mChannel.mDisplayNumber = displayNumber;
                return this;
            }

            /**
             * Sets the name to be displayed for the Channel.
             *
             * @param displayName The value of {@link TvContract.Channels#COLUMN_DISPLAY_NAME} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setDisplayName(String displayName) {
                mChannel.mDisplayName = displayName;
                return this;
            }

            /**
             * Sets the description of the Channel.
             *
             * @param description The value of {@link TvContract.Channels#COLUMN_DESCRIPTION} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setDescription(String description) {
                mChannel.mDescription = description;
                return this;
            }

            /**
             * Sets the logo of the channel.
             *
             * @param channelLogo The Uri corresponding to the logo for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             * @see TvContract.Channels.Logo
             */
            public Builder setChannelLogo(String channelLogo) {
                mChannel.mChannelLogo = channelLogo;
                return this;
            }

            /**
             * Sets the video format of the Channel.
             *
             * @param videoFormat The value of {@link TvContract.Channels#COLUMN_VIDEO_FORMAT} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setVideoFormat(String videoFormat) {
                mChannel.mVideoFormat = videoFormat;
                return this;
            }

            /**
             * Sets the original network id of the Channel.
             *
             * @param originalNetworkId The value of
             * {@link TvContract.Channels#COLUMN_ORIGINAL_NETWORK_ID} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setOriginalNetworkId(int originalNetworkId) {
                mChannel.mOriginalNetworkId = originalNetworkId;
                return this;
            }

            /**
             * Sets the transport stream id of the Channel.
             *
             * @param transportStreamId The value of
             * {@link TvContract.Channels#COLUMN_TRANSPORT_STREAM_ID} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setTransportStreamId(int transportStreamId) {
                mChannel.mTransportStreamId = transportStreamId;
                return this;
            }

            /**
             * Sets the service id of the Channel.
             *
             * @param serviceId The value of {@link TvContract.Channels#COLUMN_SERVICE_ID} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setServiceId(int serviceId) {
                mChannel.mServiceId = serviceId;
                return this;
            }

            /**
             * Sets the internal provider data of the channel.
             *
             * @param internalProviderData The value of
             * {@link TvContract.Channels#COLUMN_INTERNAL_PROVIDER_DATA} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setInternalProviderData(byte[] internalProviderData) {
                mChannel.mInternalProviderData = internalProviderData;
                return this;
            }

            /**
             * Sets the internal provider data of the channel.
             *
             * @param internalProviderData The value of
             * {@link TvContract.Channels#COLUMN_INTERNAL_PROVIDER_DATA} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setInternalProviderData(String internalProviderData) {
                mChannel.mInternalProviderData = internalProviderData.getBytes();
                return this;
            }

            /**
             * Sets the text to be displayed in the App Linking card.
             *
             * @param appLinkText The value of {@link TvContract.Channels#COLUMN_APP_LINK_TEXT} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkText(String appLinkText) {
                mChannel.mAppLinkText = appLinkText;
                return this;
            }

            /**
             * Sets the background color of the App Linking card.
             *
             * @param appLinkColor The value of {@link TvContract.Channels#COLUMN_APP_LINK_COLOR} for
             * the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkColor(int appLinkColor) {
                mChannel.mAppLinkColor = appLinkColor;
                return this;
            }

            /**
             * Sets the icon to be displayed next to the text of the App Linking card.
             *
             * @param appLinkIconUri The value of {@link TvContract.Channels#COLUMN_APP_LINK_ICON_URI}
             * for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkIconUri(String appLinkIconUri) {
                mChannel.mAppLinkIconUri = appLinkIconUri;
                return this;
            }

            /**
             * Sets the background image of the App Linking card.
             *
             * @param appLinkPosterArtUri The value of
             * {@link TvContract.Channels#COLUMN_APP_LINK_POSTER_ART_URI} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkPosterArtUri(String appLinkPosterArtUri) {
                mChannel.mAppLinkPosterArtUri = appLinkPosterArtUri;
                return this;
            }

            /**
             * Sets the App Linking Intent.
             *
             * @param appLinkIntent The Intent to be executed when the App Linking card is selected
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkIntent(Intent appLinkIntent) {
                return setAppLinkIntentUri(appLinkIntent.toUri(Intent.URI_INTENT_SCHEME));
            }

            /**
             * Sets the App Linking Intent.
             *
             * @param appLinkIntentUri The Intent that should be executed when the App Linking card is
             * selected. Use the method toUri(Intent.URI_INTENT_SCHEME) on your Intentto turn it into a
             * String. See {@link TvContract.Channels#COLUMN_APP_LINK_INTENT_URI}.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAppLinkIntentUri(String appLinkIntentUri) {
                mChannel.mAppLinkIntentUri = appLinkIntentUri;
                return this;
            }

            /**
             * Sets the network name for the channel, which may be different from its display name.
             *
             * @param networkAffiliation The value of
             * {@link TvContract.Channels#COLUMN_NETWORK_AFFILIATION} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setNetworkAffiliation(String networkAffiliation) {
                mChannel.mNetworkAffiliation = networkAffiliation;
                return this;
            }

            /**
             * Sets whether this channel can be searched for in other applications.
             *
             * @param searchable The value of
             * {@link TvContract.Channels#COLUMN_SEARCHABLE} for the channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setSearchable(boolean searchable) {
                mChannel.mSearchable = searchable ? IS_SEARCHABLE : 0;
                return this;
            }

            /**
             * Sets the type of content that will appear on this channel. This could refer to the
             * underlying broadcast standard or refer to {@link TvContract.Channels#SERVICE_TYPE_AUDIO},
             * {@link TvContract.Channels#SERVICE_TYPE_AUDIO_VIDEO}, or
             * {@link TvContract.Channels#SERVICE_TYPE_OTHER}.
             *
             * @param serviceType The value of {@link TvContract.Channels#COLUMN_SERVICE_TYPE} for the
             * channel.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setServiceType(String serviceType) {
                mChannel.mServiceType = serviceType;
                return this;
            }

            /**
             * Takes the values of the Builder object and creates a Channel object.
             * @return Channel object with values from the Builder.
             */
            public TvChannel build() {
                TvChannel channel = new TvChannel();
                channel.copyFrom(mChannel);
                if (channel.mOriginalNetworkId == INVALID_INTEGER_VALUE) {
                    throw new IllegalArgumentException("This channel must have a valid original " +
                            "network id");
                }
                return channel;
            }
        }
    }

    public class TvProgram {
        private static final long INVALID_LONG_VALUE = -1;
        private static final int INVALID_INT_VALUE = -1;
        private static final int IS_RECORDING_PROHIBITED = 1;
        private static final int IS_SEARCHABLE = 1;

        private long mId;
        private long mChannelId;
        private String mTitle;
        private String mEpisodeTitle;
        private String mSeasonNumber;
        private String mEpisodeNumber;
        private long mStartTimeUtcMillis;
        private long mEndTimeUtcMillis;
        private String mDescription;
        private String mLongDescription;
        private int mVideoWidth;
        private int mVideoHeight;
        private String mPosterArtUri;
        private String mThumbnailUri;
        private String[] mBroadcastGenres;
        private String[] mCanonicalGenres;
        private TvContentRating[] mContentRatings;
        private byte[] mInternalProviderData;
        private String mAudioLanguages;
        private int mRecordingProhibited;
        private int mSearchable;
        private String mSeasonTitle;

        public TvProgram() {

        }

        public void copyFrom(TvProgram other) {
            if (this == other) {
                return;
            }

            mId = other.mId;
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
            mBroadcastGenres = other.mBroadcastGenres;
            mCanonicalGenres = other.mCanonicalGenres;
            mContentRatings = other.mContentRatings;
            mAudioLanguages = other.mAudioLanguages;
            mRecordingProhibited = other.mRecordingProhibited;
            mSearchable = other.mSearchable;
            mSeasonTitle = other.mSeasonTitle;
            mInternalProviderData = other.mInternalProviderData;
        }

        /**
         * @return The fields of the Program in the ContentValues format to be easily inserted into the
         * TV Input Framework database.
         */
        public ContentValues toContentValues() {
            ContentValues values = new ContentValues();
            if (mId != INVALID_LONG_VALUE) {
                values.put(TvContract.Programs._ID, mId);
            }
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
            if (!TextUtils.isEmpty(mSeasonNumber)) {
                values.put(TvContract.Programs.COLUMN_SEASON_DISPLAY_NUMBER, mSeasonNumber);
            } else {
                values.putNull(TvContract.Programs.COLUMN_SEASON_NUMBER);
            }
            if (!TextUtils.isEmpty(mEpisodeNumber)) {
                values.put(TvContract.Programs.COLUMN_EPISODE_DISPLAY_NUMBER, mEpisodeNumber);
            }  else {
                values.putNull(TvContract.Programs.COLUMN_EPISODE_NUMBER);
            }
            if (!TextUtils.isEmpty(mDescription)) {
                values.put(TvContract.Programs.COLUMN_SHORT_DESCRIPTION, mDescription);
            } else {
                values.putNull(TvContract.Programs.COLUMN_SHORT_DESCRIPTION);
            }
            if (!TextUtils.isEmpty(mDescription)) {
                values.put(TvContract.Programs.COLUMN_LONG_DESCRIPTION, mLongDescription);
            } else {
                values.putNull(TvContract.Programs.COLUMN_LONG_DESCRIPTION);
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
            if (!TextUtils.isEmpty(mAudioLanguages)) {
                values.put(TvContract.Programs.COLUMN_AUDIO_LANGUAGE, mAudioLanguages);
            } else {
                values.putNull(TvContract.Programs.COLUMN_AUDIO_LANGUAGE);
            }
            if (mBroadcastGenres != null && mBroadcastGenres.length > 0) {
                values.put(TvContract.Programs.COLUMN_BROADCAST_GENRE,
                        TvContract.Programs.Genres.encode(mBroadcastGenres));
            } else {
                values.putNull(TvContract.Programs.COLUMN_BROADCAST_GENRE);
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
            if (mInternalProviderData != null && mInternalProviderData.length > 0) {
                values.put(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA, mInternalProviderData);
            } else {
                values.putNull(TvContract.Programs.COLUMN_INTERNAL_PROVIDER_DATA);
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                values.put(TvContract.Programs.COLUMN_SEARCHABLE, mSearchable);
            }
            if (!TextUtils.isEmpty(mSeasonTitle) && Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                values.put(TvContract.Programs.COLUMN_SEASON_TITLE, mSeasonTitle);
            } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                values.putNull(TvContract.Programs.COLUMN_SEASON_TITLE);
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                values.put(TvContract.Programs.COLUMN_RECORDING_PROHIBITED, mRecordingProhibited);
            }
            return values;
        }

        /**
         * Creates a Program object from a cursor including the fields defined in
         * {@link TvContract.Programs}.
         *
         * @param cursor A row from the TV Input Framework database.
         * @return A Program with the values taken from the cursor.
         * @hide
         */
        public TvProgram fromCursor(Cursor cursor) {
            Builder builder = new Builder();
            int index = 0;
            if (!cursor.isNull(index)) {
                builder.setId(cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setChannelId(cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setTitle(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setEpisodeTitle(cursor.getString(index));
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                if (!cursor.isNull(++index)) {
                    builder.setSeasonNumber(cursor.getString(index), INVALID_INT_VALUE);
                }
            } else {
                if (!cursor.isNull(++index)) {
                    builder.setSeasonNumber(cursor.getInt(index));
                }
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                if (!cursor.isNull(++index)) {
                    builder.setEpisodeNumber(cursor.getString(index), INVALID_INT_VALUE);
                }
            } else {
                if (!cursor.isNull(++index)) {
                    builder.setEpisodeNumber(cursor.getInt(index));
                }
            }
            if (!cursor.isNull(++index)) {
                builder.setDescription(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setLongDescription(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setPosterArtUri(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setThumbnailUri(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setAudioLanguages(cursor.getString(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setBroadcastGenres(TvContract.Programs.Genres.decode(cursor.getString(index)));
            }
            if (!cursor.isNull(++index)) {
                builder.setCanonicalGenres(TvContract.Programs.Genres.decode(cursor.getString(index)));
            }
            if (!cursor.isNull(++index)) {
                builder.setContentRatings(
                        stringToContentRatings(cursor.getString(index)));
            }
            if (!cursor.isNull(++index)) {
                builder.setStartTimeUtcMillis(cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setEndTimeUtcMillis(cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setVideoWidth((int) cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setVideoHeight((int) cursor.getLong(index));
            }
            if (!cursor.isNull(++index)) {
                builder.setInternalProviderData(cursor.getBlob(index));
            }
            return builder.build();
        }

        /**
         * This Builder class simplifies the creation of a {@link TvProgram} object.
         */
        public final class Builder {
            private final TvProgram mProgram;

            /**
             * Creates a new Builder object.
             */
            public Builder() {
                mProgram = new TvProgram();
            }

            /**
             * Sets a unique id for this program.
             *
             * @param programId The value of {@link TvContract.Programs#_ID} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            private Builder setId(long programId) {
                mProgram.mId = programId;
                return this;
            }

            /**
             * Sets the ID of the {@link TvChannel} that contains this program.
             *
             * @param channelId The value of {@link TvContract.Programs#COLUMN_CHANNEL_ID for the
             * program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setChannelId(long channelId) {
                mProgram.mChannelId = channelId;
                return this;
            }

            /**
             * Sets the title of this program. For a series, this is the series title.
             *
             * @param title The value of {@link TvContract.Programs#COLUMN_TITLE} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setTitle(String title) {
                mProgram.mTitle = title;
                return this;
            }

            /**
             * Sets the title of this particular episode for a series.
             *
             * @param episodeTitle The value of {@link TvContract.Programs#COLUMN_EPISODE_TITLE} for the
             * program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setEpisodeTitle(String episodeTitle) {
                mProgram.mEpisodeTitle = episodeTitle;
                return this;
            }

            /**
             * Sets the season number for this episode for a series.
             *
             * @param seasonNumber The value of {@link TvContract.Programs#COLUMN_SEASON_DISPLAY_NUMBER}
             * for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setSeasonNumber(int seasonNumber) {
                mProgram.mSeasonNumber = String.valueOf(seasonNumber);
                return this;
            }

            /**
             * Sets the season number for this episode for a series.
             *
             * @param seasonNumber The value of {@link TvContract.Programs#COLUMN_SEASON_NUMBER} for the
             * program.
             * @param numericalSeasonNumber An integer value for
             * {@link TvContract.Programs#COLUMN_SEASON_NUMBER} which will be used for API Level 23 and
             * below.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setSeasonNumber(String seasonNumber, int numericalSeasonNumber) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                    mProgram.mSeasonNumber = seasonNumber;
                } else {
                    mProgram.mSeasonNumber = String.valueOf(numericalSeasonNumber);
                }
                return this;
            }

            /**
             * Sets the episode number in a season for this episode for a series.
             *
             * @param episodeNumber The value of
             * {@link TvContract.Programs#COLUMN_EPISODE_DISPLAY_NUMBER} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setEpisodeNumber(int episodeNumber) {
                mProgram.mEpisodeNumber = String.valueOf(episodeNumber);
                return this;
            }

            /**
             * Sets the episode number in a season for this episode for a series.
             *
             * @param episodeNumber The value of
             * {@link TvContract.Programs#COLUMN_EPISODE_DISPLAY_NUMBER} for the program.
             * @param numericalEpisodeNumber An integer value for
             * {@link TvContract.Programs#COLUMN_SEASON_NUMBER} which will be used for API Level 23 and
             * below.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setEpisodeNumber(String episodeNumber, int numericalEpisodeNumber) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                    mProgram.mEpisodeNumber = episodeNumber;
                } else {
                    mProgram.mEpisodeNumber = String.valueOf(numericalEpisodeNumber);
                }
                return this;
            }

            /**
             * Sets the time when the program is going to begin in milliseconds since the epoch.
             *
             * @param startTimeUtcMillis The value of
             * {@link TvContract.Programs#COLUMN_START_TIME_UTC_MILLIS} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setStartTimeUtcMillis(long startTimeUtcMillis) {
                mProgram.mStartTimeUtcMillis = startTimeUtcMillis;
                return this;
            }

            /**
             * Sets the time when this program is going to end in milliseconds since the epoch.
             *
             * @param endTimeUtcMillis The value of
             * {@link TvContract.Programs#COLUMN_END_TIME_UTC_MILLIS} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setEndTimeUtcMillis(long endTimeUtcMillis) {
                mProgram.mEndTimeUtcMillis = endTimeUtcMillis;
                return this;
            }

            /**
             * Sets a brief description of the program. For a series, this would be a brief description
             * of the episode.
             *
             * @param description The value of {@link TvContract.Programs#COLUMN_SHORT_DESCRIPTION} for
             * the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setDescription(String description) {
                mProgram.mDescription = description;
                return this;
            }

            /**
             * Sets a longer description of a program if one exists.
             *
             * @param longDescription The value of {@link TvContract.Programs#COLUMN_LONG_DESCRIPTION}
             * for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setLongDescription(String longDescription) {
                mProgram.mLongDescription = longDescription;
                return this;
            }

            /**
             * Sets the video width of the program.
             *
             * @param width The value of {@link TvContract.Programs#COLUMN_VIDEO_WIDTH} for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setVideoWidth(int width) {
                mProgram.mVideoWidth = width;
                return this;
            }

            /**
             * Sets the video height of the program.
             *
             * @param height The value of {@link TvContract.Programs#COLUMN_VIDEO_HEIGHT} for the
             * program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setVideoHeight(int height) {
                mProgram.mVideoHeight = height;
                return this;
            }

            /**
             * Sets the content ratings for this program.
             *
             * @param contentRatings An array of {@link TvContentRating} that apply to this program
             * which will be flattened to a String to store in a database.
             * @return This Builder object to allow for chaining of calls to builder methods.
             * @see TvContract.Programs#COLUMN_CONTENT_RATING
             */
            public Builder setContentRatings(TvContentRating[] contentRatings) {
                mProgram.mContentRatings = contentRatings;
                return this;
            }

            /**
             * Sets the large poster art of the program.
             *
             * @param posterArtUri The value of {@link TvContract.Programs#COLUMN_POSTER_ART_URI} for
             * the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setPosterArtUri(String posterArtUri) {
                mProgram.mPosterArtUri = posterArtUri;
                return this;
            }

            /**
             * Sets a small thumbnail of the program.
             *
             * @param thumbnailUri The value of {@link TvContract.Programs#COLUMN_THUMBNAIL_URI} for the
             * program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setThumbnailUri(String thumbnailUri) {
                mProgram.mThumbnailUri = thumbnailUri;
                return this;
            }

            /**
             * Sets the broadcast-specified genres of the program.
             *
             * @param genres Array of genres that apply to the program based on the broadcast standard
             * which will be flattened to a String to store in a database.
             * @return This Builder object to allow for chaining of calls to builder methods.
             * @see TvContract.Programs#COLUMN_BROADCAST_GENRE
             */
            public Builder setBroadcastGenres(String[] genres) {
                mProgram.mBroadcastGenres = genres;
                return this;
            }

            /**
             * Sets the genres of the program.
             *
             * @param genres An array of {@link TvContract.Programs.Genres} that apply to the program
             * which will be flattened to a String to store in a database.
             * @return This Builder object to allow for chaining of calls to builder methods.
             * @see TvContract.Programs#COLUMN_CANONICAL_GENRE
             */
            public Builder setCanonicalGenres(String[] genres) {
                mProgram.mCanonicalGenres = genres;
                return this;
            }

            /**
             * Sets the internal provider data for the program as raw bytes.
             *
             * @param data The value of {@link TvContract.Programs#COLUMN_INTERNAL_PROVIDER_DATA} for
             * the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setInternalProviderData(byte[] data) {
                mProgram.mInternalProviderData = data;
                return this;
            }

            /**
             * Sets the available audio languages for this program as a comma-separated String.
             *
             * @param audioLanguages The value of {@link TvContract.Programs#COLUMN_AUDIO_LANGUAGE} for
             * the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setAudioLanguages(String audioLanguages) {
                mProgram.mAudioLanguages = audioLanguages;
                return this;
            }

            /**
             * Sets whether this program cannot be recorded.
             *
             * @param prohibited The value of {@link TvContract.Programs#COLUMN_RECORDING_PROHIBITED}
             * for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setRecordingProhibited(boolean prohibited) {
                mProgram.mRecordingProhibited = prohibited ? IS_RECORDING_PROHIBITED : 0;
                return this;
            }

            /**
             * Sets whether this channel can be searched for in other applications.
             *
             * @param searchable The value of {@link TvContract.Programs#COLUMN_SEARCHABLE}
             * for the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setSearchable(boolean searchable) {
                mProgram.mSearchable = searchable ? IS_SEARCHABLE : 0;
                return this;
            }

            /**
             * Sets a custom name for the season, if applicable.
             *
             * @param seasonTitle The value of {@link TvContract.Programs#COLUMN_SEASON_TITLE} for
             * the program.
             * @return This Builder object to allow for chaining of calls to builder methods.
             */
            public Builder setSeasonTitle(String seasonTitle) {
                mProgram.mSeasonTitle = seasonTitle;
                return this;
            }

            /**
             * @return A new Program with values supplied by the Builder.
             */
            public TvProgram build() {
                TvProgram program = new TvProgram();
                program.copyFrom(mProgram);
                if (mProgram.mStartTimeUtcMillis >= mProgram.mEndTimeUtcMillis) {
                    throw new IllegalArgumentException("This program must have defined start and end " +
                            "times");
                }
                return program;
            }
        }
    }
}
