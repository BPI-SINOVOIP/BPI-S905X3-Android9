/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#if !defined(_AUTOBACKLIGHT_H)
#define _AUTOBACKLIGHT_H
#include "../tvin/CTvin.h"
#include <utils/Thread.h>
#include "../vpp/CVpp.h"

class AutoBackLight: public Thread {
private:
    tv_source_input_t mAutoBacklightSource;
    int mCur_source_default_backlight;
    int mCur_sig_state;
    bool mAutoBacklight_OnOff_Flag;
    int mCurrent_backlight;
    int mCur_dest_backlight;

    void adjustDstBacklight();
    void adjustBacklight();
    int HistogramGet_AVE();
    bool  threadLoop();

public:
    enum SIG_STATE {
        SIG_STATE_STABLE = 1,
        SIG_STATE_NOSIG = 2,
    };

    AutoBackLight();
    ~AutoBackLight();
    void updateSigState(int state);
    void startAutoBacklight( tv_source_input_t tv_source_input );
    void stopAutoBacklight();
    bool isAutoBacklightOn();
};
#endif
