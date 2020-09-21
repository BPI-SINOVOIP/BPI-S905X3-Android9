/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 */

#ifndef DUMMY_PROCESSOR_H
#define DUMMY_PROCESSOR_H

#include <FbProcessor.h>

class DummyProcessor : public FbProcessor {
public:
    DummyProcessor();
    ~DummyProcessor();

    int32_t setup();
    int32_t process(
        std::shared_ptr<DrmFramebuffer> & inputfb,
        std::shared_ptr<DrmFramebuffer> & outfb);
    int32_t teardown();
};

#endif

