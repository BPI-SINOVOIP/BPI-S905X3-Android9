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

#include <memory>

#include <gtest/gtest.h>

#include "minikin/Hyphenator.h"

#include "FileUtils.h"
#include "FontTestUtils.h"
#include "GreedyLineBreaker.h"
#include "HyphenatorMap.h"
#include "LineBreakerTestHelper.h"
#include "LocaleListCache.h"
#include "MinikinInternal.h"
#include "UnicodeUtils.h"
#include "WordBreaker.h"

namespace minikin {
namespace {

using line_breaker_test_helper::ConstantRun;
using line_breaker_test_helper::LineBreakExpectation;
using line_breaker_test_helper::RectangleLineWidth;
using line_breaker_test_helper::sameLineBreak;
using line_breaker_test_helper::toString;

class GreedyLineBreakerTest : public testing::Test {
public:
    GreedyLineBreakerTest() {}

    virtual ~GreedyLineBreakerTest() {}

    virtual void SetUp() override {
        mHyphenationPattern = readWholeFile("/system/usr/hyphen-data/hyph-en-us.hyb");
        Hyphenator* hyphenator = Hyphenator::loadBinary(
                mHyphenationPattern.data(), 2 /* min prefix */, 2 /* min suffix */, "en-US");
        HyphenatorMap::add("en-US", hyphenator);
        HyphenatorMap::add("pl", Hyphenator::loadBinary(nullptr, 0, 0, "pl"));
    }

    virtual void TearDown() override { HyphenatorMap::clear(); }

protected:
    LineBreakResult doLineBreak(const U16StringPiece& textBuffer, bool doHyphenation,
                                float charWidth, float lineWidth) {
        return doLineBreak(textBuffer, doHyphenation, charWidth, "en-US", lineWidth);
    }

    LineBreakResult doLineBreak(const U16StringPiece& textBuffer, bool doHyphenation,
                                float charWidth, const std::string& lang, float lineWidth) {
        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, textBuffer.size()), lang, charWidth);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuffer, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(lineWidth);
        TabStops tabStops(nullptr, 0, 10);
        return breakLineGreedy(textBuffer, *measuredText, rectangleLineWidth, tabStops,
                               doHyphenation);
    }

private:
    std::vector<uint8_t> mHyphenationPattern;
};

