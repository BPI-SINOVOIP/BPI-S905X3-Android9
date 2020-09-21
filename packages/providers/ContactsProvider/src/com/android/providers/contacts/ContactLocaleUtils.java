/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.providers.contacts;

import android.icu.text.AlphabeticIndex;
import android.icu.text.AlphabeticIndex.ImmutableIndex;
import android.icu.text.Transliterator;
import android.os.LocaleList;
import android.provider.ContactsContract.FullNameStyle;
import android.provider.ContactsContract.PhoneticNameStyle;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;

import com.android.providers.contacts.HanziToPinyin.Token;

import com.google.common.annotations.VisibleForTesting;

import java.lang.Character.UnicodeBlock;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Set;


/**
 * This utility class provides specialized handling for locale specific
 * information: labels, name lookup keys.
 */
public class ContactLocaleUtils {
    public static final String TAG = "ContactLocale";

    private static final boolean DEBUG = false; // don't submit with true

    public static final Locale LOCALE_ARABIC = new Locale("ar");
    public static final Locale LOCALE_GREEK = new Locale("el");
    public static final Locale LOCALE_HEBREW = new Locale("he");
    // Serbian and Ukrainian labels are complementary supersets of Russian
    public static final Locale LOCALE_SERBIAN = new Locale("sr");
    public static final Locale LOCALE_UKRAINIAN = new Locale("uk");
    public static final Locale LOCALE_THAI = new Locale("th");

    // -- Note for adding locales to sDefaultLabelLocales --
    //
    // AlphabeticIndex.getBucketLabel() uses a binary search across
    // the entire label set so care should be taken about growing this
    // set too large. The following set determines for which locales
    // we will show labels other than your primary locale. General rules
    // of thumb for adding a locale: should be a supported locale; and
    // should not be included if from a name it is not deterministic
    // which way to label it (so eg Chinese cannot be added because
    // the labeling of a Chinese character varies between Simplified,
    // Traditional, and Japanese locales). Use English only for all
    // Latin based alphabets. Ukrainian and Serbian are chosen for
    // Cyrillic because their alphabets are complementary supersets
    // of Russian.
    private static final Locale[] sDefaultLabelLocales = new Locale[]{
            Locale.ENGLISH,
            Locale.JAPANESE,
            Locale.KOREAN,
            LOCALE_THAI,
            LOCALE_ARABIC,
            LOCALE_HEBREW,
            LOCALE_GREEK,
            LOCALE_UKRAINIAN,
            LOCALE_SERBIAN,
    };

    @VisibleForTesting
    static void dumpIndex(ImmutableIndex index) {
        final StringBuilder labels = new StringBuilder();
        String sep = "";
        for (int i = 0; i < index.getBucketCount(); i++) {
            labels.append(sep);
            labels.append(index.getBucket(i).getLabel());
            sep = ",";
        }
        Log.d(TAG, "Labels=[" + labels + "]");
    }

    /**
     * This class is the default implementation and should be the base class
     * for other locales.
     *
     * sortKey: same as name
     * nameLookupKeys: none
     * labels: uses ICU AlphabeticIndex for labels and extends by labeling
     *     phone numbers "#".  Eg English labels are: [A-Z], #, " "
     */
    private static class ContactLocaleUtilsBase {
        private static final String EMPTY_STRING = "";
        private static final String NUMBER_STRING = "#";

        protected final ImmutableIndex mAlphabeticIndex;
        private final int mAlphabeticIndexBucketCount;
        private final int mNumberBucketIndex;
        private final boolean mUsePinyinTransliterator;

        public ContactLocaleUtilsBase(LocaleSet systemLocales) {
            mUsePinyinTransliterator = systemLocales.shouldPreferSimplifiedChinese();

            // Build the index buckets based on the current system locale set and
            // sDefaultLabelLocales.
            if (DEBUG) {
                Log.d(TAG, "Building index buckets...");
            }
            final List<Locale> locales = getLocalesForBuckets(systemLocales);

            AlphabeticIndex ai = new AlphabeticIndex(locales.get(0))
                    .setMaxLabelCount(300);
            for (int i = 1; i < locales.size(); i++) {
                ai.addLabels(locales.get(i));
            }

            mAlphabeticIndex = ai.buildImmutableIndex();
            mAlphabeticIndexBucketCount = mAlphabeticIndex.getBucketCount();
            mNumberBucketIndex = mAlphabeticIndexBucketCount - 1;
            if (DEBUG) {
                dumpIndex(mAlphabeticIndex);
            }
        }

