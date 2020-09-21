/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.internal.inputmethod;

import android.annotation.Nullable;
import android.content.Context;
import android.content.pm.PackageManager;
import android.text.TextUtils;
import android.util.Log;
import android.util.Printer;
import android.util.Slog;
import android.view.inputmethod.InputMethodInfo;
import android.view.inputmethod.InputMethodSubtype;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.inputmethod.InputMethodUtils.InputMethodSettings;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Objects;
import java.util.TreeMap;

/**
 * InputMethodSubtypeSwitchingController controls the switching behavior of the subtypes.
 * <p>
 * This class is designed to be used from and only from
 * {@link com.android.server.InputMethodManagerService} by using
 * {@link com.android.server.InputMethodManagerService#mMethodMap} as a global lock.
 * </p>
 */
public class InputMethodSubtypeSwitchingController {
    private static final String TAG = InputMethodSubtypeSwitchingController.class.getSimpleName();
    private static final boolean DEBUG = false;
    private static final int NOT_A_SUBTYPE_ID = InputMethodUtils.NOT_A_SUBTYPE_ID;

    public static class ImeSubtypeListItem implements Comparable<ImeSubtypeListItem> {
        public final CharSequence mImeName;
        public final CharSequence mSubtypeName;
        public final InputMethodInfo mImi;
        public final int mSubtypeId;
        public final boolean mIsSystemLocale;
        public final boolean mIsSystemLanguage;

        public ImeSubtypeListItem(CharSequence imeName, CharSequence subtypeName,
                InputMethodInfo imi, int subtypeId, String subtypeLocale, String systemLocale) {
            mImeName = imeName;
            mSubtypeName = subtypeName;
            mImi = imi;
            mSubtypeId = subtypeId;
            if (TextUtils.isEmpty(subtypeLocale)) {
                mIsSystemLocale = false;
                mIsSystemLanguage = false;
            } else {
                mIsSystemLocale = subtypeLocale.equals(systemLocale);
                if (mIsSystemLocale) {
                    mIsSystemLanguage = true;
                } else {
                    // TODO: Use Locale#getLanguage or Locale#toLanguageTag
                    final String systemLanguage = parseLanguageFromLocaleString(systemLocale);
                    final String subtypeLanguage = parseLanguageFromLocaleString(subtypeLocale);
                    mIsSystemLanguage = systemLanguage.length() >= 2 &&
                            systemLanguage.equals(subtypeLanguage);
                }
            }
        }

        /**
         * Returns the language component of a given locale string.
         * TODO: Use {@link Locale#getLanguage()} instead.
         */
        private static String parseLanguageFromLocaleString(final String locale) {
            final int idx = locale.indexOf('_');
            if (idx < 0) {
                return locale;
            } else {
                return locale.substring(0, idx);
            }
        }

        private static int compareNullableCharSequences(@Nullable CharSequence c1,
                @Nullable CharSequence c2) {
            // For historical reasons, an empty text needs to put at the last.
            final boolean empty1 = TextUtils.isEmpty(c1);
            final boolean empty2 = TextUtils.isEmpty(c2);
            if (empty1 || empty2) {
                return (empty1 ? 1 : 0) - (empty2 ? 1 : 0);
            }
            return c1.toString().compareTo(c2.toString());
        }

        /**
         * Compares this object with the specified object for order. The fields of this class will
         * be compared in the following order.
         * <ol>
         *   <li>{@link #mImeName}</li>
         *   <li>{@link #mIsSystemLocale}</li>
         *   <li>{@link #mIsSystemLanguage}</li>
         *   <li>{@link #mSubtypeName}</li>
         * </ol>
         * Note: this class has a natural ordering that is inconsistent with {@link #equals(Object).
         * This method doesn't compare {@link #mSubtypeId} but {@link #equals(Object)} does.
         *
         * @param other the object to be compared.
         * @return a negative integer, zero, or positive integer as this object is less than, equal
         *         to, or greater than the specified <code>other</code> object.
         */
        @Override
        public int compareTo(ImeSubtypeListItem other) {
            int result = compareNullableCharSequences(mImeName, other.mImeName);
            if (result != 0) {
                return result;
            }
            // Subtype that has the same locale of the system's has higher priority.
            result = (mIsSystemLocale ? -1 : 0) - (other.mIsSystemLocale ? -1 : 0);
            if (result != 0) {
                return result;
            }
            // Subtype that has the same language of the system's has higher priority.
            result = (mIsSystemLanguage ? -1 : 0) - (other.mIsSystemLanguage ? -1 : 0);
            if (result != 0) {
                return result;
            }
            return compareNullableCharSequences(mSubtypeName, other.mSubtypeName);
        }

