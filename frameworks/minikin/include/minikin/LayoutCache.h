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

#ifndef MINIKIN_LAYOUT_CACHE_H
#define MINIKIN_LAYOUT_CACHE_H

#include "minikin/Layout.h"

#include <mutex>

#include <utils/JenkinsHash.h>
#include <utils/LruCache.h>

namespace minikin {

// Layout cache datatypes
class LayoutCacheKey {
public:
    LayoutCacheKey(const U16StringPiece& text, const Range& range, const MinikinPaint& paint,
                   bool dir, StartHyphenEdit startHyphen, EndHyphenEdit endHyphen)
            : mChars(text.data()),
              mNchars(text.size()),
              mStart(range.getStart()),
              mCount(range.getLength()),
              mId(paint.font->getId()),
              mStyle(paint.fontStyle),
              mSize(paint.size),
              mScaleX(paint.scaleX),
              mSkewX(paint.skewX),
              mLetterSpacing(paint.letterSpacing),
              mWordSpacing(paint.wordSpacing),
              mPaintFlags(paint.paintFlags),
              mLocaleListId(paint.localeListId),
              mFamilyVariant(paint.familyVariant),
              mStartHyphen(startHyphen),
              mEndHyphen(endHyphen),
              mIsRtl(dir),
              mHash(computeHash()) {}

    bool operator==(const LayoutCacheKey& o) const {
        return mId == o.mId && mStart == o.mStart && mCount == o.mCount && mStyle == o.mStyle &&
               mSize == o.mSize && mScaleX == o.mScaleX && mSkewX == o.mSkewX &&
               mLetterSpacing == o.mLetterSpacing && mWordSpacing == o.mWordSpacing &&
               mPaintFlags == o.mPaintFlags && mLocaleListId == o.mLocaleListId &&
               mFamilyVariant == o.mFamilyVariant && mStartHyphen == o.mStartHyphen &&
               mEndHyphen == o.mEndHyphen && mIsRtl == o.mIsRtl && mNchars == o.mNchars &&
               !memcmp(mChars, o.mChars, mNchars * sizeof(uint16_t));
    }

    android::hash_t hash() const { return mHash; }

    void copyText() {
        uint16_t* charsCopy = new uint16_t[mNchars];
        memcpy(charsCopy, mChars, mNchars * sizeof(uint16_t));
        mChars = charsCopy;
    }
    void freeText() {
        delete[] mChars;
        mChars = NULL;
    }

    void doLayout(Layout* layout, const MinikinPaint& paint) const {
        layout->mAdvances.resize(mCount, 0);
        layout->mExtents.resize(mCount);
        layout->doLayoutRun(mChars, mStart, mCount, mNchars, mIsRtl, paint, mStartHyphen,
                            mEndHyphen);
    }

    uint32_t getMemoryUsage() const { return sizeof(LayoutCacheKey) + sizeof(uint16_t) * mNchars; }

private:
    const uint16_t* mChars;
    size_t mNchars;
    size_t mStart;
    size_t mCount;
    uint32_t mId;  // for the font collection
    FontStyle mStyle;
    float mSize;
    float mScaleX;
    float mSkewX;
    float mLetterSpacing;
    float mWordSpacing;
    int32_t mPaintFlags;
    uint32_t mLocaleListId;
    FontFamily::Variant mFamilyVariant;
    StartHyphenEdit mStartHyphen;
    EndHyphenEdit mEndHyphen;
    bool mIsRtl;
    // Note: any fields added to MinikinPaint must also be reflected here.
    // TODO: language matching (possibly integrate into style)
    android::hash_t mHash;

