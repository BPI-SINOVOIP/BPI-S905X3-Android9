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

#ifndef MINIKIN_FONT_H
#define MINIKIN_FONT_H

#include <string>

#include "minikin/FontFamily.h"
#include "minikin/Hyphenator.h"

// An abstraction for platform fonts, allowing Minikin to be used with
// multiple actual implementations of fonts.

namespace minikin {

class FontCollection;
class MinikinFont;

// Possibly move into own .h file?
// Note: if you add a field here, either add it to LayoutCacheKey or to skipCache()
struct MinikinPaint {
    MinikinPaint(const std::shared_ptr<FontCollection>& font)
            : size(0),
              scaleX(0),
              skewX(0),
              letterSpacing(0),
              wordSpacing(0),
              paintFlags(0),
              localeListId(0),
              familyVariant(FontFamily::Variant::DEFAULT),
              fontFeatureSettings(),
              font(font) {}

    bool skipCache() const { return !fontFeatureSettings.empty(); }

    float size;
    float scaleX;
    float skewX;
    float letterSpacing;
    float wordSpacing;
    uint32_t paintFlags;
    uint32_t localeListId;
    FontStyle fontStyle;
    FontFamily::Variant familyVariant;
    std::string fontFeatureSettings;
    std::shared_ptr<FontCollection> font;

    void copyFrom(const MinikinPaint& paint) { *this = paint; }

    MinikinPaint(MinikinPaint&&) = default;
    MinikinPaint& operator=(MinikinPaint&&) = default;

    inline bool operator==(const MinikinPaint& paint) {
        return size == paint.size && scaleX == paint.scaleX && skewX == paint.skewX &&
               letterSpacing == paint.letterSpacing && wordSpacing == paint.wordSpacing &&
               paintFlags == paint.paintFlags && localeListId == paint.localeListId &&
               fontStyle == paint.fontStyle && familyVariant == paint.familyVariant &&
               fontFeatureSettings == paint.fontFeatureSettings && font.get() == paint.font.get();
    }

private:
    // Forbid implicit copy and assign. Use copyFrom instead.
    MinikinPaint(const MinikinPaint&) = default;
    MinikinPaint& operator=(const MinikinPaint&) = default;
};

// Only a few flags affect layout, but those that do should have values
// consistent with Android's paint flags.
enum MinikinPaintFlags {
    LinearTextFlag = 0x40,
};

struct MinikinRect {
    float mLeft = 0.0;
    float mTop = 0.0;
    float mRight = 0.0;
    float mBottom = 0.0;
    bool isEmpty() const { return mLeft == mRight || mTop == mBottom; }
    void set(const MinikinRect& r) {
        mLeft = r.mLeft;
        mTop = r.mTop;
        mRight = r.mRight;
        mBottom = r.mBottom;
    }
    void offset(float dx, float dy) {
        mLeft += dx;
        mTop += dy;
        mRight += dx;
        mBottom += dy;
    }
    void setEmpty() { mLeft = mTop = mRight = mBottom = 0.0; }
    void join(const MinikinRect& r);
};

// For holding vertical extents.
struct MinikinExtent {
    float ascent = 0.0;    // negative
    float descent = 0.0;   // positive
    float line_gap = 0.0;  // positive

    void reset() { ascent = descent = line_gap = 0.0; }

    void extendBy(const MinikinExtent& e) {
        ascent = std::min(ascent, e.ascent);
        descent = std::max(descent, e.descent);
        line_gap = std::max(line_gap, e.line_gap);
    }
};

class MinikinFont {
public:
    explicit MinikinFont(int32_t uniqueId) : mUniqueId(uniqueId) {}

    virtual ~MinikinFont() {}

    virtual float GetHorizontalAdvance(uint32_t glyph_id, const MinikinPaint& paint,
                                       const FontFakery& fakery) const = 0;

    virtual void GetBounds(MinikinRect* bounds, uint32_t glyph_id, const MinikinPaint& paint,
                           const FontFakery& fakery) const = 0;

    virtual void GetFontExtent(MinikinExtent* extent, const MinikinPaint& paint,
                               const FontFakery& fakery) const = 0;

    // Override if font can provide access to raw data
    virtual const void* GetFontData() const { return nullptr; }

    // Override if font can provide access to raw data
    virtual size_t GetFontSize() const { return 0; }

    // Override if font can provide access to raw data.
    // Returns index within OpenType collection
    virtual int GetFontIndex() const { return 0; }

    virtual const std::vector<minikin::FontVariation>& GetAxes() const = 0;

    virtual std::shared_ptr<MinikinFont> createFontWithVariation(
            const std::vector<FontVariation>&) const {
        return nullptr;
    }

    static uint32_t MakeTag(char c1, char c2, char c3, char c4) {
        return ((uint32_t)c1 << 24) | ((uint32_t)c2 << 16) | ((uint32_t)c3 << 8) | (uint32_t)c4;
    }

    int32_t GetUniqueId() const { return mUniqueId; }

private:
    const int32_t mUniqueId;
};

}  // namespace minikin

#endif  // MINIKIN_FONT_H
