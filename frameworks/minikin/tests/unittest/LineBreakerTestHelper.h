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

#include <gtest/gtest.h>

#include "minikin/Hyphenator.h"
#include "minikin/LineBreaker.h"

#include "LocaleListCache.h"
#include "MinikinInternal.h"
#include "UnicodeUtils.h"

namespace minikin {
namespace line_breaker_test_helper {

class RectangleLineWidth : public LineWidth {
public:
    RectangleLineWidth(float width) : mWidth(width) {}
    virtual ~RectangleLineWidth() {}

    float getAt(size_t) const override { return mWidth; }
    float getMin() const override { return mWidth; }
    float getLeftPaddingAt(size_t) const override { return 0; }
    float getRightPaddingAt(size_t) const override { return 0; }

private:
    float mWidth;
};

// The run implemenataion for returning the same width for all characters.
class ConstantRun : public Run {
public:
    ConstantRun(const Range& range, const std::string& lang, float width)
            : Run(range), mPaint(nullptr /* font collection */), mWidth(width) {
        mLocaleListId = LocaleListCache::getId(lang);
    }

    virtual bool isRtl() const override { return false; }
    virtual bool canHyphenate() const override { return true; }
    virtual uint32_t getLocaleListId() const { return mLocaleListId; }

    virtual void getMetrics(const U16StringPiece&, float* advances, MinikinExtent*,
                            LayoutPieces*) const {
        std::fill(advances, advances + mRange.getLength(), mWidth);
    }

    virtual std::pair<float, MinikinRect> getBounds(const U16StringPiece& /* text */,
                                                    const Range& /* range */,
                                                    const LayoutPieces& /* pieces */) const {
        return std::make_pair(mWidth, MinikinRect());
    }

    virtual const MinikinPaint* getPaint() const { return &mPaint; }

    virtual float measureHyphenPiece(const U16StringPiece&, const Range& range,
                                     StartHyphenEdit start, EndHyphenEdit end, float*,
                                     LayoutPieces*) const {
        uint32_t extraCharForHyphen = 0;
        if (isInsertion(start)) {
            extraCharForHyphen++;
        }
        if (isInsertion(end)) {
            extraCharForHyphen++;
        }
        return mWidth * (range.getLength() + extraCharForHyphen);
    }

private:
    MinikinPaint mPaint;
    uint32_t mLocaleListId;
    float mWidth;
};

struct LineBreakExpectation {
    LineBreakExpectation(const std::string& lineContent, float width, StartHyphenEdit startEdit,
                         EndHyphenEdit endEdit)
            : mLineContent(lineContent), mWidth(width), mStartEdit(startEdit), mEndEdit(endEdit){};

    std::string mLineContent;
    float mWidth;
    StartHyphenEdit mStartEdit;
    EndHyphenEdit mEndEdit;
};

static bool sameLineBreak(const std::vector<LineBreakExpectation>& expected,
                          const LineBreakResult& actual) {
    if (expected.size() != actual.breakPoints.size()) {
        return false;
    }

    uint32_t breakOffset = 0;
    for (uint32_t i = 0; i < expected.size(); ++i) {
        std::vector<uint16_t> u16Str = utf8ToUtf16(expected[i].mLineContent);

        // The expected string contains auto inserted hyphen. Remove it for computing offset.
        uint32_t lineLength = u16Str.size();
        if (isInsertion(expected[i].mStartEdit)) {
            if (u16Str[0] != '-') {
                return false;
            }
            --lineLength;
        }
        if (isInsertion(expected[i].mEndEdit)) {
            if (u16Str.back() != '-') {
                return false;
            }
            --lineLength;
        }
        breakOffset += lineLength;

        if (breakOffset != static_cast<uint32_t>(actual.breakPoints[i])) {
            return false;
        }
        if (expected[i].mWidth != actual.widths[i]) {
            return false;
        }
        HyphenEdit edit = static_cast<HyphenEdit>(actual.flags[i] & 0xFF);
        if (expected[i].mStartEdit != startHyphenEdit(edit)) {
            return false;
        }
        if (expected[i].mEndEdit != endHyphenEdit(edit)) {
            return false;
        }
    }
    return true;
}

// Make debug string.
static std::string toString(const std::vector<LineBreakExpectation>& lines) {
    std::string out;
    for (uint32_t i = 0; i < lines.size(); ++i) {
        const LineBreakExpectation& line = lines[i];

        char lineMsg[128] = {};
        snprintf(lineMsg, sizeof(lineMsg),
                 "Line %2d, Width: %5.1f, Hyphen(%hhu, %hhu), Text: \"%s\"\n", i, line.mWidth,
                 line.mStartEdit, line.mEndEdit, line.mLineContent.c_str());
        out += lineMsg;
    }
    return out;
}

// Make debug string.
static std::string toString(const U16StringPiece& textBuf, const LineBreakResult& lines) {
    std::string out;
    for (uint32_t i = 0; i < lines.breakPoints.size(); ++i) {
        const Range textRange(i == 0 ? 0 : lines.breakPoints[i - 1], lines.breakPoints[i]);
        const HyphenEdit edit = static_cast<HyphenEdit>(lines.flags[i] & 0xFF);

        const StartHyphenEdit startEdit = startHyphenEdit(edit);
        const EndHyphenEdit endEdit = endHyphenEdit(edit);
        std::string hyphenatedStr = utf16ToUtf8(textBuf.substr(textRange));

        if (isInsertion(startEdit)) {
            hyphenatedStr.insert(0, "-");
        }
        if (isInsertion(endEdit)) {
            hyphenatedStr.push_back('-');
        }
        char lineMsg[128] = {};
        snprintf(lineMsg, sizeof(lineMsg),
                 "Line %2d, Width: %5.1f, Hyphen(%hhu, %hhu), Text: \"%s\"\n", i, lines.widths[i],
                 startEdit, endEdit, hyphenatedStr.c_str());
        out += lineMsg;
    }
    return out;
}

}  // namespace line_breaker_test_helper
}  // namespace minikin