    android::hash_t computeHash() const {
        uint32_t hash = android::JenkinsHashMix(0, mId);
        hash = android::JenkinsHashMix(hash, mStart);
        hash = android::JenkinsHashMix(hash, mCount);
        hash = android::JenkinsHashMix(hash, android::hash_type(mStyle.identifier()));
        hash = android::JenkinsHashMix(hash, android::hash_type(mSize));
        hash = android::JenkinsHashMix(hash, android::hash_type(mScaleX));
        hash = android::JenkinsHashMix(hash, android::hash_type(mSkewX));
        hash = android::JenkinsHashMix(hash, android::hash_type(mLetterSpacing));
        hash = android::JenkinsHashMix(hash, android::hash_type(mWordSpacing));
        hash = android::JenkinsHashMix(hash, android::hash_type(mPaintFlags));
        hash = android::JenkinsHashMix(hash, android::hash_type(mLocaleListId));
        hash = android::JenkinsHashMix(hash,
                                       android::hash_type(static_cast<uint8_t>(mFamilyVariant)));
        hash = android::JenkinsHashMix(
                hash,
                android::hash_type(static_cast<uint8_t>(packHyphenEdit(mStartHyphen, mEndHyphen))));
        hash = android::JenkinsHashMix(hash, android::hash_type(mIsRtl));
        hash = android::JenkinsHashMixShorts(hash, mChars, mNchars);
        return android::JenkinsHashWhiten(hash);
    }
};

class LayoutCache : private android::OnEntryRemoved<LayoutCacheKey, Layout*> {
public:
    void clear() {
        std::lock_guard<std::mutex> lock(mMutex);
        mCache.clear();
    }

    // Do not use LayoutCache inside the callback function, otherwise dead-lock may happen.
    template <typename F>
    void getOrCreate(const U16StringPiece& text, const Range& range, const MinikinPaint& paint,
                     bool dir, StartHyphenEdit startHyphen, EndHyphenEdit endHyphen, F& f) {
        LayoutCacheKey key(text, range, paint, dir, startHyphen, endHyphen);
        if (paint.skipCache()) {
            Layout layoutForWord;
            key.doLayout(&layoutForWord, paint);
            f(layoutForWord);
            return;
        }

        mRequestCount++;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            Layout* layout = mCache.get(key);
            if (layout != nullptr) {
                mCacheHitCount++;
                f(*layout);
                return;
            }
        }
        // Doing text layout takes long time, so releases the mutex during doing layout.
        // Don't care even if we do the same layout in other thred.
        key.copyText();
        std::unique_ptr<Layout> layout = std::make_unique<Layout>();
        key.doLayout(layout.get(), paint);
        f(*layout);
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mCache.put(key, layout.release());
        }
    }

    void dumpStats(int fd) {
        std::lock_guard<std::mutex> lock(mMutex);
        dprintf(fd, "\nLayout Cache Info:\n");
        dprintf(fd, "  Usage: %zd/%zd entries\n", mCache.size(), kMaxEntries);
        float ratio = (mRequestCount == 0) ? 0 : mCacheHitCount / (float)mRequestCount;
        dprintf(fd, "  Hit ratio: %d/%d (%f)\n", mCacheHitCount, mRequestCount, ratio);
    }

    static LayoutCache& getInstance() {
        static LayoutCache cache(kMaxEntries);
        return cache;
    }

protected:
    LayoutCache(uint32_t maxEntries) : mCache(maxEntries), mRequestCount(0), mCacheHitCount(0) {
        mCache.setOnEntryRemovedListener(this);
    }

private:
    // callback for OnEntryRemoved
    void operator()(LayoutCacheKey& key, Layout*& value) {
        key.freeText();
        delete value;
    }

    android::LruCache<LayoutCacheKey, Layout*> mCache GUARDED_BY(mMutex);

    int32_t mRequestCount;
    int32_t mCacheHitCount;

    // static const size_t kMaxEntries = LruCache<LayoutCacheKey, Layout*>::kUnlimitedCapacity;

    // TODO: eviction based on memory footprint; for now, we just use a constant
    // number of strings
    static const size_t kMaxEntries = 5000;

    std::mutex mMutex;
};

inline android::hash_t hash_type(const LayoutCacheKey& key) {
    return key.hash();
}

}  // namespace minikin
#endif  // MINIKIN_LAYOUT_CACHE_H
