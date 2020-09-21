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

#include <memory>

#include <gtest/gtest.h>

#include "minikin/Hyphenator.h"

#include "FileUtils.h"
#include "FontTestUtils.h"
#include "HyphenatorMap.h"
#include "LineBreakerTestHelper.h"
#include "LocaleListCache.h"
#include "MinikinInternal.h"
#include "OptimalLineBreaker.h"
#include "UnicodeUtils.h"
#include "WordBreaker.h"

namespace minikin {
namespace {

using line_breaker_test_helper::ConstantRun;
using line_breaker_test_helper::LineBreakExpectation;
using line_breaker_test_helper::RectangleLineWidth;
using line_breaker_test_helper::sameLineBreak;
using line_breaker_test_helper::toString;

class OptimalLineBreakerTest : public testing::Test {
public:
    OptimalLineBreakerTest() {}

    virtual ~OptimalLineBreakerTest() {}

    virtual void SetUp() override {
        mHyphenationPattern = readWholeFile("/system/usr/hyphen-data/hyph-en-us.hyb");
        Hyphenator* hyphenator = Hyphenator::loadBinary(
                mHyphenationPattern.data(), 2 /* min prefix */, 2 /* min suffix */, "en-US");
        HyphenatorMap::add("en-US", hyphenator);
        HyphenatorMap::add("pl", Hyphenator::loadBinary(nullptr, 0, 0, "pl"));
    }

    virtual void TearDown() override { HyphenatorMap::clear(); }

protected:
    LineBreakResult doLineBreak(const U16StringPiece& textBuffer, BreakStrategy strategy,
                                HyphenationFrequency frequency, float charWidth, float lineWidth) {
        return doLineBreak(textBuffer, strategy, frequency, charWidth, "en-US", lineWidth);
    }

    LineBreakResult doLineBreak(const U16StringPiece& textBuffer, BreakStrategy strategy,
                                HyphenationFrequency frequency, float charWidth,
                                const std::string& lang, float lineWidth) {
        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, textBuffer.size()), lang, charWidth);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuffer, true /* compute hyphenation */, false /* compute full layout */);
        return doLineBreak(textBuffer, *measuredText, strategy, frequency, lineWidth);
    }

    LineBreakResult doLineBreak(const U16StringPiece& textBuffer, const MeasuredText& measuredText,
                                BreakStrategy strategy, HyphenationFrequency frequency,
                                float lineWidth) {
        RectangleLineWidth rectangleLineWidth(lineWidth);
        return breakLineOptimal(textBuffer, measuredText, rectangleLineWidth, strategy, frequency,
                                false /* justified */);
    }

private:
    std::vector<uint8_t> mHyphenationPattern;
};