        static List<Locale> getLocalesForBuckets(LocaleSet systemLocales) {

            // Create a list of locales that should be used to generate the index buckets.
            // - Source: the system locales and sDefaultLabelLocales.
            // - Rules:
            //   - Don't add the same locale multiple times.
            //   - Also special rules for Chinese (b/31115382):
            //     - Don't add multiple Chinese locales.
            //     - Don't add any Chinese locales after Japanese.

            // First, collect all the locales (allowing duplicates).
            final LocaleList localeList = systemLocales.getAllLocales();

            final List<Locale> locales = new ArrayList<>(
                    localeList.size() + sDefaultLabelLocales.length);
            for (int i = 0; i < localeList.size(); i++) {
                locales.add(localeList.get(i));
            }
            for (int i = 0; i < sDefaultLabelLocales.length; i++) {
                locales.add(sDefaultLabelLocales[i]);
            }

            // Then apply the rules to generate the final list.
            final List<Locale> ret = new ArrayList<>(locales.size());
            boolean allowChinese = true;

            for (int i = 0; i < locales.size(); i++) {
                final Locale locale = locales.get(i);

                if (ret.contains(locale)) {
                    continue;
                }
                if (LocaleSet.isLanguageChinese(locale)) {
                    if (!allowChinese) {
                        continue;
                    }
                    allowChinese = false;
                }
                if (LocaleSet.isLanguageJapanese(locale)) {
                    allowChinese = false;
                }
                if (DEBUG) {
                    Log.d(TAG, "  Adding locale: " + locale.toLanguageTag());
                }
                ret.add(locale);
            }
            return ret;
        }

        public String getSortKey(String name) {
            return name;
        }

        public int getNumberBucketIndex() {
            return mNumberBucketIndex;
        }

        /**
         * Returns the bucket index for the specified string. AlphabeticIndex
         * sorts strings into buckets numbered in order from 0 to N, where the
         * exact value of N depends on how many representative index labels are
         * used in a particular locale. This routine adds one additional bucket
         * for phone numbers. It attempts to detect phone numbers and shifts
         * the bucket indexes returned by AlphabeticIndex in order to make room
         * for the new # bucket, so the returned range becomes 0 to N+1.
         */
        public int getBucketIndex(String name) {
            boolean prefixIsNumeric = false;
            final int length = name.length();
            int offset = 0;
            while (offset < length) {
                int codePoint = Character.codePointAt(name, offset);
                // Ignore standard phone number separators and identify any
                // string that otherwise starts with a number.
                if (Character.isDigit(codePoint)) {
                    prefixIsNumeric = true;
                    break;
                } else if (!Character.isSpaceChar(codePoint) &&
                           codePoint != '+' && codePoint != '(' &&
                           codePoint != ')' && codePoint != '.' &&
                           codePoint != '-' && codePoint != '#') {
                    break;
                }
                offset += Character.charCount(codePoint);
            }
            if (prefixIsNumeric) {
                return mNumberBucketIndex;
            }

            /**
             * ICU 55 AlphabeticIndex doesn't support Simplified Chinese
             * as a secondary locale so it is necessary to use the
             * Pinyin transliterator. We also use this for a Simplified
             * Chinese primary locale because it gives more accurate letter
             * buckets. b/19835686
             */
            if (mUsePinyinTransliterator) {
                name = HanziToPinyin.getInstance().transliterate(name);
            }
            final int bucket = mAlphabeticIndex.getBucketIndex(name);
            if (bucket < 0) {
                return -1;
            }
            if (bucket >= mNumberBucketIndex) {
                return bucket + 1;
            }
            return bucket;
        }

        /**
         * Returns the number of buckets in use (one more than AlphabeticIndex
         * uses, because this class adds a bucket for phone numbers).
         */
        public int getBucketCount() {
            return mAlphabeticIndexBucketCount + 1;
        }

