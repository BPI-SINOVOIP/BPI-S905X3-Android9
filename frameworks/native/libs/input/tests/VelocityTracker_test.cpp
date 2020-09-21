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

#define LOG_TAG "VelocityTracker_test"

#include <math.h>

#include <android-base/stringprintf.h>
#include <gtest/gtest.h>
#include <input/VelocityTracker.h>

using android::base::StringPrintf;

namespace android {

constexpr int32_t DEFAULT_POINTER_ID = 0; // pointer ID used for manually defined tests

// velocity must be in the range (1-tol)*EV <= velocity <= (1+tol)*EV
// here EV = expected value, tol = VELOCITY_TOLERANCE
constexpr float VELOCITY_TOLERANCE = 0.2;

// --- VelocityTrackerTest ---
class VelocityTrackerTest : public testing::Test { };

static void checkVelocity(float Vactual, float Vtarget) {
    // Compare directions
    if ((Vactual > 0 && Vtarget <= 0) || (Vactual < 0 && Vtarget >= 0)) {
        FAIL() << StringPrintf("Velocity %f does not have the same direction"
                " as the target velocity %f", Vactual, Vtarget);
    }

    // Compare magnitudes
    const float Vlower = fabsf(Vtarget * (1 - VELOCITY_TOLERANCE));
    const float Vupper = fabsf(Vtarget * (1 + VELOCITY_TOLERANCE));
    if (fabsf(Vactual) < Vlower) {
        FAIL() << StringPrintf("Velocity %f is more than %.0f%% below target velocity %f",
                Vactual, VELOCITY_TOLERANCE * 100, Vtarget);
    }
    if (fabsf(Vactual) > Vupper) {
        FAIL() << StringPrintf("Velocity %f is more than %.0f%% above target velocity %f",
                Vactual, VELOCITY_TOLERANCE * 100, Vtarget);
    }
    SUCCEED() << StringPrintf("Velocity %f within %.0f%% of target %f)",
            Vactual, VELOCITY_TOLERANCE * 100, Vtarget);
}

void failWithMessage(std::string message) {
    FAIL() << message; // cannot do this directly from a non-void function
}

struct Position {
      nsecs_t time;
      float x;
      float y;
};


MotionEvent* createSimpleMotionEvent(const Position* positions, size_t numSamples) {
    /**
     * Only populate the basic fields of a MotionEvent, such as time and a single axis
     * Designed for use with manually-defined tests.
     * Create a new MotionEvent on the heap, caller responsible for destroying the object.
     */
    if (numSamples < 1) {
        failWithMessage(StringPrintf("Need at least 1 sample to create a MotionEvent."
                " Received numSamples=%zu", numSamples));
    }

    MotionEvent* event = new MotionEvent();
    PointerCoords coords;
    PointerProperties properties[1];

    properties[0].id = DEFAULT_POINTER_ID;
    properties[0].toolType = AMOTION_EVENT_TOOL_TYPE_FINGER;

    // First sample added separately with initialize
    coords.setAxisValue(AMOTION_EVENT_AXIS_X, positions[0].x);
    coords.setAxisValue(AMOTION_EVENT_AXIS_Y, positions[0].y);
    event->initialize(0, AINPUT_SOURCE_TOUCHSCREEN, AMOTION_EVENT_ACTION_MOVE,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, positions[0].time, 1, properties, &coords);

    for (size_t i = 1; i < numSamples; i++) {
        coords.setAxisValue(AMOTION_EVENT_AXIS_X, positions[i].x);
        coords.setAxisValue(AMOTION_EVENT_AXIS_Y, positions[i].y);
        event->addSample(positions[i].time, &coords);
    }
    return event;
}

static void computeAndCheckVelocity(const Position* positions, size_t numSamples,
            int32_t axis, float targetVelocity) {
    VelocityTracker vt(nullptr);
    float Vx, Vy;

    MotionEvent* event = createSimpleMotionEvent(positions, numSamples);
    vt.addMovement(event);

    vt.getVelocity(DEFAULT_POINTER_ID, &Vx, &Vy);

    switch (axis) {
    case AMOTION_EVENT_AXIS_X:
        checkVelocity(Vx, targetVelocity);
        break;
    case AMOTION_EVENT_AXIS_Y:
        checkVelocity(Vy, targetVelocity);
        break;
    default:
        FAIL() << "Axis must be either AMOTION_EVENT_AXIS_X or AMOTION_EVENT_AXIS_Y";
    }
    delete event;
}

/*
 * ================== VelocityTracker tests generated manually =====================================
 */
 // @todo Currently disabled, enable when switching away from lsq2 VelocityTrackerStrategy
TEST_F(VelocityTrackerTest, DISABLED_ThreePointsPositiveVelocityTest) {
    // Same coordinate is reported 2 times in a row
    // It is difficult to determine the correct answer here, but at least the direction
    // of the reported velocity should be positive.
    Position values[] = {
        { 0, 273, NAN },
        { 12585000, 293, NAN },
        { 14730000, 293, NAN },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 1600);
}

TEST_F(VelocityTrackerTest, ThreePointsZeroVelocityTest) {
    // Same coordinate is reported 3 times in a row
    Position values[] = {
        { 0, 293, NAN },
        { 6132000, 293, NAN },
        { 11283000, 293, NAN },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 0);
}

TEST_F(VelocityTrackerTest, ThreePointsLinearVelocityTest) {
    // Fixed velocity at 5 points per 10 milliseconds
    Position values[] = {
        { 0, 0, NAN },
        { 10000000, 5, NAN },
        { 20000000, 10, NAN },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 500);
}


/**
 * ================== VelocityTracker tests generated by recording real events =====================
 *
 * To add a test, record the input coordinates and event times to all calls
 * to void VelocityTracker::addMovement(const MotionEvent* event).
 * Also record all calls to VelocityTracker::clear().
 * Finally, record the output of VelocityTracker::getVelocity(...)
 * This will give you the necessary data to create a new test.
 */

// --------------- Recorded by hand on swordfish ---------------------------------------------------
// @todo Currently disabled, enable when switching away from lsq2 VelocityTrackerStrategy
TEST_F(VelocityTrackerTest, DISABLED_SwordfishFlingDown) {
    // Recording of a fling on Swordfish that could cause a fling in the wrong direction
    Position values[] = {
        { 0, 271, 96 },
        { 16071042, 269.786346, 106.922775 },
        { 35648403, 267.983063, 156.660034 },
        { 52313925, 262.638397, 220.339081 },
        { 68976522, 266.138824, 331.581116 },
        { 85639375, 274.79245, 428.113159 },
        { 96948871, 274.79245, 428.113159 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 623.577637);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 8523.348633);
}

// --------------- Recorded by hand on sailfish, generated by a script -----------------------------
// For some of these tests, the X-direction velocity checking has been removed, because the lsq2
// and the impulse VelocityTrackerStrategies did not agree within 20%.
// Since the flings were recorded in the Y-direction, the intentional user action should only
// be relevant for the Y axis.
// There have been also cases where lsq2 and impulse disagreed more than 20% in the Y-direction.
// Those recordings have been discarded because we didn't feel one strategy's interpretation was
// more correct than another's but didn't want to increase the tolerance for the entire test suite.
//
// There are 18 tests total below: 9 in the positive Y direction and 9 in the opposite.
// The recordings were loosely binned into 3 categories - slow, faster, and fast, which roughly
// characterizes the velocity of the finger motion.
// These can be treated approximately as:
// slow - less than 1 page gets scrolled
// faster - more than 1 page gets scrolled, but less than 3
// fast - entire list is scrolled (fling is done as hard as possible)

TEST_F(VelocityTrackerTest, SailfishFlingUpSlow1) {
    // Sailfish - fling up - slow - 1
    Position values[] = {
        { 235089067457000, 528.00, 983.00 },
        { 235089084684000, 527.00, 981.00 },
        { 235089093349000, 527.00, 977.00 },
        { 235089095677625, 527.00, 975.93 },
        { 235089101859000, 527.00, 970.00 },
        { 235089110378000, 528.00, 960.00 },
        { 235089112497111, 528.25, 957.51 },
        { 235089118760000, 531.00, 946.00 },
        { 235089126686000, 535.00, 931.00 },
        { 235089129316820, 536.33, 926.02 },
        { 235089135199000, 540.00, 914.00 },
        { 235089144297000, 546.00, 896.00 },
        { 235089146136443, 547.21, 892.36 },
        { 235089152923000, 553.00, 877.00 },
        { 235089160784000, 559.00, 851.00 },
        { 235089162955851, 560.66, 843.82 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 872.794617); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 951.698181); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -3604.819336); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -3044.966064); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpSlow2) {
    // Sailfish - fling up - slow - 2
    Position values[] = {
        { 235110560704000, 522.00, 1107.00 },
        { 235110575764000, 522.00, 1107.00 },
        { 235110584385000, 522.00, 1107.00 },
        { 235110588421179, 521.52, 1106.52 },
        { 235110592830000, 521.00, 1106.00 },
        { 235110601385000, 520.00, 1104.00 },
        { 235110605088160, 519.14, 1102.27 },
        { 235110609952000, 518.00, 1100.00 },
        { 235110618353000, 517.00, 1093.00 },
        { 235110621755146, 516.60, 1090.17 },
        { 235110627010000, 517.00, 1081.00 },
        { 235110634785000, 518.00, 1063.00 },
        { 235110638422450, 518.87, 1052.58 },
        { 235110643161000, 520.00, 1039.00 },
        { 235110651767000, 524.00, 1011.00 },
        { 235110655089581, 525.54, 1000.19 },
        { 235110660368000, 530.00, 980.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -4096.583008); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -3455.094238); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpSlow3) {
    // Sailfish - fling up - slow - 3
    Position values[] = {
        { 792536237000, 580.00, 1317.00 },
        { 792541538987, 580.63, 1311.94 },
        { 792544613000, 581.00, 1309.00 },
        { 792552301000, 583.00, 1295.00 },
        { 792558362309, 585.13, 1282.92 },
        { 792560828000, 586.00, 1278.00 },
        { 792569446000, 589.00, 1256.00 },
        { 792575185095, 591.54, 1241.41 },
        { 792578491000, 593.00, 1233.00 },
        { 792587044000, 597.00, 1211.00 },
        { 792592008172, 600.28, 1195.92 },
        { 792594616000, 602.00, 1188.00 },
        { 792603129000, 607.00, 1167.00 },
        { 792608831290, 609.48, 1155.83 },
        { 792612321000, 611.00, 1149.00 },
        { 792620768000, 615.00, 1131.00 },
        { 792625653873, 617.32, 1121.73 },
        { 792629200000, 619.00, 1115.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 574.33429); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 617.40564); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -2361.982666); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -2500.055664); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFaster1) {
    // Sailfish - fling up - faster - 1
    Position values[] = {
        { 235160420675000, 610.00, 1042.00 },
        { 235160428220000, 609.00, 1026.00 },
        { 235160436544000, 609.00, 1024.00 },
        { 235160441852394, 609.64, 1020.82 },
        { 235160444878000, 610.00, 1019.00 },
        { 235160452673000, 613.00, 1006.00 },
        { 235160458519743, 617.18, 992.06 },
        { 235160461061000, 619.00, 986.00 },
        { 235160469798000, 627.00, 960.00 },
        { 235160475186713, 632.22, 943.02 },
        { 235160478051000, 635.00, 934.00 },
        { 235160486489000, 644.00, 906.00 },
        { 235160491853697, 649.56, 890.56 },
        { 235160495177000, 653.00, 881.00 },
        { 235160504148000, 662.00, 858.00 },
        { 235160509231495, 666.81, 845.37 },
        { 235160512603000, 670.00, 837.00 },
        { 235160520366000, 679.00, 814.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 1274.141724); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 1438.53186); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -3877.35498); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -3695.859619); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFaster2) {
    // Sailfish - fling up - faster - 2
    Position values[] = {
        { 847153808000, 576.00, 1264.00 },
        { 847171174000, 576.00, 1262.00 },
        { 847179640000, 576.00, 1257.00 },
        { 847185187540, 577.41, 1249.22 },
        { 847187487000, 578.00, 1246.00 },
        { 847195710000, 581.00, 1227.00 },
        { 847202027059, 583.93, 1209.40 },
        { 847204324000, 585.00, 1203.00 },
        { 847212672000, 590.00, 1176.00 },
        { 847218861395, 594.36, 1157.11 },
        { 847221190000, 596.00, 1150.00 },
        { 847230484000, 602.00, 1124.00 },
        { 847235701400, 607.56, 1103.83 },
        { 847237986000, 610.00, 1095.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -4280.07959); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -4241.004395); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFaster3) {
    // Sailfish - fling up - faster - 3
    Position values[] = {
        { 235200532789000, 507.00, 1084.00 },
        { 235200549221000, 507.00, 1083.00 },
        { 235200557841000, 507.00, 1081.00 },
        { 235200558051189, 507.00, 1080.95 },
        { 235200566314000, 507.00, 1078.00 },
        { 235200574876586, 508.97, 1070.12 },
        { 235200575006000, 509.00, 1070.00 },
        { 235200582900000, 514.00, 1054.00 },
        { 235200591276000, 525.00, 1023.00 },
        { 235200591701829, 525.56, 1021.42 },
        { 235200600064000, 542.00, 976.00 },
        { 235200608519000, 563.00, 911.00 },
        { 235200608527086, 563.02, 910.94 },
        { 235200616933000, 590.00, 844.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -8715.686523); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -7639.026367); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFast1) {
    // Sailfish - fling up - fast - 1
    Position values[] = {
        { 920922149000, 561.00, 1412.00 },
        { 920930185000, 559.00, 1377.00 },
        { 920930262463, 558.98, 1376.66 },
        { 920938547000, 559.00, 1371.00 },
        { 920947096857, 562.91, 1342.68 },
        { 920947302000, 563.00, 1342.00 },
        { 920955502000, 577.00, 1272.00 },
        { 920963931021, 596.87, 1190.54 },
        { 920963987000, 597.00, 1190.00 },
        { 920972530000, 631.00, 1093.00 },
        { 920980765511, 671.31, 994.68 },
        { 920980906000, 672.00, 993.00 },
        { 920989261000, 715.00, 903.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 5670.329102); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, 5991.866699); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -13021.101562); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -15093.995117); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFast2) {
    // Sailfish - fling up - fast - 2
    Position values[] = {
        { 235247153233000, 518.00, 1168.00 },
        { 235247170452000, 517.00, 1167.00 },
        { 235247178908000, 515.00, 1159.00 },
        { 235247179556213, 514.85, 1158.39 },
        { 235247186821000, 515.00, 1125.00 },
        { 235247195265000, 521.00, 1051.00 },
        { 235247196389476, 521.80, 1041.15 },
        { 235247203649000, 538.00, 932.00 },
        { 235247212253000, 571.00, 794.00 },
        { 235247213222491, 574.72, 778.45 },
        { 235247220736000, 620.00, 641.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -20286.958984); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -20494.587891); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingUpFast3) {
    // Sailfish - fling up - fast - 3
    Position values[] = {
        { 235302568736000, 529.00, 1167.00 },
        { 235302576644000, 523.00, 1140.00 },
        { 235302579395063, 520.91, 1130.61 },
        { 235302585140000, 522.00, 1130.00 },
        { 235302593615000, 527.00, 1065.00 },
        { 235302596207444, 528.53, 1045.12 },
        { 235302602102000, 559.00, 872.00 },
        { 235302610545000, 652.00, 605.00 },
        { 235302613019881, 679.26, 526.73 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -39295.941406); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, -36461.421875); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownSlow1) {
    // Sailfish - fling down - slow - 1
    Position values[] = {
        { 235655749552755, 582.00, 432.49 },
        { 235655750638000, 582.00, 433.00 },
        { 235655758865000, 582.00, 440.00 },
        { 235655766221523, 581.16, 448.43 },
        { 235655767594000, 581.00, 450.00 },
        { 235655776044000, 580.00, 462.00 },
        { 235655782890696, 579.18, 474.35 },
        { 235655784360000, 579.00, 477.00 },
        { 235655792795000, 578.00, 496.00 },
        { 235655799559531, 576.27, 515.04 },
        { 235655800612000, 576.00, 518.00 },
        { 235655809535000, 574.00, 542.00 },
        { 235655816988015, 572.17, 564.86 },
        { 235655817685000, 572.00, 567.00 },
        { 235655825981000, 569.00, 595.00 },
        { 235655833808653, 566.26, 620.60 },
        { 235655834541000, 566.00, 623.00 },
        { 235655842893000, 563.00, 649.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -419.749695); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -398.303894); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 3309.016357); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 3969.099854); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownSlow2) {
    // Sailfish - fling down - slow - 2
    Position values[] = {
        { 235671152083370, 485.24, 558.28 },
        { 235671154126000, 485.00, 559.00 },
        { 235671162497000, 484.00, 566.00 },
        { 235671168750511, 483.27, 573.29 },
        { 235671171071000, 483.00, 576.00 },
        { 235671179390000, 482.00, 588.00 },
        { 235671185417210, 481.31, 598.98 },
        { 235671188173000, 481.00, 604.00 },
        { 235671196371000, 480.00, 624.00 },
        { 235671202084196, 479.27, 639.98 },
        { 235671204235000, 479.00, 646.00 },
        { 235671212554000, 478.00, 673.00 },
        { 235671219471011, 476.39, 697.12 },
        { 235671221159000, 476.00, 703.00 },
        { 235671229592000, 474.00, 734.00 },
        { 235671236281462, 472.43, 758.38 },
        { 235671238098000, 472.00, 765.00 },
        { 235671246532000, 470.00, 799.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -262.80426); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -243.665344); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4215.682129); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4587.986816); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownSlow3) {
    // Sailfish - fling down - slow - 3
    Position values[] = {
        { 170983201000, 557.00, 533.00 },
        { 171000668000, 556.00, 534.00 },
        { 171007359750, 554.73, 535.27 },
        { 171011197000, 554.00, 536.00 },
        { 171017660000, 552.00, 540.00 },
        { 171024201831, 549.97, 544.73 },
        { 171027333000, 549.00, 547.00 },
        { 171034603000, 545.00, 557.00 },
        { 171041043371, 541.98, 567.55 },
        { 171043147000, 541.00, 571.00 },
        { 171051052000, 536.00, 586.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -723.413513); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -651.038452); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 2091.502441); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 1934.517456); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFaster1) {
    // Sailfish - fling down - faster - 1
    Position values[] = {
        { 235695280333000, 558.00, 451.00 },
        { 235695283971237, 558.43, 454.45 },
        { 235695289038000, 559.00, 462.00 },
        { 235695297388000, 561.00, 478.00 },
        { 235695300638465, 561.83, 486.25 },
        { 235695305265000, 563.00, 498.00 },
        { 235695313591000, 564.00, 521.00 },
        { 235695317305492, 564.43, 532.68 },
        { 235695322181000, 565.00, 548.00 },
        { 235695330709000, 565.00, 577.00 },
        { 235695333972227, 565.00, 588.10 },
        { 235695339250000, 565.00, 609.00 },
        { 235695347839000, 565.00, 642.00 },
        { 235695351313257, 565.00, 656.18 },
        { 235695356412000, 565.00, 677.00 },
        { 235695364899000, 563.00, 710.00 },
        { 235695368118682, 562.24, 722.52 },
        { 235695373403000, 564.00, 744.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4254.639648); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4698.415039); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFaster2) {
    // Sailfish - fling down - faster - 2
    Position values[] = {
        { 235709624766000, 535.00, 579.00 },
        { 235709642256000, 534.00, 580.00 },
        { 235709643350278, 533.94, 580.06 },
        { 235709650760000, 532.00, 584.00 },
        { 235709658615000, 530.00, 593.00 },
        { 235709660170495, 529.60, 594.78 },
        { 235709667095000, 527.00, 606.00 },
        { 235709675616000, 524.00, 628.00 },
        { 235709676983261, 523.52, 631.53 },
        { 235709684289000, 521.00, 652.00 },
        { 235709692763000, 518.00, 682.00 },
        { 235709693804993, 517.63, 685.69 },
        { 235709701438000, 515.00, 709.00 },
        { 235709709830000, 512.00, 739.00 },
        { 235709710626776, 511.72, 741.85 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -430.440247); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -447.600311); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 3953.859375); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4316.155273); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFaster3) {
    // Sailfish - fling down - faster - 3
    Position values[] = {
        { 235727628927000, 540.00, 440.00 },
        { 235727636810000, 537.00, 454.00 },
        { 235727646176000, 536.00, 454.00 },
        { 235727653586628, 535.12, 456.65 },
        { 235727654557000, 535.00, 457.00 },
        { 235727663024000, 534.00, 465.00 },
        { 235727670410103, 533.04, 479.45 },
        { 235727670691000, 533.00, 480.00 },
        { 235727679255000, 531.00, 501.00 },
        { 235727687233704, 529.09, 526.73 },
        { 235727687628000, 529.00, 528.00 },
        { 235727696113000, 526.00, 558.00 },
        { 235727704057546, 523.18, 588.98 },
        { 235727704576000, 523.00, 591.00 },
        { 235727713099000, 520.00, 626.00 },
        { 235727720880776, 516.33, 655.36 },
        { 235727721580000, 516.00, 658.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4484.617676); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 4927.92627); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFast1) {
    // Sailfish - fling down - fast - 1
    Position values[] = {
        { 235762352849000, 467.00, 286.00 },
        { 235762360250000, 443.00, 344.00 },
        { 235762362787412, 434.77, 363.89 },
        { 235762368807000, 438.00, 359.00 },
        { 235762377220000, 425.00, 423.00 },
        { 235762379608561, 421.31, 441.17 },
        { 235762385698000, 412.00, 528.00 },
        { 235762394133000, 406.00, 648.00 },
        { 235762396429369, 404.37, 680.67 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 19084.931641); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 16064.685547); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFast2) {
    // Sailfish - fling down - fast - 2
    Position values[] = {
        { 235772487188000, 576.00, 204.00 },
        { 235772495159000, 553.00, 236.00 },
        { 235772503568000, 551.00, 240.00 },
        { 235772508192247, 545.55, 254.17 },
        { 235772512051000, 541.00, 266.00 },
        { 235772520794000, 520.00, 337.00 },
        { 235772525015263, 508.92, 394.43 },
        { 235772529174000, 498.00, 451.00 },
        { 235772537635000, 484.00, 589.00 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 18660.048828); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 16918.439453); // lsq2
}


TEST_F(VelocityTrackerTest, SailfishFlingDownFast3) {
    // Sailfish - fling down - fast - 3
    Position values[] = {
        { 507650295000, 628.00, 233.00 },
        { 507658234000, 605.00, 269.00 },
        { 507666784000, 601.00, 274.00 },
        { 507669660483, 599.65, 275.68 },
        { 507675427000, 582.00, 308.00 },
        { 507683740000, 541.00, 404.00 },
        { 507686506238, 527.36, 435.95 },
        { 507692220000, 487.00, 581.00 },
        { 507700707000, 454.00, 792.00 },
        { 507703352649, 443.71, 857.77 },
    };
    size_t count = sizeof(values) / sizeof(Position);
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -6772.508301); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_X, -6388.48877); // lsq2
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 29765.908203); // impulse
    computeAndCheckVelocity(values, count, AMOTION_EVENT_AXIS_Y, 28354.796875); // lsq2
}


} // namespace android