        @Override
        public String toString() {
            return "ImeSubtypeListItem{"
                    + "mImeName=" + mImeName
                    + " mSubtypeName=" + mSubtypeName
                    + " mSubtypeId=" + mSubtypeId
                    + " mIsSystemLocale=" + mIsSystemLocale
                    + " mIsSystemLanguage=" + mIsSystemLanguage
                    + "}";
        }

        @Override
        public boolean equals(Object o) {
            if (o == this) {
                return true;
            }
            if (o instanceof ImeSubtypeListItem) {
                final ImeSubtypeListItem that = (ImeSubtypeListItem)o;
                return Objects.equals(this.mImi, that.mImi) && this.mSubtypeId == that.mSubtypeId;
            }
            return false;
        }
    }

    private static class InputMethodAndSubtypeList {
        private final Context mContext;
        // Used to load label
        private final PackageManager mPm;
        private final String mSystemLocaleStr;
        private final InputMethodSettings mSettings;

        public InputMethodAndSubtypeList(Context context, InputMethodSettings settings) {
            mContext = context;
            mSettings = settings;
            mPm = context.getPackageManager();
            final Locale locale = context.getResources().getConfiguration().locale;
            mSystemLocaleStr = locale != null ? locale.toString() : "";
        }

        private final TreeMap<InputMethodInfo, List<InputMethodSubtype>> mSortedImmis =
                new TreeMap<>(
                        new Comparator<InputMethodInfo>() {
                            @Override
                            public int compare(InputMethodInfo imi1, InputMethodInfo imi2) {
                                if (imi2 == null)
                                    return 0;
                                if (imi1 == null)
                                    return 1;
                                if (mPm == null) {
                                    return imi1.getId().compareTo(imi2.getId());
                                }
                                CharSequence imiId1 = imi1.loadLabel(mPm) + "/" + imi1.getId();
                                CharSequence imiId2 = imi2.loadLabel(mPm) + "/" + imi2.getId();
                                return imiId1.toString().compareTo(imiId2.toString());
                            }
                        });

        public List<ImeSubtypeListItem> getSortedInputMethodAndSubtypeList(
                boolean includeAuxiliarySubtypes, boolean isScreenLocked) {
            final ArrayList<ImeSubtypeListItem> imList = new ArrayList<>();
            final HashMap<InputMethodInfo, List<InputMethodSubtype>> immis =
                    mSettings.getExplicitlyOrImplicitlyEnabledInputMethodsAndSubtypeListLocked(
                            mContext);
            if (immis == null || immis.size() == 0) {
                return Collections.emptyList();
            }
            if (isScreenLocked && includeAuxiliarySubtypes) {
                if (DEBUG) {
                    Slog.w(TAG, "Auxiliary subtypes are not allowed to be shown in lock screen.");
                }
                includeAuxiliarySubtypes = false;
            }
            mSortedImmis.clear();
            mSortedImmis.putAll(immis);
            for (InputMethodInfo imi : mSortedImmis.keySet()) {
                if (imi == null) {
                    continue;
                }
                List<InputMethodSubtype> explicitlyOrImplicitlyEnabledSubtypeList = immis.get(imi);
                HashSet<String> enabledSubtypeSet = new HashSet<>();
                for (InputMethodSubtype subtype : explicitlyOrImplicitlyEnabledSubtypeList) {
                    enabledSubtypeSet.add(String.valueOf(subtype.hashCode()));
                }
                final CharSequence imeLabel = imi.loadLabel(mPm);
                if (enabledSubtypeSet.size() > 0) {
                    final int subtypeCount = imi.getSubtypeCount();
                    if (DEBUG) {
                        Slog.v(TAG, "Add subtypes: " + subtypeCount + ", " + imi.getId());
                    }
                    for (int j = 0; j < subtypeCount; ++j) {
                        final InputMethodSubtype subtype = imi.getSubtypeAt(j);
                        final String subtypeHashCode = String.valueOf(subtype.hashCode());
                        // We show all enabled IMEs and subtypes when an IME is shown.
                        if (enabledSubtypeSet.contains(subtypeHashCode)
                                && (includeAuxiliarySubtypes || !subtype.isAuxiliary())) {
                            final CharSequence subtypeLabel =
                                    subtype.overridesImplicitlyEnabledSubtype() ? null : subtype
                                            .getDisplayName(mContext, imi.getPackageName(),
                                                    imi.getServiceInfo().applicationInfo);
                            imList.add(new ImeSubtypeListItem(imeLabel,
                                    subtypeLabel, imi, j, subtype.getLocale(), mSystemLocaleStr));

                            // Removing this subtype from enabledSubtypeSet because we no
                            // longer need to add an entry of this subtype to imList to avoid
                            // duplicated entries.
                            enabledSubtypeSet.remove(subtypeHashCode);
                        }
                    }
                } else {
                    imList.add(new ImeSubtypeListItem(imeLabel, null, imi, NOT_A_SUBTYPE_ID, null,
                            mSystemLocaleStr));
                }
            }
            Collections.sort(imList);
            return imList;
        }
    }

