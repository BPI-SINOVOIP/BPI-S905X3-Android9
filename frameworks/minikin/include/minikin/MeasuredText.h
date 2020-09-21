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

#ifndef MINIKIN_MEASURED_TEXT_H
#define MINIKIN_MEASURED_TEXT_H

#include <deque>
#include <vector>

#include "minikin/FontCollection.h"
#include "minikin/Layout.h"
#include "minikin/LayoutPieces.h"
#include "minikin/Macros.h"
#include "minikin/MinikinFont.h"
#include "minikin/Range.h"
#include "minikin/U16StringPiece.h"

namespace minikin {

class Run {
public:
    Run(const Range& range) : mRange(range) {}
    virtual ~Run() {}

    // Returns true if this run is RTL. Otherwise returns false.
    virtual bool isRtl() const = 0;

    // Returns true if this run is a target of hyphenation. Otherwise return false.
    virtual bool canHyphenate() const = 0;

    // Returns the locale list ID for this run.
    virtual uint32_t getLocaleListId() const = 0;

    // Fills the each character's advances, extents and overhangs.
    virtual void getMetrics(const U16StringPiece& text, float* advances, MinikinExtent* extents,
                            LayoutPieces* piece) const = 0;

    virtual std::pair<float, MinikinRect> getBounds(const U16StringPiece& text, const Range& range,
                                                    const LayoutPieces& pieces) const = 0;

    // Following two methods are only called when the implementation returns true for
    // canHyphenate method.

    // Returns the paint pointer used for this run.
    // Returns null if canHyphenate has not returned true.
    virtual const MinikinPaint* getPaint() const { return nullptr; }

    // Measures the hyphenation piece and fills each character's advances and overhangs.
    virtual float measureHyphenPiece(const U16StringPiece& /* text */,
                                     const Range& /* hyphenPieceRange */,
                                     StartHyphenEdit /* startHyphen */,
                                     EndHyphenEdit /* endHyphen */, float* /* advances */,
                                     LayoutPieces* /* pieces */) const {
        return 0.0;
    }

    inline const Range& getRange() const { return mRange; }

protected:
    const Range mRange;
};

class StyleRun : public Run {
public:
    StyleRun(const Range& range, MinikinPaint&& paint, bool isRtl)
            : Run(range), mPaint(std::move(paint)), mIsRtl(isRtl) {}

    bool canHyphenate() const override { return true; }
    uint32_t getLocaleListId() const override { return mPaint.localeListId; }
    bool isRtl() const override { return mIsRtl; }

    void getMetrics(const U16StringPiece& text, float* advances, MinikinExtent* extents,
                    LayoutPieces* pieces) const override {
        Bidi bidiFlag = mIsRtl ? Bidi::FORCE_RTL : Bidi::FORCE_LTR;
        Layout::measureText(text, mRange, bidiFlag, mPaint, StartHyphenEdit::NO_EDIT,
                            EndHyphenEdit::NO_EDIT, advances, extents, pieces);
    }

    std::pair<float, MinikinRect> getBounds(const U16StringPiece& text, const Range& range,
                                            const LayoutPieces& pieces) const override {
        Bidi bidiFlag = mIsRtl ? Bidi::FORCE_RTL : Bidi::FORCE_LTR;
        return Layout::getBoundsWithPrecomputedPieces(text, range, bidiFlag, mPaint, pieces);
    }

    const MinikinPaint* getPaint() const override { return &mPaint; }

    float measureHyphenPiece(const U16StringPiece& text, const Range& range,
                             StartHyphenEdit startHyphen, EndHyphenEdit endHyphen, float* advances,
                             LayoutPieces* pieces) const override {
        Bidi bidiFlag = mIsRtl ? Bidi::FORCE_RTL : Bidi::FORCE_LTR;
        return Layout::measureText(text, range, bidiFlag, mPaint, startHyphen, endHyphen, advances,
                                   nullptr /* extent */, pieces);
    }

private:
    MinikinPaint mPaint;
    const bool mIsRtl;
};

class ReplacementRun : public Run {
public:
    ReplacementRun(const Range& range, float width, uint32_t localeListId)
            : Run(range), mWidth(width), mLocaleListId(localeListId) {}

    bool isRtl() const { return false; }
    bool canHyphenate() const { return false; }
    uint32_t getLocaleListId() const { return mLocaleListId; }

