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

#define LOG_TAG "Minikin"
#include "minikin/MeasuredText.h"

#include "minikin/Layout.h"

#include "LayoutUtils.h"
#include "LineBreakerUtil.h"

namespace minikin {

void MeasuredText::measure(const U16StringPiece& textBuf, bool computeHyphenation,
                           bool computeLayout) {
    if (textBuf.size() == 0) {
        return;
    }
    LayoutPieces* piecesOut = computeLayout ? &layoutPieces : nullptr;
    CharProcessor proc(textBuf);
    for (const auto& run : runs) {
        const Range& range = run->getRange();
        const uint32_t runOffset = range.getStart();
        run->getMetrics(textBuf, widths.data() + runOffset, extents.data() + runOffset, piecesOut);

        if (!computeHyphenation || !run->canHyphenate()) {
            continue;
        }

        proc.updateLocaleIfNecessary(*run);
        for (uint32_t i = range.getStart(); i < range.getEnd(); ++i) {
            proc.feedChar(i, textBuf[i], widths[i]);

            const uint32_t nextCharOffset = i + 1;
            if (nextCharOffset != proc.nextWordBreak) {
                continue;  // Wait until word break point.
            }

            populateHyphenationPoints(textBuf, *run, *proc.hyphenator, proc.contextRange(),
                                      proc.wordRange(), &hyphenBreaks, piecesOut);
        }
    }
}

void MeasuredText::buildLayout(const U16StringPiece& textBuf, const Range& range,
                               const MinikinPaint& paint, Bidi bidiFlags,
                               StartHyphenEdit startHyphen, EndHyphenEdit endHyphen,
                               Layout* layout) {
    layout->doLayoutWithPrecomputedPieces(textBuf, range, bidiFlags, paint, startHyphen, endHyphen,
                                          layoutPieces);
}

MinikinRect MeasuredText::getBounds(const U16StringPiece& textBuf, const Range& range) {
    MinikinRect rect;
    float advance = 0.0f;
    for (const auto& run : runs) {
        const Range& runRange = run->getRange();
        if (!Range::intersects(range, runRange)) {
            continue;
        }
        std::pair<float, MinikinRect> next =
                run->getBounds(textBuf, Range::intersection(runRange, range), layoutPieces);
        MinikinRect nextRect = next.second;
        nextRect.offset(advance, 0);
        rect.join(nextRect);
        advance += next.first;
    }
    return rect;
}

}  // namespace minikin