    private static int calculateSubtypeId(InputMethodInfo imi, InputMethodSubtype subtype) {
        return subtype != null ? InputMethodUtils.getSubtypeIdFromHashCode(imi,
                subtype.hashCode()) : NOT_A_SUBTYPE_ID;
    }

    private static class StaticRotationList {
        private final List<ImeSubtypeListItem> mImeSubtypeList;
        public StaticRotationList(final List<ImeSubtypeListItem> imeSubtypeList) {
            mImeSubtypeList = imeSubtypeList;
        }

        /**
         * Returns the index of the specified input method and subtype in the given list.
         * @param imi The {@link InputMethodInfo} to be searched.
         * @param subtype The {@link InputMethodSubtype} to be searched. null if the input method
         * does not have a subtype.
         * @return The index in the given list. -1 if not found.
         */
        private int getIndex(InputMethodInfo imi, InputMethodSubtype subtype) {
            final int currentSubtypeId = calculateSubtypeId(imi, subtype);
            final int N = mImeSubtypeList.size();
            for (int i = 0; i < N; ++i) {
                final ImeSubtypeListItem isli = mImeSubtypeList.get(i);
                // Skip until the current IME/subtype is found.
                if (imi.equals(isli.mImi) && isli.mSubtypeId == currentSubtypeId) {
                    return i;
                }
            }
            return -1;
        }

        /**
         * Provides the basic operation to implement bi-directional IME rotation.
         * @param onlyCurrentIme {@code true} to limit the search space to IME subtypes that belong
         * to {@code imi}.
         * @param imi {@link InputMethodInfo} that will be used in conjunction with {@code subtype}
         * from which we find the adjacent IME subtype.
         * @param subtype {@link InputMethodSubtype} that will be used in conjunction with
         * {@code imi} from which we find the next IME subtype.  {@code null} if the input method
         * does not have a subtype.
         * @param forward {@code true} to do forward search the next IME subtype. Specify
         * {@code false} to do backward search.
         * @return The IME subtype found. {@code null} if no IME subtype is found.
         */
        @Nullable
        public ImeSubtypeListItem getNextInputMethodLocked(boolean onlyCurrentIme,
                InputMethodInfo imi, @Nullable InputMethodSubtype subtype, boolean forward) {
            if (imi == null) {
                return null;
            }
            if (mImeSubtypeList.size() <= 1) {
                return null;
            }
            final int currentIndex = getIndex(imi, subtype);
            if (currentIndex < 0) {
                return null;
            }
            final int N = mImeSubtypeList.size();
            for (int i = 1; i < N; ++i) {
                // Start searching the next IME/subtype from +/- 1 indices.
                final int offset = forward ? i : N - i;
                final int candidateIndex = (currentIndex + offset) % N;
                final ImeSubtypeListItem candidate = mImeSubtypeList.get(candidateIndex);
                // Skip if searching inside the current IME only, but the candidate is not
                // the current IME.
                if (onlyCurrentIme && !imi.equals(candidate.mImi)) {
                    continue;
                }
                return candidate;
            }
            return null;
        }

