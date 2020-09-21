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

import static android.autofillservice.cts.Helper.getAutofillIds;

import static com.google.common.truth.Truth.assertWithMessage;

import android.app.assist.AssistStructure;
import android.app.assist.AssistStructure.ViewNode;
import android.content.IntentSender;
import android.os.Bundle;
import android.service.autofill.CustomDescription;
import android.service.autofill.Dataset;
import android.service.autofill.FillCallback;
import android.service.autofill.FillResponse;
import android.service.autofill.Sanitizer;
import android.service.autofill.SaveInfo;
import android.service.autofill.UserData;
import android.service.autofill.Validator;
import android.util.Pair;
import android.view.autofill.AutofillId;
import android.view.autofill.AutofillValue;
import android.widget.RemoteViews;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.regex.Pattern;

/**
 * Helper class used to produce a {@link FillResponse} based on expected fields that should be
 * present in the {@link AssistStructure}.
 *
 * <p>Typical usage:
 *
 * <pre class="prettyprint">
 * InstrumentedAutoFillService.setFillResponse(new CannedFillResponse.Builder()
 *               .addDataset(new CannedDataset.Builder("dataset_name")
 *                   .setField("resource_id1", AutofillValue.forText("value1"))
 *                   .setField("resource_id2", AutofillValue.forText("value2"))
 *                   .build())
 *               .build());
 * </pre class="prettyprint">
 */
final class CannedFillResponse {

    private final ResponseType mResponseType;
    private final List<CannedDataset> mDatasets;
    private final ArrayList<Pair<Sanitizer, AutofillId[]>> mSanitizers;
    private final String mFailureMessage;
    private final int mSaveType;
    private final Validator mValidator;
    private final String[] mRequiredSavableIds;
    private final String[] mOptionalSavableIds;
    private final AutofillId[] mRequiredSavableAutofillIds;
    private final String mSaveDescription;
    private final CustomDescription mCustomDescription;
    private final Bundle mExtras;
    private final RemoteViews mPresentation;
    private final RemoteViews mHeader;
    private final RemoteViews mFooter;
    private final IntentSender mAuthentication;
    private final String[] mAuthenticationIds;
    private final String[] mIgnoredIds;
    private final int mNegativeActionStyle;
    private final IntentSender mNegativeActionListener;
    private final int mSaveInfoFlags;
    private final int mFillResponseFlags;
    private final AutofillId mSaveTriggerId;
    private final long mDisableDuration;
    private final AutofillId[] mFieldClassificationIds;
    private final boolean mFieldClassificationIdsOverflow;

    private CannedFillResponse(Builder builder) {
        mResponseType = builder.mResponseType;
        mDatasets = builder.mDatasets;
        mFailureMessage = builder.mFailureMessage;
        mValidator = builder.mValidator;
        mRequiredSavableIds = builder.mRequiredSavableIds;
        mRequiredSavableAutofillIds = builder.mRequiredSavableAutofillIds;
        mOptionalSavableIds = builder.mOptionalSavableIds;
        mSaveDescription = builder.mSaveDescription;
        mCustomDescription = builder.mCustomDescription;
        mSaveType = builder.mSaveType;
        mExtras = builder.mExtras;
        mPresentation = builder.mPresentation;
        mHeader = builder.mHeader;
        mFooter = builder.mFooter;
        mAuthentication = builder.mAuthentication;
        mAuthenticationIds = builder.mAuthenticationIds;
        mIgnoredIds = builder.mIgnoredIds;
        mNegativeActionStyle = builder.mNegativeActionStyle;
        mNegativeActionListener = builder.mNegativeActionListener;
        mSanitizers = builder.mSanitizers;
        mSaveInfoFlags = builder.mSaveInfoFlags;
        mFillResponseFlags = builder.mFillResponseFlags;
        mSaveTriggerId = builder.mSaveTriggerId;
        mDisableDuration = builder.mDisableDuration;
        mFieldClassificationIds = builder.mFieldClassificationIds;
        mFieldClassificationIdsOverflow = builder.mFieldClassificationIdsOverflow;
    }

    /**
     * Constant used to pass a {@code null} response to the
     * {@link FillCallback#onSuccess(FillResponse)} method.
     */
    static final CannedFillResponse NO_RESPONSE =
            new Builder(ResponseType.NULL).build();

    /**
     * Constant used to emulate a timeout by not calling any method on {@link FillCallback}.
     */
    static final CannedFillResponse DO_NOT_REPLY_RESPONSE =
            new Builder(ResponseType.TIMEOUT).build();


    String getFailureMessage() {
        return mFailureMessage;
    }

