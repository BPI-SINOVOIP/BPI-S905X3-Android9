/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "TestSceneBase.h"

#include <vector>

class RoundRectClippingAnimation : public TestScene {
public:
    int mSpacing, mSize;

    RoundRectClippingAnimation(int spacing, int size) : mSpacing(spacing), mSize(size) {}

    std::vector<sp<RenderNode> > cards;
    void createContent(int width, int height, Canvas& canvas) override {
        canvas.drawColor(0xFFFFFFFF, SkBlendMode::kSrcOver);
        canvas.insertReorderBarrier(true);
        int ci = 0;

        for (int x = 0; x < width; x += mSpacing) {
            for (int y = 0; y < height; y += mSpacing) {
                auto color = BrightColors[ci++ % BrightColorsCount];
                auto card = TestUtils::createNode(
                        x, y, x + mSize, y + mSize, [&](RenderProperties& props, Canvas& canvas) {
                            canvas.drawColor(color, SkBlendMode::kSrcOver);
                            props.mutableOutline().setRoundRect(0, 0, props.getWidth(),
                                                                props.getHeight(), mSize * .25, 1);
                            props.mutableOutline().setShouldClip(true);
                        });
                canvas.drawRenderNode(card.get());
                cards.push_back(card);
            }
        }

        canvas.insertReorderBarrier(false);
    }
    void doFrame(int frameNr) override {
        int curFrame = frameNr % 50;
        if (curFrame > 25) curFrame = 50 - curFrame;
        for (auto& card : cards) {
            card->mutateStagingProperties().setTranslationX(curFrame);
            card->mutateStagingProperties().setTranslationY(curFrame);
            card->setPropertyFieldsDirty(RenderNode::X | RenderNode::Y);
        }
    }
};

static TestScene::Registrar _RoundRectClippingGpu(TestScene::Info{
        "roundRectClipping-gpu",
        "A bunch of RenderNodes with round rect clipping outlines that's GPU limited.",
        [](const TestScene::Options&) -> test::TestScene* {
            return new RoundRectClippingAnimation(dp(40), dp(200));
        }});

static TestScene::Registrar _RoundRectClippingCpu(TestScene::Info{
        "roundRectClipping-cpu",
        "A bunch of RenderNodes with round rect clipping outlines that's CPU limited.",
        [](const TestScene::Options&) -> test::TestScene* {
            return new RoundRectClippingAnimation(dp(20), dp(20));
        }});
