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

#include "minikin/MeasuredText.h"

#include <gtest/gtest.h>

#include "minikin/LineBreaker.h"

#include "FontTestUtils.h"
#include "UnicodeUtils.h"

namespace minikin {

constexpr float CHAR_WIDTH = 10.0;  // Mock implementation always returns 10.0 for advance.

TEST(MeasuredTextTest, RunTests) {
    constexpr uint32_t CHAR_COUNT = 6;
    constexpr float REPLACEMENT_WIDTH = 20.0f;
    auto font = buildFontCollection("Ascii.ttf");

    MeasuredTextBuilder builder;

    MinikinPaint paint1(font);
    paint1.size = 10.0f;  // make 1em = 10px
    builder.addStyleRun(0, 2, std::move(paint1), false /* is RTL */);
    builder.addReplacementRun(2, 4, REPLACEMENT_WIDTH, 0 /* locale list id */);
    MinikinPaint paint2(font);
    paint2.size = 10.0f;  // make 1em = 10px
    builder.addStyleRun(4, 6, std::move(paint2), false /* is RTL */);

    std::vector<uint16_t> text(CHAR_COUNT, 'a');

    std::unique_ptr<MeasuredText> measuredText =
            builder.build(text, true /* compute hyphenation */, false /* compute full layout */);

    ASSERT_TRUE(measuredText);

    // ReplacementRun assigns all width to the first character and leave zeros others.
    std::vector<float> expectedWidths = {CHAR_WIDTH, CHAR_WIDTH, REPLACEMENT_WIDTH,
                                         0,          CHAR_WIDTH, CHAR_WIDTH};

    EXPECT_EQ(expectedWidths, measuredText->widths);
}

}  // namespace minikin