        /**
         * Returns the label for the specified bucket index if a valid index,
         * otherwise returns an empty string. '#' is returned for the phone
         * number bucket; for all others, the AlphabeticIndex label is returned.
         */
        public String getBucketLabel(int bucketIndex) {
            if (bucketIndex < 0 || bucketIndex >= getBucketCount()) {
                return EMPTY_STRING;
            } else if (bucketIndex == mNumberBucketIndex) {
                return NUMBER_STRING;
            } else if (bucketIndex > mNumberBucketIndex) {
                --bucketIndex;
            }
            return mAlphabeticIndex.getBucket(bucketIndex).getLabel();
        }

        @SuppressWarnings("unused")
        public Iterator<String> getNameLookupKeys(String name, int nameStyle) {
            return null;
        }

        public ArrayList<String> getLabels() {
            final int bucketCount = getBucketCount();
            final ArrayList<String> labels = new ArrayList<String>(bucketCount);
            for(int i = 0; i < bucketCount; ++i) {
                labels.add(getBucketLabel(i));
            }
            return labels;
        }
    }

    /**
     * Japanese specific locale overrides.
     *
     * sortKey: unchanged (same as name)
     * nameLookupKeys: unchanged (none)
     * labels: extends default labels by labeling unlabeled CJ characters
     *     with the Japanese character 他 ("misc"). Japanese labels are:
     *     あ, か, さ, た, な, は, ま, や, ら, わ, 他, [A-Z], #, " "
     */
    private static class JapaneseContactUtils extends ContactLocaleUtilsBase {
        // \u4ed6 is Japanese character 他 ("misc")
        private static final String JAPANESE_MISC_LABEL = "\u4ed6";
        private final int mMiscBucketIndex;

        public JapaneseContactUtils(LocaleSet locales) {
            super(locales);
            // Determine which bucket AlphabeticIndex is lumping unclassified
            // Japanese characters into by looking up the bucket index for
            // a representative Kanji/CJK unified ideograph (\u65e5 is the
            // character '日').
            mMiscBucketIndex = super.getBucketIndex("\u65e5");
        }

        // Set of UnicodeBlocks for unified CJK (Chinese) characters and
        // Japanese characters. This includes all code blocks that might
        // contain a character used in Japanese (which is why unified CJK
        // blocks are included but Korean Hangul and jamo are not).
        private static final Set<Character.UnicodeBlock> CJ_BLOCKS;
        static {
            Set<UnicodeBlock> set = new ArraySet<>();
            set.add(UnicodeBlock.HIRAGANA);
            set.add(UnicodeBlock.KATAKANA);
            set.add(UnicodeBlock.KATAKANA_PHONETIC_EXTENSIONS);
            set.add(UnicodeBlock.HALFWIDTH_AND_FULLWIDTH_FORMS);
            set.add(UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS);
            set.add(UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A);
            set.add(UnicodeBlock.CJK_UNIFIED_IDEOGRAPHS_EXTENSION_B);
            set.add(UnicodeBlock.CJK_SYMBOLS_AND_PUNCTUATION);
            set.add(UnicodeBlock.CJK_RADICALS_SUPPLEMENT);
            set.add(UnicodeBlock.CJK_COMPATIBILITY);
            set.add(UnicodeBlock.CJK_COMPATIBILITY_FORMS);
            set.add(UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS);
            set.add(UnicodeBlock.CJK_COMPATIBILITY_IDEOGRAPHS_SUPPLEMENT);
            CJ_BLOCKS = Collections.unmodifiableSet(set);
        }

        /**
         * Helper routine to identify unlabeled Chinese or Japanese characters
         * to put in a 'misc' bucket.
         *
         * @return true if the specified Unicode code point is Chinese or
         *              Japanese
         */
        private static boolean isChineseOrJapanese(int codePoint) {
            return CJ_BLOCKS.contains(UnicodeBlock.of(codePoint));
        }

        /**
         * Returns the bucket index for the specified string. Adds an
         * additional 'misc' bucket for Kanji characters to the base class set.
         */
        @Override
        public int getBucketIndex(String name) {
            final int bucketIndex = super.getBucketIndex(name);
            if ((bucketIndex == mMiscBucketIndex &&
                 !isChineseOrJapanese(Character.codePointAt(name, 0))) ||
                bucketIndex > mMiscBucketIndex) {
                return bucketIndex + 1;
            }
            return bucketIndex;
        }

        /**
         * Returns the number of buckets in use (one more than the base class
         * uses, because this class adds a bucket for Kanji).
         */
        @Override
        public int getBucketCount() {
            return super.getBucketCount() + 1;
        }

