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

#ifndef MINIKIN_LAYOUT_PIECES_H
#define MINIKIN_LAYOUT_PIECES_H

#include <unordered_map>

#include "minikin/Layout.h"
#include "minikin/LayoutCache.h"

namespace minikin {

struct LayoutPieces {
    struct KeyHasher {
        std::size_t operator()(const LayoutCacheKey& key) const { return key.hash(); }
    };

    LayoutPieces() {}

    ~LayoutPieces() {
        for (const auto it : offsetMap) {
            const_cast<LayoutCacheKey*>(&it.first)->freeText();
        }
    }

    std::unordered_map<LayoutCacheKey, Layout, KeyHasher> offsetMap;

    void insert(const U16StringPiece& textBuf, const Range& range, const MinikinPaint& paint,
                bool dir, StartHyphenEdit startEdit, EndHyphenEdit endEdit, const Layout& layout) {
        auto result = offsetMap.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(textBuf, range, paint, dir, startEdit, endEdit),
                std::forward_as_tuple(layout));
        if (result.second) {
            const_cast<LayoutCacheKey*>(&result.first->first)->copyText();
        }
    }

    template <typename F>
    void getOrCreate(const U16StringPiece& textBuf, const Range& range, const MinikinPaint& paint,
                     bool dir, StartHyphenEdit startEdit, EndHyphenEdit endEdit, F& f) const {
        auto it = offsetMap.find(LayoutCacheKey(textBuf, range, paint, dir, startEdit, endEdit));
        if (it == offsetMap.end()) {
            LayoutCache::getInstance().getOrCreate(textBuf, range, paint, dir, startEdit, endEdit,
                                                   f);
        } else {
            f(it->second);
        }
    }

    uint32_t getMemoryUsage() const {
        uint32_t result = 0;
        for (const auto& i : offsetMap) {
            result += i.first.getMemoryUsage() + i.second.getMemoryUsage();
        }
        return result;
    }
};

}  // namespace minikin

#endif  // MINIKIN_LAYOUT_PIECES_H