TEST_F(GreedyLineBreakerTest, testBreakWithoutHyphenation) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool NO_HYPHEN = false;  // No hyphenation in this test case.
    const std::vector<uint16_t> textBuf = utf8ToUtf16("This is an example text.");

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    // Note that disable clang-format everywhere since aligned expectation is more readable.
    {
        constexpr float LINE_WIDTH = 1000 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 24 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 23 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is an example ", 18 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."              ,  5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 8 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "     , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 7 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "This is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an "     , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "example ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
                { "exampl", 6 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e "    , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
                { "examp" , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "le "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
                { "exam"  , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ple "  , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "text"  , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."     , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 3 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Thi" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "s "  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "an " , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "exa" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "mpl" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e "  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "tex" , 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t."  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
                { "ex" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "am" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "pl" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e " , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "te" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "xt" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testBreakWithHyphenation) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool NO_HYPHEN = true;  // Do hyphenation in this test case.
    // "hyphenation" is hyphnated to "hy-phen-a-tion".
    const std::vector<uint16_t> textBuf = utf8ToUtf16("Hyphenation is hyphenation.");

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit END_HYPHEN = EndHyphenEdit::INSERT_HYPHEN;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    // Note that disable clang-format everywhere since aligned expectation is more readable.
    {
        constexpr float LINE_WIDTH = 1000 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"Hyphenation is hyphenation.", 27 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 27 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"Hyphenation is hyphenation.", 27 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 26 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphenation is " , 14 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphenation."    , 12 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 17 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphenation is " , 14 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphenation."    , 12 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 12 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphenation " , 11 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "          , 2  * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphenation." , 12 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 10 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphena-", 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphena-", 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 8 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphena-", 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion is ", 7 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphena-", 8 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion."   , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 7 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hyphen-", 7 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ation " , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "    , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hyphen-", 7 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ation." , 6 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 6 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hy-"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phena-", 6 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion " , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hy-"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phena-", 6 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 5 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hy-"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phen-" , 5 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ation ", 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "   , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hy-"   , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phen-" , 5 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "a-"    , 2 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion." , 5 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 4 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hy-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phen" , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a-"   , 2 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion ", 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is "  , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hy-"  , 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phen" , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a-"   , 2 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tion" , 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."    , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 3 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hy-", 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phe", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "na-", 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tio", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n " , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hy-", 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "phe", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "na-", 3 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "tio", 3 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n." , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 2 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "Hy" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ph" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "en" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a-" , 2 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ti" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "on ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "is ", 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "hy" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "ph" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "en" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a-" , 2 * CHAR_WIDTH, NO_START_HYPHEN, END_HYPHEN },
                { "ti" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "on" , 2 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "."  , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 1 * CHAR_WIDTH;
        // clang-format off
        std::vector<LineBreakExpectation> expect = {
                { "H" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "y" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "p" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "h" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "i" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "o" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "i" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "s ", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "h" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "y" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "p" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "h" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "e" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "a" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "t" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "i" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "o" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "n" , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
                { "." , 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN },
        };
        // clang-format on

        const auto actual = doLineBreak(textBuf, NO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testHyphenationStartLineChange) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.
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

        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, "pl", LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 18 * CHAR_WIDTH;
        std::vector<LineBreakExpectation> expect = {
                {"czerwono-niebieska", 18 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, "pl", LINE_WIDTH);
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

        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, "pl", LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testZeroWidthLine) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.
    constexpr float LINE_WIDTH = 0 * CHAR_WIDTH;

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    {
        const auto textBuf = utf8ToUtf16("");
        std::vector<LineBreakExpectation> expect = {};
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        const auto textBuf = utf8ToUtf16("A");
        std::vector<LineBreakExpectation> expect = {
                {"A", 1 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testZeroWidthCharacter) {
    constexpr float CHAR_WIDTH = 0.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    {
        constexpr float LINE_WIDTH = 1.0;
        const auto textBuf = utf8ToUtf16("This is an example text.");
        std::vector<LineBreakExpectation> expect = {
                {"This is an example text.", 0, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
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
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testLocaleSwitchTest) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.

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
                textBuf, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(LINE_WIDTH);
        TabStops tabStops(nullptr, 0, 0);

        const auto actual =
                breakLineGreedy(textBuf, *measuredText, rectangleLineWidth, tabStops, DO_HYPHEN);
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
                textBuf, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(LINE_WIDTH);
        TabStops tabStops(nullptr, 0, 0);

        const auto actual =
                breakLineGreedy(textBuf, *measuredText, rectangleLineWidth, tabStops, DO_HYPHEN);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testEmailOrUrl) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        const auto textBuf = utf8ToUtf16("This is an url: http://a.b");
        std::vector<LineBreakExpectation> expect = {
                {"This is an url: ", 15 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"http://a.b", 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
        const auto textBuf = utf8ToUtf16("This is an email: a@example.com");
        std::vector<LineBreakExpectation> expect = {
                {"This is an email: ", 17 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"a@example.com", 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };
        const auto actual = doLineBreak(textBuf, DO_HYPHEN, CHAR_WIDTH, LINE_WIDTH);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

TEST_F(GreedyLineBreakerTest, testLocaleSwitch_InEmailOrUrl) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;

    constexpr float LINE_WIDTH = 24 * CHAR_WIDTH;
    {
        const auto textBuf = utf8ToUtf16("This is an url: http://a.b");
        std::vector<LineBreakExpectation> expect = {
                {"This is an url: ", 15 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"http://a.b", 10 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "fr-FR", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuf, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(LINE_WIDTH);
        TabStops tabStops(nullptr, 0, 0);

        const auto actual =
                breakLineGreedy(textBuf, *measuredText, rectangleLineWidth, tabStops, DO_HYPHEN);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
    {
        const auto textBuf = utf8ToUtf16("This is an email: a@example.com");
        std::vector<LineBreakExpectation> expect = {
                {"This is an email: ", 17 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
                {"a@example.com", 13 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, 18), "en-US", CHAR_WIDTH);
        builder.addCustomRun<ConstantRun>(Range(18, textBuf.size()), "fr-FR", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuf, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(LINE_WIDTH);
        TabStops tabStops(nullptr, 0, 0);

        const auto actual =
                breakLineGreedy(textBuf, *measuredText, rectangleLineWidth, tabStops, DO_HYPHEN);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

// b/68669534
TEST_F(GreedyLineBreakerTest, CrashFix_Space_Tab) {
    constexpr float CHAR_WIDTH = 10.0;
    constexpr bool DO_HYPHEN = true;  // Do hyphenation in this test case.

    constexpr StartHyphenEdit NO_START_HYPHEN = StartHyphenEdit::NO_EDIT;
    constexpr EndHyphenEdit NO_END_HYPHEN = EndHyphenEdit::NO_EDIT;
    {
        constexpr float LINE_WIDTH = 5 * CHAR_WIDTH;
        const auto textBuf = utf8ToUtf16("a \tb");
        std::vector<LineBreakExpectation> expect = {
                {"a \tb", 4 * CHAR_WIDTH, NO_START_HYPHEN, NO_END_HYPHEN},
        };

        MeasuredTextBuilder builder;
        builder.addCustomRun<ConstantRun>(Range(0, textBuf.size()), "en-US", CHAR_WIDTH);
        std::unique_ptr<MeasuredText> measuredText = builder.build(
                textBuf, false /* compute hyphenation */, false /* compute full layout */);
        RectangleLineWidth rectangleLineWidth(LINE_WIDTH);
        TabStops tabStops(nullptr, 0, CHAR_WIDTH);

        const auto actual =
                breakLineGreedy(textBuf, *measuredText, rectangleLineWidth, tabStops, DO_HYPHEN);
        EXPECT_TRUE(sameLineBreak(expect, actual)) << toString(expect) << std::endl
                                                   << " vs " << std::endl
                                                   << toString(textBuf, actual);
    }
}

}  // namespace
}  // namespace minikin