        /**
         * Returns the label for the specified bucket index if a valid index,
         * otherwise returns an empty string. '他' is returned for unclassified
         * Kanji; for all others, the label determined by the base class is
         * returned.
         */
        @Override
        public String getBucketLabel(int bucketIndex) {
            if (bucketIndex == mMiscBucketIndex) {
                return JAPANESE_MISC_LABEL;
            } else if (bucketIndex > mMiscBucketIndex) {
                --bucketIndex;
            }
            return super.getBucketLabel(bucketIndex);
        }

        @Override
        public Iterator<String> getNameLookupKeys(String name, int nameStyle) {
            // Hiragana and Katakana will be positively identified as Japanese.
            if (nameStyle == PhoneticNameStyle.JAPANESE) {
                return getRomajiNameLookupKeys(name);
            }
            return null;
        }

        private static boolean mInitializedTransliterator;
        private static Transliterator mJapaneseTransliterator;

        private static Transliterator getJapaneseTransliterator() {
            synchronized(JapaneseContactUtils.class) {
                if (!mInitializedTransliterator) {
                    mInitializedTransliterator = true;
                    Transliterator t = null;
                    try {
                        t = Transliterator.getInstance("Hiragana-Latin; Katakana-Latin;"
                                + " Latin-Ascii");
                    } catch (IllegalArgumentException e) {
                        Log.w(TAG, "Hiragana/Katakana-Latin transliterator data"
                                + " is missing");
                    }
                    mJapaneseTransliterator = t;
                }
                return mJapaneseTransliterator;
            }
        }

        public static Iterator<String> getRomajiNameLookupKeys(String name) {
            final Transliterator t = getJapaneseTransliterator();
            if (t == null) {
                return null;
            }
            final String romajiName = t.transliterate(name);
            if (TextUtils.isEmpty(romajiName) ||
                    TextUtils.equals(name, romajiName)) {
                return null;
            }
            final ArraySet<String> keys = new ArraySet<>();
            keys.add(romajiName);
            return keys.iterator();
        }

        /**
         * Returns the number for "#" bucket index.
         * Adds an additional 'misc' bucket for Kanji characters to the base class set.
         */
        @Override
        public int getNumberBucketIndex() {
            final int numberBucketIndex = super.getNumberBucketIndex();
            if (numberBucketIndex > mMiscBucketIndex) {
                return numberBucketIndex + 1;
            }
            return numberBucketIndex;
        }
    }

    /**
     * Simplified Chinese specific locale overrides. Uses ICU Transliterator
     * for generating pinyin transliteration.
     *
     * sortKey: unchanged (same as name)
     * nameLookupKeys: adds additional name lookup keys
     *     - Chinese character's pinyin and pinyin's initial character.
     *     - Latin word and initial character.
     * labels: unchanged
     *     Simplified Chinese labels are the same as English: [A-Z], #, " "
     */
    private static class SimplifiedChineseContactUtils
        extends ContactLocaleUtilsBase {
        public SimplifiedChineseContactUtils(LocaleSet locales) {
            super(locales);
        }

        @Override
        public Iterator<String> getNameLookupKeys(String name, int nameStyle) {
            if (nameStyle != FullNameStyle.JAPANESE &&
                    nameStyle != FullNameStyle.KOREAN) {
                return getPinyinNameLookupKeys(name);
            }
            return null;
        }

        public static Iterator<String> getPinyinNameLookupKeys(String name) {
            // TODO : Reduce the object allocation.
            ArraySet<String> keys = new ArraySet<>();
            ArrayList<Token> tokens = HanziToPinyin.getInstance().getTokens(name);
            final int tokenCount = tokens.size();
            final StringBuilder keyPinyin = new StringBuilder();
            final StringBuilder keyInitial = new StringBuilder();
            // There is no space among the Chinese Characters, the variant name
            // lookup key wouldn't work for Chinese. The keyOriginal is used to
            // build the lookup keys for itself.
            final StringBuilder keyOriginal = new StringBuilder();
            for (int i = tokenCount - 1; i >= 0; i--) {
                final Token token = tokens.get(i);
                if (Token.UNKNOWN == token.type) {
                    continue;
                }
                if (Token.PINYIN == token.type) {
                    keyPinyin.insert(0, token.target);
                    keyInitial.insert(0, token.target.charAt(0));
                } else if (Token.LATIN == token.type) {
                    // Avoid adding space at the end of String.
                    if (keyPinyin.length() > 0) {
                        keyPinyin.insert(0, ' ');
                    }
                    if (keyOriginal.length() > 0) {
                        keyOriginal.insert(0, ' ');
                    }
                    keyPinyin.insert(0, token.source);
                    keyInitial.insert(0, token.source.charAt(0));
                }
                keyOriginal.insert(0, token.source);
                keys.add(keyOriginal.toString());
                keys.add(keyPinyin.toString());
                keys.add(keyInitial.toString());
            }
            return keys.iterator();
        }
    }