        protected void dump(final Printer pw, final String prefix) {
            final int N = mImeSubtypeList.size();
            for (int i = 0; i < N; ++i) {
                final int rank = i;
                final ImeSubtypeListItem item = mImeSubtypeList.get(i);
                pw.println(prefix + "rank=" + rank + " item=" + item);
            }
        }
    }

    private static class DynamicRotationList {
        private static final String TAG = DynamicRotationList.class.getSimpleName();
        private final List<ImeSubtypeListItem> mImeSubtypeList;
        private final int[] mUsageHistoryOfSubtypeListItemIndex;

        private DynamicRotationList(final List<ImeSubtypeListItem> imeSubtypeListItems) {
            mImeSubtypeList = imeSubtypeListItems;
            mUsageHistoryOfSubtypeListItemIndex = new int[mImeSubtypeList.size()];
            final int N = mImeSubtypeList.size();
            for (int i = 0; i < N; i++) {
                mUsageHistoryOfSubtypeListItemIndex[i] = i;
            }
        }

        /**
         * Returns the index of the specified object in
         * {@link #mUsageHistoryOfSubtypeListItemIndex}.
         * <p>We call the index of {@link #mUsageHistoryOfSubtypeListItemIndex} as "Usage Rank"
         * so as not to be confused with the index in {@link #mImeSubtypeList}.
         * @return -1 when the specified item doesn't belong to {@link #mImeSubtypeList} actually.
         */
        private int getUsageRank(final InputMethodInfo imi, InputMethodSubtype subtype) {
            final int currentSubtypeId = calculateSubtypeId(imi, subtype);
            final int N = mUsageHistoryOfSubtypeListItemIndex.length;
            for (int usageRank = 0; usageRank < N; usageRank++) {
                final int subtypeListItemIndex = mUsageHistoryOfSubtypeListItemIndex[usageRank];
                final ImeSubtypeListItem subtypeListItem =
                        mImeSubtypeList.get(subtypeListItemIndex);
                if (subtypeListItem.mImi.equals(imi) &&
                        subtypeListItem.mSubtypeId == currentSubtypeId) {
                    return usageRank;
                }
            }
            // Not found in the known IME/Subtype list.
            return -1;
        }

        public void onUserAction(InputMethodInfo imi, InputMethodSubtype subtype) {
            final int currentUsageRank = getUsageRank(imi, subtype);
            // Do nothing if currentUsageRank == -1 (not found), or currentUsageRank == 0
            if (currentUsageRank <= 0) {
                return;
            }
            final int currentItemIndex = mUsageHistoryOfSubtypeListItemIndex[currentUsageRank];
            System.arraycopy(mUsageHistoryOfSubtypeListItemIndex, 0,
                    mUsageHistoryOfSubtypeListItemIndex, 1, currentUsageRank);
            mUsageHistoryOfSubtypeListItemIndex[0] = currentItemIndex;
        }