    ResponseType getResponseType() {
        return mResponseType;
    }

    /**
     * Creates a new response, replacing the dataset field ids by the real ids from the assist
     * structure.
     */
    FillResponse asFillResponse(Function<String, ViewNode> nodeResolver) {
        final FillResponse.Builder builder = new FillResponse.Builder()
                .setFlags(mFillResponseFlags);
        if (mDatasets != null) {
            for (CannedDataset cannedDataset : mDatasets) {
                final Dataset dataset = cannedDataset.asDataset(nodeResolver);
                assertWithMessage("Cannot create datase").that(dataset).isNotNull();
                builder.addDataset(dataset);
            }
        }
        if (mRequiredSavableIds != null || mRequiredSavableAutofillIds != null) {
            final SaveInfo.Builder saveInfo;
            if (mRequiredSavableAutofillIds != null) {
                saveInfo = new SaveInfo.Builder(mSaveType, mRequiredSavableAutofillIds);
            } else {
                saveInfo = mRequiredSavableIds == null || mRequiredSavableIds.length == 0
                        ? new SaveInfo.Builder(mSaveType)
                            : new SaveInfo.Builder(mSaveType,
                                    getAutofillIds(nodeResolver, mRequiredSavableIds));
            }

            saveInfo.setFlags(mSaveInfoFlags);

            if (mValidator != null) {
                saveInfo.setValidator(mValidator);
            }
            if (mOptionalSavableIds != null) {
                saveInfo.setOptionalIds(getAutofillIds(nodeResolver, mOptionalSavableIds));
            }
            if (mSaveDescription != null) {
                saveInfo.setDescription(mSaveDescription);
            }
            saveInfo.setNegativeAction(mNegativeActionStyle, mNegativeActionListener);

            if (mCustomDescription != null) {
                saveInfo.setCustomDescription(mCustomDescription);
            }

            for (Pair<Sanitizer, AutofillId[]> sanitizer : mSanitizers) {
                saveInfo.addSanitizer(sanitizer.first, sanitizer.second);
            }

            if (mSaveTriggerId != null) {
                saveInfo.setTriggerId(mSaveTriggerId);
            }
            builder.setSaveInfo(saveInfo.build());
        }
        if (mIgnoredIds != null) {
            builder.setIgnoredIds(getAutofillIds(nodeResolver, mIgnoredIds));
        }
        if (mAuthenticationIds != null) {
            builder.setAuthentication(getAutofillIds(nodeResolver, mAuthenticationIds),
                    mAuthentication, mPresentation);
        }
        if (mDisableDuration > 0) {
            builder.disableAutofill(mDisableDuration);
        }
        if (mFieldClassificationIdsOverflow) {
            final int length = UserData.getMaxFieldClassificationIdsSize() + 1;
            final AutofillId[] fieldIds = new AutofillId[length];
            for (int i = 0; i < length; i++) {
                fieldIds[i] = new AutofillId(i);
            }
            builder.setFieldClassificationIds(fieldIds);
        } else if (mFieldClassificationIds != null) {
            builder.setFieldClassificationIds(mFieldClassificationIds);
        }
        if (mExtras != null) {
            builder.setClientState(mExtras);
        }
        if (mHeader != null) {
            builder.setHeader(mHeader);
        }
        if (mFooter != null) {
            builder.setFooter(mFooter);
        }
        return builder.build();
    }

    @Override
    public String toString() {
        return "CannedFillResponse: [type=" + mResponseType
                + ",datasets=" + mDatasets
                + ", requiredSavableIds=" + Arrays.toString(mRequiredSavableIds)
                + ", optionalSavableIds=" + Arrays.toString(mOptionalSavableIds)
                + ", requiredSavableAutofillIds=" + Arrays.toString(mRequiredSavableAutofillIds)
                + ", saveInfoFlags=" + mSaveInfoFlags
                + ", fillResponseFlags=" + mFillResponseFlags
                + ", failureMessage=" + mFailureMessage
                + ", saveDescription=" + mSaveDescription
                + ", mCustomDescription=" + mCustomDescription
                + ", hasPresentation=" + (mPresentation != null)
                + ", hasHeader=" + (mHeader != null)
                + ", hasFooter=" + (mFooter != null)
                + ", hasAuthentication=" + (mAuthentication != null)
                + ", authenticationIds=" + Arrays.toString(mAuthenticationIds)
                + ", ignoredIds=" + Arrays.toString(mIgnoredIds)
                + ", sanitizers =" + mSanitizers
                + ", saveTriggerId=" + mSaveTriggerId
                + ", disableDuration=" + mDisableDuration
                + ", fieldClassificationIds=" + Arrays.toString(mFieldClassificationIds)
                + ", fieldClassificationIdsOverflow=" + mFieldClassificationIdsOverflow
                + "]";
    }

