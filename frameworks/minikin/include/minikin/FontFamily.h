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

#ifndef MINIKIN_FONT_FAMILY_H
#define MINIKIN_FONT_FAMILY_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "minikin/FontStyle.h"
#include "minikin/HbUtils.h"
#include "minikin/Macros.h"
#include "minikin/SparseBitSet.h"

namespace minikin {

class Font;
class MinikinFont;

// attributes representing transforms (fake bold, fake italic) to match styles
class FontFakery {
public:
    FontFakery() : mFakeBold(false), mFakeItalic(false) {}
    FontFakery(bool fakeBold, bool fakeItalic) : mFakeBold(fakeBold), mFakeItalic(fakeItalic) {}
    // TODO: want to support graded fake bolding
    bool isFakeBold() { return mFakeBold; }
    bool isFakeItalic() { return mFakeItalic; }

private:
    bool mFakeBold;
    bool mFakeItalic;
};

struct FakedFont {
    // ownership is the enclosing FontCollection
    const Font* font;
    FontFakery fakery;
};

typedef uint32_t AxisTag;

// Represents a single font file.
class Font {
public:
    class Builder {
    public:
        Builder(const std::shared_ptr<MinikinFont>& typeface) : mTypeface(typeface) {}

        // Override the font style. If not called, info from OS/2 table is used.
        Builder& setStyle(FontStyle style) {
            mWeight = style.weight();
            mSlant = style.slant();
            mIsWeightSet = mIsSlantSet = true;
            return *this;
        }

        // Override the font weight. If not called, info from OS/2 table is used.
        Builder& setWeight(uint16_t weight) {
            mWeight = weight;
            mIsWeightSet = true;
            return *this;
        }

        // Override the font slant. If not called, info from OS/2 table is used.
        Builder& setSlant(FontStyle::Slant slant) {
            mSlant = slant;
            mIsSlantSet = true;
            return *this;
        }

        Font build();

    private:
        std::shared_ptr<MinikinFont> mTypeface;
        uint16_t mWeight = static_cast<uint16_t>(FontStyle::Weight::NORMAL);
        FontStyle::Slant mSlant = FontStyle::Slant::UPRIGHT;
        bool mIsWeightSet = false;
        bool mIsSlantSet = false;
    };

    Font(Font&& o) = default;
    Font& operator=(Font&& o) = default;

    inline const std::shared_ptr<MinikinFont>& typeface() const { return mTypeface; }
    inline FontStyle style() const { return mStyle; }
    inline const HbFontUniquePtr& baseFont() const { return mBaseFont; }

    std::unordered_set<AxisTag> getSupportedAxes() const;

private:
    // Use Builder instead.
    Font(std::shared_ptr<MinikinFont>&& typeface, FontStyle style, HbFontUniquePtr&& baseFont)
            : mTypeface(std::move(typeface)), mStyle(style), mBaseFont(std::move(baseFont)) {}

    static HbFontUniquePtr prepareFont(const std::shared_ptr<MinikinFont>& typeface);
    static FontStyle analyzeStyle(const HbFontUniquePtr& font);

    std::shared_ptr<MinikinFont> mTypeface;
    FontStyle mStyle;
    HbFontUniquePtr mBaseFont;

    MINIKIN_PREVENT_COPY_AND_ASSIGN(Font);
};

struct FontVariation {
    FontVariation(AxisTag axisTag, float value) : axisTag(axisTag), value(value) {}
    AxisTag axisTag;
    float value;
};

class FontFamily {
public:
    // Must be the same value as FontConfig.java
    enum class Variant : uint8_t {
        DEFAULT = 0,  // Must be the same as FontConfig.VARIANT_DEFAULT
        COMPACT = 1,  // Must be the same as FontConfig.VARIANT_COMPACT
        ELEGANT = 2,  // Must be the same as FontConfig.VARIANT_ELEGANT
    };

    explicit FontFamily(std::vector<Font>&& fonts);
    FontFamily(Variant variant, std::vector<Font>&& fonts);
    FontFamily(uint32_t localeListId, Variant variant, std::vector<Font>&& fonts);

    FakedFont getClosestMatch(FontStyle style) const;

    uint32_t localeListId() const { return mLocaleListId; }
    Variant variant() const { return mVariant; }

    // API's for enumerating the fonts in a family. These don't guarantee any particular order
    size_t getNumFonts() const { return mFonts.size(); }
    const Font* getFont(size_t index) const { return &mFonts[index]; }
    FontStyle getStyle(size_t index) const { return mFonts[index].style(); }
    bool isColorEmojiFamily() const { return mIsColorEmoji; }
    const std::unordered_set<AxisTag>& supportedAxes() const { return mSupportedAxes; }

    // Get Unicode coverage.
    const SparseBitSet& getCoverage() const { return mCoverage; }

    // Returns true if the font has a glyph for the code point and variation selector pair.
    // Caller should acquire a lock before calling the method.
    bool hasGlyph(uint32_t codepoint, uint32_t variationSelector) const;

    // Returns true if this font family has a variaion sequence table (cmap format 14 subtable).
    bool hasVSTable() const { return !mCmapFmt14Coverage.empty(); }

    // Creates new FontFamily based on this family while applying font variations. Returns nullptr
    // if none of variations apply to this family.
    std::shared_ptr<FontFamily> createFamilyWithVariation(
            const std::vector<FontVariation>& variations) const;

private:
    void computeCoverage();

    uint32_t mLocaleListId;
    Variant mVariant;
    std::vector<Font> mFonts;
    std::unordered_set<AxisTag> mSupportedAxes;
    bool mIsColorEmoji;

    SparseBitSet mCoverage;
    std::vector<std::unique_ptr<SparseBitSet>> mCmapFmt14Coverage;

    MINIKIN_PREVENT_COPY_AND_ASSIGN(FontFamily);
};

}  // namespace minikin

#endif  // MINIKIN_FONT_FAMILY_H