        /**
         * Provides the basic operation to implement bi-directional IME rotation.
         * @param onlyCurrentIme {@code true} to limit the search space to IME subtypes that belong
         * to {@code imi}.
         * @param imi {@link InputMethodInfo} that will be used in conjunction with {@code subtype}
         * from which we find the adjacent IME subtype.
         * @param subtype {@link InputMethodSubtype} that will be used in conjunction with
         * {@code imi} from which we find the next IME subtype.  {@code null} if the input method
         * does not have a subtype.
         * @param forward {@code true} to do forward search the next IME subtype. Specify
         * {@code false} to do backward search.
         * @return The IME subtype found. {@code null} if no IME subtype is found.
         */
        @Nullable
        public ImeSubtypeListItem getNextInputMethodLocked(boolean onlyCurrentIme,
                InputMethodInfo imi, @Nullable InputMethodSubtype subtype, boolean forward) {
            int currentUsageRank = getUsageRank(imi, subtype);
            if (currentUsageRank < 0) {
                if (DEBUG) {
                    Slog.d(TAG, "IME/subtype is not found: " + imi.getId() + ", " + subtype);
                }
                return null;
            }
            final int N = mUsageHistoryOfSubtypeListItemIndex.length;
            for (int i = 1; i < N; i++) {
                final int offset = forward ? i : N - i;
                final int subtypeListItemRank = (currentUsageRank + offset) % N;
                final int subtypeListItemIndex =
                        mUsageHistoryOfSubtypeListItemIndex[subtypeListItemRank];
                final ImeSubtypeListItem subtypeListItem =
                        mImeSubtypeList.get(subtypeListItemIndex);
                if (onlyCurrentIme && !imi.equals(subtypeListItem.mImi)) {
                    continue;
                }
                return subtypeListItem;
            }
            return null;
        }

        protected void dump(final Printer pw, final String prefix) {
            for (int i = 0; i < mUsageHistoryOfSubtypeListItemIndex.length; ++i) {
                final int rank = mUsageHistoryOfSubtypeListItemIndex[i];
                final ImeSubtypeListItem item = mImeSubtypeList.get(i);
                pw.println(prefix + "rank=" + rank + " item=" + item);
            }
        }
    }

    @VisibleForTesting
    public static class ControllerImpl {
        private final DynamicRotationList mSwitchingAwareRotationList;
        private final StaticRotationList mSwitchingUnawareRotationList;

        public static ControllerImpl createFrom(final ControllerImpl currentInstance,
                final List<ImeSubtypeListItem> sortedEnabledItems) {
            DynamicRotationList switchingAwareRotationList = null;
            {
                final List<ImeSubtypeListItem> switchingAwareImeSubtypes =
                        filterImeSubtypeList(sortedEnabledItems,
                                true /* supportsSwitchingToNextInputMethod */);
                if (currentInstance != null &&
                        currentInstance.mSwitchingAwareRotationList != null &&
                        Objects.equals(currentInstance.mSwitchingAwareRotationList.mImeSubtypeList,
                                switchingAwareImeSubtypes)) {
                    // Can reuse the current instance.
                    switchingAwareRotationList = currentInstance.mSwitchingAwareRotationList;
                }
                if (switchingAwareRotationList == null) {
                    switchingAwareRotationList = new DynamicRotationList(switchingAwareImeSubtypes);
                }
            }

            StaticRotationList switchingUnawareRotationList = null;
            {
                final List<ImeSubtypeListItem> switchingUnawareImeSubtypes = filterImeSubtypeList(
                        sortedEnabledItems, false /* supportsSwitchingToNextInputMethod */);
                if (currentInstance != null &&
                        currentInstance.mSwitchingUnawareRotationList != null &&
                        Objects.equals(
                                currentInstance.mSwitchingUnawareRotationList.mImeSubtypeList,
                                switchingUnawareImeSubtypes)) {
                    // Can reuse the current instance.
                    switchingUnawareRotationList = currentInstance.mSwitchingUnawareRotationList;
                }
                if (switchingUnawareRotationList == null) {
                    switchingUnawareRotationList =
                            new StaticRotationList(switchingUnawareImeSubtypes);
                }
            }

            return new ControllerImpl(switchingAwareRotationList, switchingUnawareRotationList);
        }

        private ControllerImpl(final DynamicRotationList switchingAwareRotationList,
                final StaticRotationList switchingUnawareRotationList) {
            mSwitchingAwareRotationList = switchingAwareRotationList;
            mSwitchingUnawareRotationList = switchingUnawareRotationList;
        }