    enum ResponseType {
        NORMAL,
        NULL,
        TIMEOUT
    }

    static class Builder {
        private final List<CannedDataset> mDatasets = new ArrayList<>();
        private final ArrayList<Pair<Sanitizer, AutofillId[]>> mSanitizers = new ArrayList<>();
        private final ResponseType mResponseType;
        private String mFailureMessage;
        private Validator mValidator;
        private String[] mRequiredSavableIds;
        private String[] mOptionalSavableIds;
        private AutofillId[] mRequiredSavableAutofillIds;
        private String mSaveDescription;
        public CustomDescription mCustomDescription;
        public int mSaveType = -1;
        private Bundle mExtras;
        private RemoteViews mPresentation;
        private RemoteViews mFooter;
        private RemoteViews mHeader;
        private IntentSender mAuthentication;
        private String[] mAuthenticationIds;
        private String[] mIgnoredIds;
        private int mNegativeActionStyle;
        private IntentSender mNegativeActionListener;
        private int mSaveInfoFlags;
        private int mFillResponseFlags;
        private AutofillId mSaveTriggerId;
        private long mDisableDuration;
        private AutofillId[] mFieldClassificationIds;
        private boolean mFieldClassificationIdsOverflow;

        public Builder(ResponseType type) {
            mResponseType = type;
        }

        public Builder() {
            this(ResponseType.NORMAL);
        }

        public Builder addDataset(CannedDataset dataset) {
            assertWithMessage("already set failure").that(mFailureMessage).isNull();
            mDatasets.add(dataset);
            return this;
        }

        /**
         * Sets the validator for this request
         */
        public Builder setValidator(Validator validator) {
            mValidator = validator;
            return this;
        }

        /**
         * Sets the required savable ids based on their {@code resourceId}.
         */
        public Builder setRequiredSavableIds(int type, String... ids) {
            if (mRequiredSavableAutofillIds != null) {
                throw new IllegalStateException("Already set required autofill ids: "
                        + Arrays.toString(mRequiredSavableAutofillIds));
            }
            mSaveType = type;
            mRequiredSavableIds = ids;
            return this;
        }

        /**
         * Sets the required savable ids based on their {@code autofillId}.
         */
        public Builder setRequiredSavableAutofillIds(int type, AutofillId... ids) {
            if (mRequiredSavableIds != null) {
                throw new IllegalStateException("Already set required resource ids: "
                        + Arrays.toString(mRequiredSavableIds));
            }
            mSaveType = type;
            mRequiredSavableAutofillIds = ids;
            return this;
        }

        public Builder setSaveInfoFlags(int flags) {
            mSaveInfoFlags = flags;
            return this;
        }

        public Builder setFillResponseFlags(int flags) {
            mFillResponseFlags = flags;
            return this;
        }

        /**
         * Sets the optional savable ids based on they {@code resourceId}.
         */
        public Builder setOptionalSavableIds(String... ids) {
            mOptionalSavableIds = ids;
            return this;
        }

        /**
         * Sets the description passed to the {@link SaveInfo}.
         */
        public Builder setSaveDescription(String description) {
            mSaveDescription = description;
            return this;
        }

        /**
         * Sets the description passed to the {@link SaveInfo}.
         */
        public Builder setCustomDescription(CustomDescription description) {
            mCustomDescription = description;
            return this;
        }

        /**
         * Sets the extra passed to {@link
         * android.service.autofill.FillResponse.Builder#setClientState(Bundle)}.
         */
        public Builder setExtras(Bundle data) {
            mExtras = data;
            return this;
        }

        /**
         * Sets the view to present the response in the UI.
         */
        public Builder setPresentation(RemoteViews presentation) {
            mPresentation = presentation;
            return this;
        }

        /**
         * Sets the authentication intent.
         */
        public Builder setAuthentication(IntentSender authentication, String... ids) {
            mAuthenticationIds = ids;
            mAuthentication = authentication;
            return this;
        }

        /**
         * Sets the ignored fields based on resource ids.
         */
        public Builder setIgnoreFields(String...ids) {
            mIgnoredIds = ids;
            return this;
        }

        /**
         * Sets the negative action spec.
         */
        public Builder setNegativeAction(int style, IntentSender listener) {
            mNegativeActionStyle = style;
            mNegativeActionListener = listener;
            return this;
        }