    private static ContactLocaleUtils sSingleton;

    private final LocaleSet mLocales;
    private final ContactLocaleUtilsBase mUtils;

    private ContactLocaleUtils(LocaleSet locales) {
        if (locales == null) {
            mLocales = LocaleSet.newDefault();
        } else {
            mLocales = locales;
        }
        if (mLocales.shouldPreferJapanese()) {
            mUtils = new JapaneseContactUtils(mLocales);
        } else if (mLocales.shouldPreferSimplifiedChinese()) {
            mUtils = new SimplifiedChineseContactUtils(mLocales);
        } else {
            mUtils = new ContactLocaleUtilsBase(mLocales);
        }
        Log.i(TAG, "AddressBook Labels [" + mLocales.toString() + "]: "
                + getLabels().toString());
    }

    public boolean isLocale(LocaleSet locales) {
        return mLocales.equals(locales);
    }

    public static synchronized ContactLocaleUtils getInstance() {
        if (sSingleton == null) {
            sSingleton = new ContactLocaleUtils(LocaleSet.newDefault());
        }
        return sSingleton;
    }

    @VisibleForTesting
    public static ContactLocaleUtils newInstanceForTest(Locale... locales) {
        return new ContactLocaleUtils(LocaleSet.newForTest(locales));
    }

    @VisibleForTesting
    public static synchronized void setLocaleForTest(Locale... locales) {
        setLocales(LocaleSet.newForTest(locales));
    }

    public static synchronized void setLocales(LocaleSet locales) {
        if (sSingleton == null || !sSingleton.isLocale(locales)) {
            if (DEBUG) {
                Log.d(TAG, "Setting locale(s) to " + locales);
            }
            sSingleton = new ContactLocaleUtils(locales);
        }
    }

    public String getSortKey(String name, int nameStyle) {
        return mUtils.getSortKey(name);
    }

    public int getBucketIndex(String name) {
        return mUtils.getBucketIndex(name);
    }

    public int getNumberBucketIndex() {
        return mUtils.getNumberBucketIndex();
    }

    public int getBucketCount() {
        return mUtils.getBucketCount();
    }

    public String getBucketLabel(int bucketIndex) {
        return mUtils.getBucketLabel(bucketIndex);
    }

    public String getLabel(String name) {
        return getBucketLabel(getBucketIndex(name));
    }

    public ArrayList<String> getLabels() {
        return mUtils.getLabels();
    }

    /**
     *  Determine which utility should be used for generating NameLookupKey.
     *  (ie, whether we generate Romaji or Pinyin lookup keys or not)
     *
     *  Hiragana and Katakana are tagged as JAPANESE; Kanji is unclassified
     *  and tagged as CJK. For Hiragana/Katakana names, generate Romaji
     *  lookup keys when not in a Chinese or Korean locale.
     *
     *  Otherwise, use the default behavior of that locale:
     *  a. For Japan, generate Romaji lookup keys for Hiragana/Katakana.
     *  b. For Simplified Chinese locale, generate Pinyin lookup keys.
     */
    public Iterator<String> getNameLookupKeys(String name, int nameStyle) {
        if (!mLocales.isPrimaryLocaleCJK()) {
            if (mLocales.shouldPreferSimplifiedChinese()) {
                if (nameStyle == FullNameStyle.CHINESE ||
                        nameStyle == FullNameStyle.CJK) {
                    return SimplifiedChineseContactUtils.getPinyinNameLookupKeys(name);
                }
            } else {
                if (nameStyle == FullNameStyle.JAPANESE) {
                    return JapaneseContactUtils.getRomajiNameLookupKeys(name);
                }
            }
        }
        return mUtils.getNameLookupKeys(name, nameStyle);
    }

}