    void getMetrics(const U16StringPiece& /* unused */, float* advances,
                    MinikinExtent* /* unused */, LayoutPieces* /* pieces */) const override {
        advances[0] = mWidth;
        // TODO: Get the extents information from the caller.
    }

    std::pair<float, MinikinRect> getBounds(const U16StringPiece& /* text */,
                                            const Range& /* range */,
                                            const LayoutPieces& /* pieces */) const override {
        // Bounding Box is not used in replacement run.
        return std::make_pair(mWidth, MinikinRect());
    }

private:
    const float mWidth;
    const uint32_t mLocaleListId;
};

// Represents a hyphenation break point.
struct HyphenBreak {
    // The break offset.
    uint32_t offset;

    // The hyphenation type.
    HyphenationType type;

    // The width of preceding piece after break at hyphenation point.
    float first;

    // The width of following piece after break at hyphenation point.
    float second;

    HyphenBreak(uint32_t offset, HyphenationType type, float first, float second)
            : offset(offset), type(type), first(first), second(second) {}
};

class MeasuredText {
public:
    // Character widths.
    std::vector<float> widths;

    // Font vertical extents for characters.
    // TODO: Introduce compression for extents. Usually, this has the same values for all chars.
    std::vector<MinikinExtent> extents;

    // Hyphenation points.
    std::vector<HyphenBreak> hyphenBreaks;

    // The style information.
    std::vector<std::unique_ptr<Run>> runs;

    // The copied layout pieces for construcing final layouts.
    // TODO: Stop assigning width/extents if layout pieces are available for reducing memory impact.
    LayoutPieces layoutPieces;

    uint32_t getMemoryUsage() const {
        return sizeof(float) * widths.size() + sizeof(MinikinExtent) * extents.size() +
               sizeof(HyphenBreak) * hyphenBreaks.size() + layoutPieces.getMemoryUsage();
    }

    void buildLayout(const U16StringPiece& textBuf, const Range& range, const MinikinPaint& paint,
                     Bidi bidiFlag, StartHyphenEdit startHyphen, EndHyphenEdit endHyphen,
                     Layout* layout);
    MinikinRect getBounds(const U16StringPiece& textBuf, const Range& range);

    MeasuredText(MeasuredText&&) = default;
    MeasuredText& operator=(MeasuredText&&) = default;

    MINIKIN_PREVENT_COPY_AND_ASSIGN(MeasuredText);

private:
    friend class MeasuredTextBuilder;

    void measure(const U16StringPiece& textBuf, bool computeHyphenation, bool computeLayout);

    // Use MeasuredTextBuilder instead.
    MeasuredText(const U16StringPiece& textBuf, std::vector<std::unique_ptr<Run>>&& runs,
                 bool computeHyphenation, bool computeLayout)
            : widths(textBuf.size()), extents(textBuf.size()), runs(std::move(runs)) {
        measure(textBuf, computeHyphenation, computeLayout);
    }
};

class MeasuredTextBuilder {
public:
    MeasuredTextBuilder() {}

    void addStyleRun(int32_t start, int32_t end, MinikinPaint&& paint, bool isRtl) {
        mRuns.emplace_back(std::make_unique<StyleRun>(Range(start, end), std::move(paint), isRtl));
    }

    void addReplacementRun(int32_t start, int32_t end, float width, uint32_t localeListId) {
        mRuns.emplace_back(
                std::make_unique<ReplacementRun>(Range(start, end), width, localeListId));
    }

    template <class T, typename... Args>
    void addCustomRun(Args&&... args) {
        mRuns.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    std::unique_ptr<MeasuredText> build(const U16StringPiece& textBuf, bool computeHyphenation,
                                        bool computeLayout) {
        // Unable to use make_unique here since make_unique is not a friend of MeasuredText.
        return std::unique_ptr<MeasuredText>(
                new MeasuredText(textBuf, std::move(mRuns), computeHyphenation, computeLayout));
    }

    MINIKIN_PREVENT_COPY_ASSIGN_AND_MOVE(MeasuredTextBuilder);

private:
    std::vector<std::unique_ptr<Run>> mRuns;
};

}  // namespace minikin

#endif  // MINIKIN_MEASURED_TEXT_H