        /**
         * Adds a save sanitizer.
         */
        public Builder addSanitizer(Sanitizer sanitizer, AutofillId... ids) {
            mSanitizers.add(new Pair<>(sanitizer, ids));
            return this;
        }

        public CannedFillResponse build() {
            return new CannedFillResponse(this);
        }

        /**
         * Sets the response to call {@link FillCallback#onFailure(CharSequence)}.
         */
        public Builder returnFailure(String message) {
            assertWithMessage("already added datasets").that(mDatasets).isEmpty();
            mFailureMessage = message;
            return this;
        }

        /**
         * Sets the view that explicitly triggers save.
         */
        public Builder setSaveTriggerId(AutofillId id) {
            assertWithMessage("already set").that(mSaveTriggerId).isNull();
            mSaveTriggerId = id;
            return this;
        }

        public Builder disableAutofill(long duration) {
            assertWithMessage("already set").that(mDisableDuration).isEqualTo(0L);
            mDisableDuration = duration;
            return this;
        }

        /**
         * Sets the ids used for field classification.
         */
        public Builder setFieldClassificationIds(AutofillId... ids) {
            assertWithMessage("already set").that(mFieldClassificationIds).isNull();
            mFieldClassificationIds = ids;
            return this;
        }

        /**
         * Forces the service to throw an exception when setting the fields classification ids.
         */
        public Builder setFieldClassificationIdsOverflow() {
            mFieldClassificationIdsOverflow = true;
            return this;
        }

        public Builder setHeader(RemoteViews header) {
            assertWithMessage("already set").that(mHeader).isNull();
            mHeader = header;
            return this;
        }

        public Builder setFooter(RemoteViews footer) {
            assertWithMessage("already set").that(mFooter).isNull();
            mFooter = footer;
            return this;
        }
    }

    /**
     * Helper class used to produce a {@link Dataset} based on expected fields that should be
     * present in the {@link AssistStructure}.
     *
     * <p>Typical usage:
     *
     * <pre class="prettyprint">
     * InstrumentedAutoFillService.setFillResponse(new CannedFillResponse.Builder()
     *               .addDataset(new CannedDataset.Builder("dataset_name")
     *                   .setField("resource_id1", AutofillValue.forText("value1"))
     *                   .setField("resource_id2", AutofillValue.forText("value2"))
     *                   .build())
     *               .build());
     * </pre class="prettyprint">
     */
    static class CannedDataset {
        private final Map<String, AutofillValue> mFieldValues;
        private final Map<AutofillId, AutofillValue> mFieldValuesById;
        private final Map<AutofillId, RemoteViews> mFieldPresentationsById;
        private final Map<String, RemoteViews> mFieldPresentations;
        private final Map<String, Pair<Boolean, Pattern>> mFieldFilters;
        private final RemoteViews mPresentation;
        private final IntentSender mAuthentication;
        private final String mId;

        private CannedDataset(Builder builder) {
            mFieldValues = builder.mFieldValues;
            mFieldValuesById = builder.mFieldValuesById;
            mFieldPresentationsById = builder.mFieldPresentationsById;
            mFieldPresentations = builder.mFieldPresentations;
            mFieldFilters = builder.mFieldFilters;
            mPresentation = builder.mPresentation;
            mAuthentication = builder.mAuthentication;
            mId = builder.mId;
        }

        /**
         * Creates a new dataset, replacing the field ids by the real ids from the assist structure.
         */
        Dataset asDataset(Function<String, ViewNode> nodeResolver) {
            final Dataset.Builder builder = (mPresentation == null)
                    ? new Dataset.Builder()
                    : new Dataset.Builder(mPresentation);

            if (mFieldValues != null) {
                for (Map.Entry<String, AutofillValue> entry : mFieldValues.entrySet()) {
                    final String id = entry.getKey();
                    final ViewNode node = nodeResolver.apply(id);
                    if (node == null) {
                        throw new AssertionError("No node with resource id " + id);
                    }
                    final AutofillId autofillId = node.getAutofillId();
                    final AutofillValue value = entry.getValue();
                    final RemoteViews presentation = mFieldPresentations.get(id);
                    final Pair<Boolean, Pattern> filter = mFieldFilters.get(id);
                    if (presentation != null) {
                        if (filter == null) {
                            builder.setValue(autofillId, value, presentation);
                        } else {
                            builder.setValue(autofillId, value, filter.second, presentation);
                        }
                    } else {
                        if (filter == null) {
                            builder.setValue(autofillId, value);
                        } else {
                            builder.setValue(autofillId, value, filter.second);
                        }
                    }
                }
            }
            if (mFieldValuesById != null) {
                // NOTE: filter is not yet supported when calling methods that explicitly pass
                // autofill id
                for (Map.Entry<AutofillId, AutofillValue> entry : mFieldValuesById.entrySet()) {
                    final AutofillId autofillId = entry.getKey();
                    final AutofillValue value = entry.getValue();
                    final RemoteViews presentation = mFieldPresentationsById.get(autofillId);
                    if (presentation != null) {
                        builder.setValue(autofillId, value, presentation);
                    } else {
                        builder.setValue(autofillId, value);
                    }
                }
            }
            builder.setId(mId).setAuthentication(mAuthentication);
            return builder.build();
        }

