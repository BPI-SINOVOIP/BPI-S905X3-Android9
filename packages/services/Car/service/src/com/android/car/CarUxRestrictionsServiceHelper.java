/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car;

import android.annotation.Nullable;
import android.annotation.XmlRes;
import android.car.drivingstate.CarDrivingStateEvent;
import android.car.drivingstate.CarDrivingStateEvent.CarDrivingState;
import android.car.drivingstate.CarUxRestrictions;
import android.content.Context;
import android.content.res.TypedArray;
import android.content.res.XmlResourceParser;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Pair;
import android.util.Xml;

import org.xmlpull.v1.XmlPullParserException;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Helper class to {@link CarUxRestrictionsManagerService} and it takes care of the foll:
 * <ol>
 * <li>Parses the given XML resource and builds a hashmap to store the driving state to UX
 * restrictions mapping information provided in the XML.</li>
 * <li>Finds the UX restrictions for the given driving state and speed from the data structure it
 * built above.</li>
 * </ol>
 */
/* package */ class CarUxRestrictionsServiceHelper {
    private static final String TAG = "UxRServiceHelper";
    private static final int UX_RESTRICTIONS_UNKNOWN = -1;
    // XML tags to parse
    private static final String ROOT_ELEMENT = "UxRestrictions";
    private static final String RESTRICTION_MAPPING = "RestrictionMapping";
    private static final String RESTRICTION_PARAMETERS = "RestrictionParameters";
    private static final String DRIVING_STATE = "DrivingState";
    private static final String RESTRICTIONS = "Restrictions";
    private static final String STRING_RESTRICTIONS = "StringRestrictions";
    private static final String CONTENT_RESTRICTIONS = "ContentRestrictions";

    /* Hashmap that maps driving state to RestrictionsInfo.
    RestrictionsInfo maintains a list of RestrictionsPerSpeedRange.
    The list size will be one for Parked and Idling states, but could be more than one
    for Moving state, if moving state supports multiple speed ranges.*/
    private Map<Integer, RestrictionsInfo> mRestrictionsMap = new HashMap<>();
    private RestrictionParameters mRestrictionParameters = new RestrictionParameters();
    private final Context mContext;
    @XmlRes
    private final int mXmlResource;

    CarUxRestrictionsServiceHelper(Context context, @XmlRes int xmlRes) {
        mContext = context;
        mXmlResource = xmlRes;
    }

    /**
     * Loads the UX restrictions related information from the XML resource.
     *
     * @return true if successful, false if the XML is malformed
     */
    public boolean loadUxRestrictionsFromXml() throws IOException, XmlPullParserException {
        mRestrictionsMap.clear();
        XmlResourceParser parser = mContext.getResources().getXml(mXmlResource);
        AttributeSet attrs = Xml.asAttributeSet(parser);
        int type;
        // Traverse till we get to the first tag
        while ((type = parser.next()) != XmlResourceParser.END_DOCUMENT
                && type != XmlResourceParser.START_TAG) {
        }
        if (!ROOT_ELEMENT.equals(parser.getName())) {
            Log.e(TAG, "XML root element invalid: " + parser.getName());
            return false;
        }

        // Traverse till the end and every time we hit a start tag, check for the type of the tag
        // and load the corresponding information.
        while (parser.getEventType() != XmlResourceParser.END_DOCUMENT) {
            if (parser.next() == XmlResourceParser.START_TAG) {
                switch (parser.getName()) {
                    case RESTRICTION_MAPPING:
                        if (!mapDrivingStateToRestrictions(parser, attrs)) {
                            return false;
                        }
                        break;
                    case RESTRICTION_PARAMETERS:
                        if (!parseRestrictionParameters(parser, attrs)) {
                            // Failure to parse is automatically handled by falling back to
                            // defaults.  Just log the information here
                            if (Log.isLoggable(TAG, Log.INFO)) {
                                Log.i(TAG,
                                        "Error reading restrictions parameters.  Falling back to "
                                                + "platform defaults.");
                            }
                        }
                        break;
                    default:
                        Log.w(TAG, "Unknown class:" + parser.getName());
                }
            }
        }
        return true;
    }

    /**
     * Parses the information in the <restrictionMapping> tag to construct the mapping from
     * driving state to UX restrictions.
     */
    private boolean mapDrivingStateToRestrictions(XmlResourceParser parser, AttributeSet attrs)
            throws IOException, XmlPullParserException {
        if (parser == null || attrs == null) {
            Log.e(TAG, "Invalid arguments");
            return false;
        }
        // The parser should be at the <RestrictionMapping> tag at this point.
        if (!RESTRICTION_MAPPING.equals(parser.getName())) {
            Log.e(TAG, "Parser not at RestrictionMapping element: " + parser.getName());
            return false;
        }
        if (!traverseToTag(parser, DRIVING_STATE)) {
            Log.e(TAG, "No <" + DRIVING_STATE + "> tag in XML");
            return false;
        }
        // Handle all the <DrivingState> tags.
        while (DRIVING_STATE.equals(parser.getName())) {
            if (parser.getEventType() == XmlResourceParser.START_TAG) {
                // 1. Get the driving state attributes: driving state and speed range
                TypedArray a = mContext.getResources().obtainAttributes(attrs,
                        R.styleable.UxRestrictions_DrivingState);
                int drivingState = a
                        .getInt(R.styleable.UxRestrictions_DrivingState_state,
                                CarDrivingStateEvent.DRIVING_STATE_UNKNOWN);
                float minSpeed = a
                        .getFloat(
                                R.styleable
                                        .UxRestrictions_DrivingState_minSpeed,
                                RestrictionsPerSpeedRange.SPEED_INVALID);
                float maxSpeed = a
                        .getFloat(
                                R.styleable
                                        .UxRestrictions_DrivingState_maxSpeed,
                                RestrictionsPerSpeedRange.SPEED_INVALID);
                a.recycle();

                // 2. Traverse to the <Restrictions> tag
                if (!traverseToTag(parser, RESTRICTIONS)) {
                    Log.e(TAG, "No <" + RESTRICTIONS + "> tag in XML");
                    return false;
                }

                // 3. Parse the restrictions for this driving state
                Pair<Boolean, Integer> restrictions = parseRestrictions(parser, attrs);
                if (Log.isLoggable(TAG, Log.DEBUG)) {
                    Log.d(TAG, "Map " + drivingState + " : " + restrictions);
                }

                // Update the hashmap if the driving state and restrictions info are valid.
                if (drivingState != CarDrivingStateEvent.DRIVING_STATE_UNKNOWN
                        && restrictions != null) {
                    addToRestrictionsMap(drivingState, minSpeed, maxSpeed, restrictions.first,
                            restrictions.second);
                }
            }
            parser.next();
        }
        return true;
    }

    /**
     * Parses the <restrictions> tag nested with the <drivingState>.  This provides the restrictions
     * for the enclosing driving state.
     */
    @Nullable
    private Pair<Boolean, Integer> parseRestrictions(XmlResourceParser parser, AttributeSet attrs)
            throws IOException, XmlPullParserException {
        int restrictions = UX_RESTRICTIONS_UNKNOWN;
        boolean requiresOpt = true;
        if (parser == null || attrs == null) {
            Log.e(TAG, "Invalid Arguments");
            return null;
        }

        while (RESTRICTIONS.equals(parser.getName())
                && parser.getEventType() == XmlResourceParser.START_TAG) {
            TypedArray a = mContext.getResources().obtainAttributes(attrs,
                    R.styleable.UxRestrictions_Restrictions);
            restrictions = a.getInt(
                    R.styleable.UxRestrictions_Restrictions_uxr,
                    CarUxRestrictions.UX_RESTRICTIONS_FULLY_RESTRICTED);
            requiresOpt = a.getBoolean(
                    R.styleable.UxRestrictions_Restrictions_requiresDistractionOptimization, true);
            a.recycle();
            parser.next();
        }
        return new Pair<>(requiresOpt, restrictions);
    }

    private void addToRestrictionsMap(int drivingState, float minSpeed, float maxSpeed,
            boolean requiresOpt, int restrictions) {
        RestrictionsPerSpeedRange res = new RestrictionsPerSpeedRange(minSpeed, maxSpeed,
                restrictions, requiresOpt);
        RestrictionsInfo restrictionsList = mRestrictionsMap.get(drivingState);
        if (restrictionsList == null) {
            restrictionsList = new RestrictionsInfo();
        }
        restrictionsList.addRestrictions(res);
        mRestrictionsMap.put(drivingState, restrictionsList);
    }

    private boolean traverseToTag(XmlResourceParser parser, String tag)
            throws IOException, XmlPullParserException {
        if (tag == null || parser == null) {
            return false;
        }
        int type;
        while ((type = parser.next()) != XmlResourceParser.END_DOCUMENT) {
            if (type == XmlResourceParser.START_TAG && parser.getName().equals(tag)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Parses the information in the <RestrictionParameters> tag to read the parameters for the
     * applicable UX restrictions
     */
    private boolean parseRestrictionParameters(XmlResourceParser parser, AttributeSet attrs)
            throws IOException, XmlPullParserException {
        if (parser == null || attrs == null) {
            Log.e(TAG, "Invalid arguments");
            return false;
        }
        // The parser should be at the <RestrictionParameters> tag at this point.
        if (!RESTRICTION_PARAMETERS.equals(parser.getName())) {
            Log.e(TAG, "Parser not at RestrictionParameters element: " + parser.getName());
            return false;
        }
        while (parser.getEventType() != XmlResourceParser.END_DOCUMENT) {
            int type = parser.next();
            // Break if we have parsed all <RestrictionParameters>
            if (type == XmlResourceParser.END_TAG && RESTRICTION_PARAMETERS.equals(
                    parser.getName())) {
                return true;
            }
            if (type == XmlResourceParser.START_TAG) {
                TypedArray a = null;
                switch (parser.getName()) {
                    case STRING_RESTRICTIONS:
                        a = mContext.getResources().obtainAttributes(attrs,
                                R.styleable.UxRestrictions_StringRestrictions);
                        mRestrictionParameters.mMaxStringLength = a
                                .getInt(R.styleable.UxRestrictions_StringRestrictions_maxLength,
                                        UX_RESTRICTIONS_UNKNOWN);

                        break;
                    case CONTENT_RESTRICTIONS:
                        a = mContext.getResources().obtainAttributes(attrs,
                                R.styleable.UxRestrictions_ContentRestrictions);
                        mRestrictionParameters.mMaxCumulativeContentItems = a.getInt(R.styleable
                                        .UxRestrictions_ContentRestrictions_maxCumulativeItems,
                                UX_RESTRICTIONS_UNKNOWN);
                        mRestrictionParameters.mMaxContentDepth = a
                                .getInt(R.styleable.UxRestrictions_ContentRestrictions_maxDepth,
                                        UX_RESTRICTIONS_UNKNOWN);
                        break;
                    default:
                        if (Log.isLoggable(TAG, Log.DEBUG)) {
                            Log.d(TAG, "Unsupported Restriction Parameters in XML: "
                                    + parser.getName());
                        }
                        break;
                }
                if (a != null) {
                    a.recycle();
                }
            }
        }
        return true;
    }

    /**
     * Dump the driving state to UX restrictions mapping.
     */
    public void dump(PrintWriter writer) {
        for (Integer state : mRestrictionsMap.keySet()) {
            RestrictionsInfo list = mRestrictionsMap.get(state);
            writer.println("===========================================");
            writer.println("Driving State to UXR");
            if (list != null && list.mRestrictionsList != null) {
                writer.println("State:" + getDrivingStateName(state) + " num restrictions:"
                        + list.mRestrictionsList.size());
                for (RestrictionsPerSpeedRange r : list.mRestrictionsList) {
                    writer.println(
                            "Speed Range: " + r.mMinSpeed + "-" + r.mMaxSpeed + " Requires DO? "
                                    + r.mRequiresDistractionOptimization + " Restrictions: 0x"
                                    + Integer.toHexString(r.mRestrictions));
                    writer.println("===========================================");
                }
            }
        }
        writer.println("Max String length: " + mRestrictionParameters.mMaxStringLength);
        writer.println(
                "Max Cumul Content Items: " + mRestrictionParameters.mMaxCumulativeContentItems);
        writer.println("Max Content depth: " + mRestrictionParameters.mMaxContentDepth);
    }

    private static String getDrivingStateName(int state) {
        switch (state) {
            case 0:
                return "parked";
            case 1:
                return "idling";
            case 2:
                return "moving";
            default:
                return "unknown";
        }
    }

    /**
     * Get the UX restrictions for the given driving state and speed.
     *
     * @param drivingState driving state of the vehicle
     * @param currentSpeed speed of the vehicle
     * @return UX restrictions for the given driving state and speed.
     */
    public CarUxRestrictions getUxRestrictions(@CarDrivingState int drivingState,
            float currentSpeed) {
        RestrictionsPerSpeedRange restrictions;
        RestrictionsInfo restrictionsList = mRestrictionsMap.get(drivingState);
        // If the XML hasn't been parsed or if the given driving state is not supported in the
        // XML, return fully restricted.
        if (restrictionsList == null || restrictionsList.mRestrictionsList == null
                || restrictionsList.mRestrictionsList.isEmpty()) {
            return createUxRestrictionsEvent(true,
                    CarUxRestrictions.UX_RESTRICTIONS_FULLY_RESTRICTED);
        }
        // For Parked and Idling, the restrictions list will have only one item, since multiple
        // speed ranges don't make sense in those driving states.
        if (restrictionsList.mRestrictionsList.size() == 1) {
            restrictions = restrictionsList.mRestrictionsList.get(0);
        } else {
            restrictions = restrictionsList.findRestrictions(currentSpeed);
        }
        if (restrictions != null) {
            return createUxRestrictionsEvent(restrictions.mRequiresDistractionOptimization,
                    restrictions.mRestrictions);
        } else {
            return createUxRestrictionsEvent(true,
                    CarUxRestrictions.UX_RESTRICTIONS_FULLY_RESTRICTED);
        }
    }

    /* package */ CarUxRestrictions createUxRestrictionsEvent(boolean requiresOpt,
            @CarUxRestrictions.CarUxRestrictionsInfo int uxr) {
        // In case the UXR is not baseline, set requiresDistractionOptimization to true since it
        // doesn't make sense to have an active non baseline restrictions without
        // requiresDistractionOptimization set to true.
        if (uxr != CarUxRestrictions.UX_RESTRICTIONS_BASELINE) {
            requiresOpt = true;
        }
        CarUxRestrictions.Builder builder = new CarUxRestrictions.Builder(requiresOpt, uxr,
                SystemClock.elapsedRealtimeNanos());
        if (mRestrictionParameters.mMaxStringLength != UX_RESTRICTIONS_UNKNOWN) {
            builder.setMaxStringLength(mRestrictionParameters.mMaxStringLength);
        }
        if (mRestrictionParameters.mMaxCumulativeContentItems != UX_RESTRICTIONS_UNKNOWN) {
            builder.setMaxCumulativeContentItems(
                    mRestrictionParameters.mMaxCumulativeContentItems);
        }
        if (mRestrictionParameters.mMaxContentDepth != UX_RESTRICTIONS_UNKNOWN) {
            builder.setMaxContentDepth(mRestrictionParameters.mMaxContentDepth);
        }
        return builder.build();
    }

    /**
     * Container for the UX restrictions that could be parametrized
     */
    private class RestrictionParameters {
        int mMaxStringLength = UX_RESTRICTIONS_UNKNOWN;
        int mMaxCumulativeContentItems = UX_RESTRICTIONS_UNKNOWN;
        int mMaxContentDepth = UX_RESTRICTIONS_UNKNOWN;
    }

    /**
     * Container for UX restrictions for a speed range.
     * Speed range is valid only for the {@link CarDrivingStateEvent#DRIVING_STATE_MOVING}.
     */
    private class RestrictionsPerSpeedRange {
        static final int SPEED_INVALID = -1;
        final float mMinSpeed;
        final float mMaxSpeed;
        final int mRestrictions;
        final boolean mRequiresDistractionOptimization;

        RestrictionsPerSpeedRange(float minSpeed, float maxSpeed, int restrictions,
                boolean requiresOpt) {
            mMinSpeed = minSpeed;
            mMaxSpeed = maxSpeed;
            mRestrictions = restrictions;
            mRequiresDistractionOptimization = requiresOpt;
        }

        /**
         * Return if the given speed is in the range of ( {@link #mMinSpeed}, {@link #mMaxSpeed}
         *
         * @param speed Speed to check
         * @return true if in range false if not.
         */
        boolean includes(float speed) {
            if (mMinSpeed != SPEED_INVALID && mMaxSpeed == SPEED_INVALID) {
                // This is for a range [minSpeed, infinity).  If maxSpeed
                // is invalid and mMinSpeed is a valid, this represents a
                // anything greater than mMinSpeed.
                return speed >= mMinSpeed;
            } else {
                return speed >= mMinSpeed && speed < mMaxSpeed;
            }
        }
    }

    /**
     * Container for a list of {@link RestrictionsPerSpeedRange} per driving state.
     */
    private class RestrictionsInfo {
        private List<RestrictionsPerSpeedRange> mRestrictionsList = new ArrayList<>();

        void addRestrictions(RestrictionsPerSpeedRange r) {
            mRestrictionsList.add(r);
        }

        /**
         * Find the restrictions for the given speed.  It finds the range that the given speed falls
         * in and gets the restrictions for that speed.
         */
        @Nullable
        RestrictionsPerSpeedRange findRestrictions(float speed) {
            for (RestrictionsPerSpeedRange r : mRestrictionsList) {
                if (r.includes(speed)) {
                    return r;
                }
            }
            return null;
        }
    }
}