        /**
         * Provides the basic operation to implement bi-directional IME rotation.
         * @param onlyCurrentIme {@code true} to limit the search space to IME subtypes that belong
         * to {@code imi}.
         * @param imi {@link InputMethodInfo} that will be used in conjunction with {@code subtype}
         * from which we find the adjacent IME subtype.
         * @param subtype {@link InputMethodSubtype} that will be used in conjunction with
         * {@code imi} from which we find the next IME subtype.  {@code null} if the input method
         * does not have a subtype.
         * @param forward {@code true} to do forward search the next IME subtype. Specify
         * {@code false} to do backward search.
         * @return The IME subtype found. {@code null} if no IME subtype is found.
         */
        @Nullable
        public ImeSubtypeListItem getNextInputMethod(boolean onlyCurrentIme, InputMethodInfo imi,
                @Nullable InputMethodSubtype subtype, boolean forward) {
            if (imi == null) {
                return null;
            }
            if (imi.supportsSwitchingToNextInputMethod()) {
                return mSwitchingAwareRotationList.getNextInputMethodLocked(onlyCurrentIme, imi,
                        subtype, forward);
            } else {
                return mSwitchingUnawareRotationList.getNextInputMethodLocked(onlyCurrentIme, imi,
                        subtype, forward);
            }
        }

        public void onUserActionLocked(InputMethodInfo imi, InputMethodSubtype subtype) {
            if (imi == null) {
                return;
            }
            if (imi.supportsSwitchingToNextInputMethod()) {
                mSwitchingAwareRotationList.onUserAction(imi, subtype);
            }
        }

        private static List<ImeSubtypeListItem> filterImeSubtypeList(
                final List<ImeSubtypeListItem> items,
                final boolean supportsSwitchingToNextInputMethod) {
            final ArrayList<ImeSubtypeListItem> result = new ArrayList<>();
            final int ALL_ITEMS_COUNT = items.size();
            for (int i = 0; i < ALL_ITEMS_COUNT; i++) {
                final ImeSubtypeListItem item = items.get(i);
                if (item.mImi.supportsSwitchingToNextInputMethod() ==
                        supportsSwitchingToNextInputMethod) {
                    result.add(item);
                }
            }
            return result;
        }

        protected void dump(final Printer pw) {
            pw.println("    mSwitchingAwareRotationList:");
            mSwitchingAwareRotationList.dump(pw, "      ");
            pw.println("    mSwitchingUnawareRotationList:");
            mSwitchingUnawareRotationList.dump(pw, "      ");
        }
    }

    private final InputMethodSettings mSettings;
    private InputMethodAndSubtypeList mSubtypeList;
    private ControllerImpl mController;

    private InputMethodSubtypeSwitchingController(InputMethodSettings settings, Context context) {
        mSettings = settings;
        resetCircularListLocked(context);
    }

    public static InputMethodSubtypeSwitchingController createInstanceLocked(
            InputMethodSettings settings, Context context) {
        return new InputMethodSubtypeSwitchingController(settings, context);
    }

    public void onUserActionLocked(InputMethodInfo imi, InputMethodSubtype subtype) {
        if (mController == null) {
            if (DEBUG) {
                Log.e(TAG, "mController shouldn't be null.");
            }
            return;
        }
        mController.onUserActionLocked(imi, subtype);
    }

    public void resetCircularListLocked(Context context) {
        mSubtypeList = new InputMethodAndSubtypeList(context, mSettings);
        mController = ControllerImpl.createFrom(mController,
                mSubtypeList.getSortedInputMethodAndSubtypeList(
                        false /* includeAuxiliarySubtypes */, false /* isScreenLocked */));
    }

    public ImeSubtypeListItem getNextInputMethodLocked(boolean onlyCurrentIme, InputMethodInfo imi,
            InputMethodSubtype subtype, boolean forward) {
        if (mController == null) {
            if (DEBUG) {
                Log.e(TAG, "mController shouldn't be null.");
            }
            return null;
        }
        return mController.getNextInputMethod(onlyCurrentIme, imi, subtype, forward);
    }

    public List<ImeSubtypeListItem> getSortedInputMethodAndSubtypeListLocked(
            boolean includingAuxiliarySubtypes, boolean isScreenLocked) {
        return mSubtypeList.getSortedInputMethodAndSubtypeList(
                includingAuxiliarySubtypes, isScreenLocked);
    }

    public void dump(final Printer pw) {
        if (mController != null) {
            mController.dump(pw);
        } else {
            pw.println("    mController=null");
        }
    }
}