TEST_F(OptimalLineBreakerTest, testBreakWithoutHyphenation) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr BreakStrategy BALANCED = BreakStrategy::Balanced;
    constexpr HyphenationFrequency NO_HYPHENATION = HyphenationFrequency::None;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;
    const std::vector<uint16_t> textBuf = utf8ToUtf16("This is an example text.");

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit END_HYPHEN = EndHyphenEdit::INSERT_HYPHEN;
    // Note that disable clang-format everywhere since aligned expectation is more readable.
    {
        constexpr float LINE_WIDTH = 1000 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 23 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an example " , 18 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."               ,  5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);

        // clang-format off
        expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an ex-" , 14 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample text."    , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 17 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an exam-" , 16 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple text."        ,  9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an ex-", 14 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample text."   , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 16 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an exam-" , 16 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple text."        ,  9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an ex-", 14 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample text."   , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 15 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an ex-", 14 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample text."   , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is an ex-", 14 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample text."   , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 13 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an "   , 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example text." , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 12 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an ", 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example "   ,  7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."      ,  5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is " ,  7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an exam-" ,  8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple text.",  9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 9 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This "   , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is " , 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an exam-" , 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple text.", 9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This "   , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is " , 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an exam-" , 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple text.", 9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 8 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This "   , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an ex-"  , 6 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 7 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This "   , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an ex-"  , 6 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ample "  , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 6 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an ", 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "exa"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mple " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an ", 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "exam-" , 5 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple "  , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 5 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an ", 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "exa"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mple " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is an ", 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "exam-" , 5 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple "  , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 4 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "exa"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mple " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text"  , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."     , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "exa"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mple " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "t"     , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ext."  , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This ", 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ex-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "am-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple " , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text" , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."    , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "This ", 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ex-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "am-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple " , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "te"   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xt."  , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 3 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                // TODO: Is this desperate break working correctly?
                { "T"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "his ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "e"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xam" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ple ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "tex" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t."  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                // TODO: Is this desperate break working correctly?
                { "T"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "his ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "e"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xam" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ple ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "te"  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xt." , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                // TODO: Is this desperate break working correctly?
                { "T"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "his ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ex-" , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "am-" , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ple ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "tex" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t."  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                // TODO: Is this desperate break working correctly?
                {"T"   , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"his ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"is " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"an " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"ex-" , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN},
                {"am-" , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN},
                {"ple ", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                // TODO: Is this desperate break working correctly?
                {"te"  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"xt." , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 2 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Th" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "e"  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xa" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mp" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "le ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "te" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xt" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                { "Th" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "e"  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xa" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mp" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "le ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                // TODO: Is this desperate break working correctly?
                { "t"  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ex" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t." , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 1 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "T" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "h" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "i" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "s ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "i" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "s ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "x" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "m" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "p" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "l" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "x" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "." , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testHyphenationStartLineChange) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;
    // "hyphenation" is hyphnated to "hy-phen-a-tion".
    const std::vector<uint16_t> textBuf = utf8ToUtf16("czerwono-niebieska");

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    constexpr StartHyphenEdit START_HYPHEN = StartHyphenEdit::INSERT_HYPHEN;

    // Note that disable clang-format everywhere since aligned expectation is more readable.
    {
        constexpr float LINE_WIDTH = 1000 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"czerwono-niebieska", 18 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, "pl",
                                        LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 18 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"czerwono-niebieska", 18 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, "pl",
                                        LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 13 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                {"czerwono-" ,  9 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"-niebieska", 10 * CHAR_WIDTH,    START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, "pl",
                                        LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testZeroWidthLine) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;
    constexpr float LINE_WIDTH = 0 * CHAR_WIDTH;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    {
        const auto textBuf = utf8ToUtf16("");
        std::vector<LineBreakExpectation> expect = {};
        const auto actual =
                doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        const auto textBuf = utf8ToUtf16("A");
        std::vector<LineBreakExpectation> expect = {
                {"A", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual =
                doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        const auto textBuf = utf8ToUtf16("AB");
        std::vector<LineBreakExpectation> expect = {
                {"A", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"B", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual =
                doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testZeroWidthCharacter) {
    constexpr float CHAR_WIDTH = 0.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    {
        constexpr float LINE_WIDTH = 1.0;
        const auto textBuf = utf8ToUtf16("This is an example text.");
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 0, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual =
                doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 0.0;
        const auto textBuf = utf8ToUtf16("This is an example text.");
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 0, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual =
                doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testLocaleSwitchTest) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
    const auto textBuf = utf8ToUtf16("This is an example text.");
    {
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "en-US", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuf, true /* compute hyphenation */, false /* compute full layout */);

        const auto actual =
                doLineBreak(textBuf, *measuredText, HIGH_QUALITY, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "fr-FR", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuf, true /* compute hyphenation */, false /* compute full layout */);
        const auto actual =
                doLineBreak(textBuf, *measuredText, HIGH_QUALITY, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testEmailOrUrl) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr BreakStrategy BALANCED = BreakStrategy::Balanced;
    constexpr HyphenationFrequency NO_HYPHENATION = HyphenationFrequency::None;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        const auto textBuf = utf8ToUtf16("This is an url: http://a.b");
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                // TODO: Fix this. Prefer not to break inside URL.
                {"This is an url: http://a", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {".b",                        2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on
        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        // clang-format off
        expect = {
                {"This is an url: ", 15 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"http://a.b",       10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        const auto textBuf = utf8ToUtf16("This is an email: a@example.com");
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                {"This is an email: ", 17 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"a@example.com"     , 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, HIGH_QUALITY, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, HIGH_QUALITY, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NO_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, BALANCED, NORMAL_HYPHENATION, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(OptimalLineBreakerTest, testLocaleSwitch_InEmailOrUrl) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr BreakStrategy HIGH_QUALITY = BreakStrategy::HighQuality;
    constexpr BreakStrategy BALANCED = BreakStrategy::Balanced;
    constexpr HyphenationFrequency NO_HYPHENATION = HyphenationFrequency::None;
    constexpr HyphenationFrequency NORMAL_HYPHENATION = HyphenationFrequency::Normal;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
    {
        const auto textBuf = utf8ToUtf16("This is an url: http://a.b");
        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "fr-FR", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measured = builder.build(
                textBuf, true /* compute hyphenation */, false /* compute full layout */);

        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                // TODO: Fix this. Prefer not to break inside URL.
                {"This is an url: http://a", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {".b",                        2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, *measured, HIGH_QUALITY, NO_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, *measured, HIGH_QUALITY, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);

        // clang-format off
        expect = {
                {"This is an url: ", 15 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"http://a.b",       10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on

        actual = doLineBreak(textBuf, *measured, BALANCED, NO_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, *measured, BALANCED, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        const auto textBuf = utf8ToUtf16("This is an email: a@example.com");
        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "fr-FR", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measured = builder.build(
                textBuf, true /* compute hyphenation */, false /* compute full layout */);

        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                {"This is an email: ", 17 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"a@example.com",      13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        // clang-format on

        auto actual = doLineBreak(textBuf, *measured, HIGH_QUALITY, NO_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, *measured, HIGH_QUALITY, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, *measured, BALANCED, NO_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
        actual = doLineBreak(textBuf, *measured, BALANCED, NORMAL_HYPHENATION, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

}  // namespace
}  // namespace minikin