        @Override
        public String toString() {
            return "CannedDataset " + mId + " : [hasPresentation=" + (mPresentation != null)
                    + ", fieldPresentations=" + (mFieldPresentations)
                    + ", fieldPresentationsById=" + (mFieldPresentationsById)
                    + ", hasAuthentication=" + (mAuthentication != null)
                    + ", fieldValues=" + mFieldValues
                    + ", fieldValuesById=" + mFieldValuesById
                    + ", fieldFilters=" + mFieldFilters + "]";
        }

        static class Builder {
            private final Map<String, AutofillValue> mFieldValues = new HashMap<>();
            private final Map<AutofillId, AutofillValue> mFieldValuesById = new HashMap<>();
            private final Map<String, RemoteViews> mFieldPresentations = new HashMap<>();
            private final Map<AutofillId, RemoteViews> mFieldPresentationsById = new HashMap<>();
            private final Map<String, Pair<Boolean, Pattern>> mFieldFilters = new HashMap<>();

            private RemoteViews mPresentation;
            private IntentSender mAuthentication;
            private String mId;

            public Builder() {

            }

            public Builder(RemoteViews presentation) {
                mPresentation = presentation;
            }

            /**
             * Sets the canned value of a text field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, String text) {
                return setField(id, AutofillValue.forText(text));
            }

            /**
             * Sets the canned value of a text field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, String text, Pattern filter) {
                return setField(id, AutofillValue.forText(text), true, filter);
            }

            public Builder setUnfilterableField(String id, String text) {
                return setField(id, AutofillValue.forText(text), false, null);
            }

            /**
             * Sets the canned value of a list field based on its its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, int index) {
                return setField(id, AutofillValue.forList(index));
            }

            /**
             * Sets the canned value of a toggle field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, boolean toggled) {
                return setField(id, AutofillValue.forToggle(toggled));
            }

            /**
             * Sets the canned value of a date field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, long date) {
                return setField(id, AutofillValue.forDate(date));
            }

            /**
             * Sets the canned value of a date field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, AutofillValue value) {
                mFieldValues.put(id, value);
                return this;
            }

            /**
             * Sets the canned value of a date field based on its {@code autofillId}.
             */
            public Builder setField(AutofillId autofillId, AutofillValue value) {
                mFieldValuesById.put(autofillId, value);
                return this;
            }

            /**
             * Sets the canned value of a date field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, AutofillValue value, boolean filterable,
                    Pattern filter) {
                setField(id, value);
                mFieldFilters.put(id, new Pair<>(filterable, filter));
                return this;
            }

            /**
             * Sets the canned value of a field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, String text, RemoteViews presentation) {
                setField(id, text);
                mFieldPresentations.put(id, presentation);
                return this;
            }

            /**
             * Sets the canned value of a date field based on its {@code autofillId}.
             */
            public Builder setField(AutofillId autofillId, String text, RemoteViews presentation) {
                setField(autofillId, AutofillValue.forText(text));
                mFieldPresentationsById.put(autofillId, presentation);
                return this;
            }

            /**
             * Sets the canned value of a field based on its {@code id}.
             *
             * <p>The meaning of the id is defined by the object using the canned dataset.
             * For example, {@link InstrumentedAutoFillService.Replier} resolves the id based on
             * {@link IdMode}.
             */
            public Builder setField(String id, String text, RemoteViews presentation,
                    Pattern filter) {
                setField(id, text, presentation);
                mFieldFilters.put(id, new Pair<>(true, filter));
                return this;
            }

            /**
             * Sets the view to present the response in the UI.
             */
            public Builder setPresentation(RemoteViews presentation) {
                mPresentation = presentation;
                return this;
            }

            /**
             * Sets the authentication intent.
             */
            public Builder setAuthentication(IntentSender authentication) {
                mAuthentication = authentication;
                return this;
            }

            /**
             * Sets the name.
             */
            public Builder setId(String id) {
                mId = id;
                return this;
            }

            public CannedDataset build() {
                return new CannedDataset(this);
            }
        }
    }
}
