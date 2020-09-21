/*
 * Copyright (C) 2014 The Android Open Source Project
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

static data_1_2_t<double, double, double> g_fmod_intel_data[] = {
  { // Entry 0
    -0x1.57e8932492c0p-10,
    -0x1.200ad685e7f44p3,
    -0x1.000014abd446dp0
  },
  { // Entry 1
    -0x1.d7dbf487ffd0p-11,
    -0x1.3333333333334p-1,
    0x1.10a83585649f6p-4
  },
  { // Entry 2
    0x1.p-1072,
    0x1.0000000000001p-41,
    0x1.4p-1072
  },
  { // Entry 3
    0x1.p-1072,
    0x1.0000000000001p-1017,
    0x1.4p-1072
  },
  { // Entry 4
    0x1.fc8420e88cbfp18,
    0x1.11f783ee89b08p99,
    0x1.0abe1a29d8e8cp19
  },
  { // Entry 5
    0x1.50p-61,
    0x1.5555555555552p-12,
    0x1.1111111111106p-14
  },
  { // Entry 6
    -0.0,
    -0x1.0p-117,
    -0x1.0p-117
  },
  { // Entry 7
    -0.0,
    -0x1.0p-117,
    0x1.0p-117
  },
  { // Entry 8
    0.0,
    0x1.0p-117,
    -0x1.0p-117
  },
  { // Entry 9
    0.0,
    0x1.0p-117,
    0x1.0p-117
  },
  { // Entry 10
    -0x1.p-117,
    -0x1.0p-117,
    0x1.0p15
  },
  { // Entry 11
    -0x1.p-117,
    -0x1.0p-117,
    0x1.0p16
  },
  { // Entry 12
    0x1.p-117,
    0x1.0p-117,
    0x1.0p15
  },
  { // Entry 13
    0x1.p-117,
    0x1.0p-117,
    0x1.0p16
  },
  { // Entry 14
    -0x1.p-117,
    -0x1.0p-117,
    0x1.0p117
  },
  { // Entry 15
    -0x1.p-117,
    -0x1.0p-117,
    0x1.0p118
  },
  { // Entry 16
    0x1.p-117,
    0x1.0p-117,
    0x1.0p117
  },
  { // Entry 17
    0x1.p-117,
    0x1.0p-117,
    0x1.0p118
  },
  { // Entry 18
    0.0,
    0x1.0p15,
    -0x1.0p-117
  },
  { // Entry 19
    0.0,
    0x1.0p15,
    0x1.0p-117
  },
  { // Entry 20
    0.0,
    0x1.0p16,
    -0x1.0p-117
  },
  { // Entry 21
    0.0,
    0x1.0p16,
    0x1.0p-117
  },
  { // Entry 22
    0.0,
    0x1.0p15,
    0x1.0p15
  },
  { // Entry 23
    0x1.p15,
    0x1.0p15,
    0x1.0p16
  },
  { // Entry 24
    0.0,
    0x1.0p16,
    0x1.0p15
  },
  { // Entry 25
    0.0,
    0x1.0p16,
    0x1.0p16
  },
  { // Entry 26
    0x1.p15,
    0x1.0p15,
    0x1.0p117
  },
  { // Entry 27
    0x1.p15,
    0x1.0p15,
    0x1.0p118
  },
  { // Entry 28
    0x1.p16,
    0x1.0p16,
    0x1.0p117
  },
  { // Entry 29
    0x1.p16,
    0x1.0p16,
    0x1.0p118
  },
  { // Entry 30
    0.0,
    0x1.0p117,
    -0x1.0p-117
  },
  { // Entry 31
    0.0,
    0x1.0p117,
    0x1.0p-117
  },
  { // Entry 32
    0.0,
    0x1.0p118,
    -0x1.0p-117
  },
  { // Entry 33
    0.0,
    0x1.0p118,
    0x1.0p-117
  },
  { // Entry 34
    0.0,
    0x1.0p117,
    0x1.0p15
  },
  { // Entry 35
    0.0,
    0x1.0p117,
    0x1.0p16
  },
  { // Entry 36
    0.0,
    0x1.0p118,
    0x1.0p15
  },
  { // Entry 37
    0.0,
    0x1.0p118,
    0x1.0p16
  },
  { // Entry 38
    0.0,
    0x1.0p117,
    0x1.0p117
  },
  { // Entry 39
    0x1.p117,
    0x1.0p117,
    0x1.0p118
  },
  { // Entry 40
    0.0,
    0x1.0p118,
    0x1.0p117
  },
  { // Entry 41
    0.0,
    0x1.0p118,
    0x1.0p118
  },
  { // Entry 42
    0.0,
    0x1.9p6,
    0x1.4p3
  },
  { // Entry 43
    0x1.p0,
    0x1.9p6,
    0x1.6p3
  },
  { // Entry 44
    0x1.p2,
    0x1.9p6,
    0x1.8p3
  },
  { // Entry 45
    0x1.p0,
    0x1.940p6,
    0x1.4p3
  },
  { // Entry 46
    0x1.p1,
    0x1.940p6,
    0x1.6p3
  },
  { // Entry 47
    0x1.40p2,
    0x1.940p6,
    0x1.8p3
  },
  { // Entry 48
    0x1.p1,
    0x1.980p6,
    0x1.4p3
  },
  { // Entry 49
    0x1.80p1,
    0x1.980p6,
    0x1.6p3
  },
  { // Entry 50
    0x1.80p2,
    0x1.980p6,
    0x1.8p3
  },
  { // Entry 51
    0x1.80p1,
    0x1.9c0p6,
    0x1.4p3
  },
  { // Entry 52
    0x1.p2,
    0x1.9c0p6,
    0x1.6p3
  },
  { // Entry 53
    0x1.c0p2,
    0x1.9c0p6,
    0x1.8p3
  },
  { // Entry 54
    0x1.p2,
    0x1.ap6,
    0x1.4p3
  },
  { // Entry 55
    0x1.40p2,
    0x1.ap6,
    0x1.6p3
  },
  { // Entry 56
    0x1.p3,
    0x1.ap6,
    0x1.8p3
  },
  { // Entry 57
    0x1.40p2,
    0x1.a40p6,
    0x1.4p3
  },
  { // Entry 58
    0x1.80p2,
    0x1.a40p6,
    0x1.6p3
  },
  { // Entry 59
    0x1.20p3,
    0x1.a40p6,
    0x1.8p3
  },
  { // Entry 60
    0x1.80p2,
    0x1.a80p6,
    0x1.4p3
  },
  { // Entry 61
    0x1.c0p2,
    0x1.a80p6,
    0x1.6p3
  },
  { // Entry 62
    0x1.40p3,
    0x1.a80p6,
    0x1.8p3
  },
  { // Entry 63
    0x1.c0p2,
    0x1.ac0p6,
    0x1.4p3
  },
  { // Entry 64
    0x1.p3,
    0x1.ac0p6,
    0x1.6p3
  },
  { // Entry 65
    0x1.60p3,
    0x1.ac0p6,
    0x1.8p3
  },
  { // Entry 66
    0x1.p3,
    0x1.bp6,
    0x1.4p3
  },
  { // Entry 67
    0x1.20p3,
    0x1.bp6,
    0x1.6p3
  },
  { // Entry 68
    0.0,
    0x1.bp6,
    0x1.8p3
  },
  { // Entry 69
    0x1.20p3,
    0x1.b40p6,
    0x1.4p3
  },
  { // Entry 70
    0x1.40p3,
    0x1.b40p6,
    0x1.6p3
  },
  { // Entry 71
    0x1.p0,
    0x1.b40p6,
    0x1.8p3
  },
  { // Entry 72
    0.0,
    0x1.b80p6,
    0x1.4p3
  },
  { // Entry 73
    0.0,
    0x1.b80p6,
    0x1.6p3
  },
  { // Entry 74
    0x1.p1,
    0x1.b80p6,
    0x1.8p3
  },
  { // Entry 75
    -0.0,
    -0x1.0000000000001p0,
    -0x1.0000000000001p0
  },
  { // Entry 76
    -0x1.p-52,
    -0x1.0000000000001p0,
    -0x1.0p0
  },
  { // Entry 77
    -0x1.80p-52,
    -0x1.0000000000001p0,
    -0x1.fffffffffffffp-1
  },
  { // Entry 78
    -0x1.p0,
    -0x1.0p0,
    -0x1.0000000000001p0
  },
  { // Entry 79
    -0.0,
    -0x1.0p0,
    -0x1.0p0
  },
  { // Entry 80
    -0x1.p-53,
    -0x1.0p0,
    -0x1.fffffffffffffp-1
  },
  { // Entry 81
    -0x1.fffffffffffff0p-1,
    -0x1.fffffffffffffp-1,
    -0x1.0000000000001p0
  },
  { // Entry 82
    -0x1.fffffffffffff0p-1,
    -0x1.fffffffffffffp-1,
    -0x1.0p0
  },
  { // Entry 83
    -0.0,
    -0x1.fffffffffffffp-1,
    -0x1.fffffffffffffp-1
  },
  { // Entry 84
    -0x1.80p-52,
    -0x1.0000000000001p0,
    0x1.fffffffffffffp-1
  },
  { // Entry 85
    -0x1.p-52,
    -0x1.0000000000001p0,
    0x1.0p0
  },
  { // Entry 86
    -0.0,
    -0x1.0000000000001p0,
    0x1.0000000000001p0
  },
  { // Entry 87
    -0x1.p-53,
    -0x1.0p0,
    0x1.fffffffffffffp-1
  },
  { // Entry 88
    -0.0,
    -0x1.0p0,
    0x1.0p0
  },
  { // Entry 89
    -0x1.p0,
    -0x1.0p0,
    0x1.0000000000001p0
  },
  { // Entry 90
    -0.0,
    -0x1.fffffffffffffp-1,
    0x1.fffffffffffffp-1
  },
  { // Entry 91
    -0x1.fffffffffffff0p-1,
    -0x1.fffffffffffffp-1,
    0x1.0p0
  },
  { // Entry 92
    -0x1.fffffffffffff0p-1,
    -0x1.fffffffffffffp-1,
    0x1.0000000000001p0
  },
  { // Entry 93
    0x1.fffffffffffff0p-1,
    0x1.fffffffffffffp-1,
    -0x1.0000000000001p0
  },
  { // Entry 94
    0x1.fffffffffffff0p-1,
    0x1.fffffffffffffp-1,
    -0x1.0p0
  },
  { // Entry 95
    0.0,
    0x1.fffffffffffffp-1,
    -0x1.fffffffffffffp-1
  },
  { // Entry 96
    0x1.p0,
    0x1.0p0,
    -0x1.0000000000001p0
  },
  { // Entry 97
    0.0,
    0x1.0p0,
    -0x1.0p0
  },
  { // Entry 98
    0x1.p-53,
    0x1.0p0,
    -0x1.fffffffffffffp-1
  },
  { // Entry 99
    0.0,
    0x1.0000000000001p0,
    -0x1.0000000000001p0
  },
  { // Entry 100
    0x1.p-52,
    0x1.0000000000001p0,
    -0x1.0p0
  },
  { // Entry 101
    0x1.80p-52,
    0x1.0000000000001p0,
    -0x1.fffffffffffffp-1
  },
  { // Entry 102
    0.0,
    0x1.fffffffffffffp-1,
    0x1.fffffffffffffp-1
  },
  { // Entry 103
    0x1.fffffffffffff0p-1,
    0x1.fffffffffffffp-1,
    0x1.0p0
  },
  { // Entry 104
    0x1.fffffffffffff0p-1,
    0x1.fffffffffffffp-1,
    0x1.0000000000001p0
  },
  { // Entry 105
    0x1.p-53,
    0x1.0p0,
    0x1.fffffffffffffp-1
  },
  { // Entry 106
    0.0,
    0x1.0p0,
    0x1.0p0
  },
  { // Entry 107
    0x1.p0,
    0x1.0p0,
    0x1.0000000000001p0
  },
  { // Entry 108
    0x1.80p-52,
    0x1.0000000000001p0,
    0x1.fffffffffffffp-1
  },
  { // Entry 109
    0x1.p-52,
    0x1.0000000000001p0,
    0x1.0p0
  },
  { // Entry 110
    0.0,
    0x1.0000000000001p0,
    0x1.0000000000001p0
  },
  { // Entry 111
    -0.0,
    -0x1.0p-1074,
    0x1.0p-1074
  },
  { // Entry 112
    -0.0,
    -0.0,
    0x1.0p-1074
  },
  { // Entry 113
    0.0,
    0x1.0p-1074,
    0x1.0p-1074
  },
  { // Entry 114
    -0.0,
    -0x1.0p-1074,
    -0x1.0p-1074
  },
  { // Entry 115
    -0.0,
    -0.0,
    -0x1.0p-1074
  },
  { // Entry 116
    0.0,
    0x1.0p-1074,
    -0x1.0p-1074
  },
  { // Entry 117
    -0x1.p-1074,
    -0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 118
    -0.0,
    -0.0,
    0x1.fffffffffffffp1023
  },
  { // Entry 119
    0x1.p-1074,
    0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 120
    -0x1.p-1074,
    -0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 121
    -0.0,
    -0.0,
    -0x1.fffffffffffffp1023
  },
  { // Entry 122
    0x1.p-1074,
    0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 123
    0x1.p-1074,
    0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 124
    -0x1.p-1074,
    -0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 125
    -0x1.p-1074,
    -0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 126
    0x1.p-1074,
    0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 127
    0.0,
    0x1.fffffffffffffp1023,
    0x1.0p-1074
  },
  { // Entry 128
    -0.0,
    -0x1.fffffffffffffp1023,
    -0x1.0p-1074
  },
  { // Entry 129
    -0.0,
    -0x1.fffffffffffffp1023,
    0x1.0p-1074
  },
  { // Entry 130
    0.0,
    0x1.fffffffffffffp1023,
    -0x1.0p-1074
  },
  { // Entry 131
    0.0,
    0x1.fffffffffffffp1023,
    0x1.fffffffffffffp1023
  },
  { // Entry 132
    0.0,
    0x1.fffffffffffffp1023,
    -0x1.fffffffffffffp1023
  },
  { // Entry 133
    -0.0,
    -0x1.fffffffffffffp1023,
    0x1.fffffffffffffp1023
  },
  { // Entry 134
    -0.0,
    -0x1.fffffffffffffp1023,
    -0x1.fffffffffffffp1023
  },
  { // Entry 135
    -0x1.80p-1,
    -0x1.0000000000001p51,
    0x1.fffffffffffffp-1
  },
  { // Entry 136
    -0x1.p-1,
    -0x1.0000000000001p51,
    0x1.0p0
  },
  { // Entry 137
    -0.0,
    -0x1.0000000000001p51,
    0x1.0000000000001p0
  },
  { // Entry 138
    -0x1.p-2,
    -0x1.0p51,
    0x1.fffffffffffffp-1
  },
  { // Entry 139
    -0.0,
    -0x1.0p51,
    0x1.0p0
  },
  { // Entry 140
    -0x1.00000000000020p-1,
    -0x1.0p51,
    0x1.0000000000001p0
  },
  { // Entry 141
    -0.0,
    -0x1.fffffffffffffp50,
    0x1.fffffffffffffp-1
  },
  { // Entry 142
    -0x1.80p-1,
    -0x1.fffffffffffffp50,
    0x1.0p0
  },
  { // Entry 143
    -0x1.00000000000040p-2,
    -0x1.fffffffffffffp50,
    0x1.0000000000001p0
  },
  { // Entry 144
    0.0,
    0x1.fffffffffffffp51,
    0x1.fffffffffffffp-1
  },
  { // Entry 145
    0x1.p-1,
    0x1.fffffffffffffp51,
    0x1.0p0
  },
  { // Entry 146
    0x1.00000000000040p-1,
    0x1.fffffffffffffp51,
    0x1.0000000000001p0
  },
  { // Entry 147
    0x1.p-1,
    0x1.0p52,
    0x1.fffffffffffffp-1
  },
  { // Entry 148
    0.0,
    0x1.0p52,
    0x1.0p0
  },
  { // Entry 149
    0x1.p-52,
    0x1.0p52,
    0x1.0000000000001p0
  },
  { // Entry 150
    0x1.00000000000010p-1,
    0x1.0000000000001p52,
    0x1.fffffffffffffp-1
  },
  { // Entry 151
    0.0,
    0x1.0000000000001p52,
    0x1.0p0
  },
  { // Entry 152
    0.0,
    0x1.0000000000001p52,
    0x1.0000000000001p0
  },
  { // Entry 153
    -0x1.80p-52,
    -0x1.0000000000001p53,
    0x1.fffffffffffffp-1
  },
  { // Entry 154
    -0.0,
    -0x1.0000000000001p53,
    0x1.0p0
  },
  { // Entry 155
    -0.0,
    -0x1.0000000000001p53,
    0x1.0000000000001p0
  },
  { // Entry 156
    -0x1.p-53,
    -0x1.0p53,
    0x1.fffffffffffffp-1
  },
  { // Entry 157
    -0.0,
    -0x1.0p53,
    0x1.0p0
  },
  { // Entry 158
    -0x1.p-51,
    -0x1.0p53,
    0x1.0000000000001p0
  },
  { // Entry 159
    -0.0,
    -0x1.fffffffffffffp52,
    0x1.fffffffffffffp-1
  },
  { // Entry 160
    -0.0,
    -0x1.fffffffffffffp52,
    0x1.0p0
  },
  { // Entry 161
    -0x1.80p-51,
    -0x1.fffffffffffffp52,
    0x1.0000000000001p0
  },
  { // Entry 162
    0.0,
    0x1.fffffffffffffp50,
    0x1.fffffffffffffp-1
  },
  { // Entry 163
    0x1.80p-1,
    0x1.fffffffffffffp50,
    0x1.0p0
  },
  { // Entry 164
    0x1.00000000000040p-2,
    0x1.fffffffffffffp50,
    0x1.0000000000001p0
  },
  { // Entry 165
    0x1.p-2,
    0x1.0p51,
    0x1.fffffffffffffp-1
  },
  { // Entry 166
    0.0,
    0x1.0p51,
    0x1.0p0
  },
  { // Entry 167
    0x1.00000000000020p-1,
    0x1.0p51,
    0x1.0000000000001p0
  },
  { // Entry 168
    0x1.80p-1,
    0x1.0000000000001p51,
    0x1.fffffffffffffp-1
  },
  { // Entry 169
    0x1.p-1,
    0x1.0000000000001p51,
    0x1.0p0
  },
  { // Entry 170
    0.0,
    0x1.0000000000001p51,
    0x1.0000000000001p0
  },
  { // Entry 171
    0.0,
    0x1.fffffffffffffp51,
    0x1.fffffffffffffp-1
  },
  { // Entry 172
    0x1.p-1,
    0x1.fffffffffffffp51,
    0x1.0p0
  },
  { // Entry 173
    0x1.00000000000040p-1,
    0x1.fffffffffffffp51,
    0x1.0000000000001p0
  },
  { // Entry 174
    0x1.p-1,
    0x1.0p52,
    0x1.fffffffffffffp-1
  },
  { // Entry 175
    0.0,
    0x1.0p52,
    0x1.0p0
  },
  { // Entry 176
    0x1.p-52,
    0x1.0p52,
    0x1.0000000000001p0
  },
  { // Entry 177
    0x1.00000000000010p-1,
    0x1.0000000000001p52,
    0x1.fffffffffffffp-1
  },
  { // Entry 178
    0.0,
    0x1.0000000000001p52,
    0x1.0p0
  },
  { // Entry 179
    0.0,
    0x1.0000000000001p52,
    0x1.0000000000001p0
  },
  { // Entry 180
    -0.0,
    -0x1.0000000000001p53,
    -0x1.0000000000001p0
  },
  { // Entry 181
    -0.0,
    -0x1.0000000000001p53,
    -0x1.0p0
  },
  { // Entry 182
    -0x1.80p-52,
    -0x1.0000000000001p53,
    -0x1.fffffffffffffp-1
  },
  { // Entry 183
    -0x1.p-51,
    -0x1.0p53,
    -0x1.0000000000001p0
  },
  { // Entry 184
    -0.0,
    -0x1.0p53,
    -0x1.0p0
  },
  { // Entry 185
    -0x1.p-53,
    -0x1.0p53,
    -0x1.fffffffffffffp-1
  },
  { // Entry 186
    -0x1.80p-51,
    -0x1.fffffffffffffp52,
    -0x1.0000000000001p0
  },
  { // Entry 187
    -0.0,
    -0x1.fffffffffffffp52,
    -0x1.0p0
  },
  { // Entry 188
    -0.0,
    -0x1.fffffffffffffp52,
    -0x1.fffffffffffffp-1
  },
  { // Entry 189
    0x1.fffffffffffff0p1023,
    0x1.fffffffffffffp1023,
    HUGE_VAL
  },
  { // Entry 190
    0x1.fffffffffffff0p1023,
    0x1.fffffffffffffp1023,
    -HUGE_VAL
  },
  { // Entry 191
    -0x1.fffffffffffff0p1023,
    -0x1.fffffffffffffp1023,
    HUGE_VAL
  },
  { // Entry 192
    -0x1.fffffffffffff0p1023,
    -0x1.fffffffffffffp1023,
    -HUGE_VAL
  },
  { // Entry 193
    0x1.p-1022,
    0x1.0p-1022,
    HUGE_VAL
  },
  { // Entry 194
    -0x1.p-1022,
    -0x1.0p-1022,
    HUGE_VAL
  },
  { // Entry 195
    0x1.p-1022,
    0x1.0p-1022,
    -HUGE_VAL
  },
  { // Entry 196
    -0x1.p-1022,
    -0x1.0p-1022,
    -HUGE_VAL
  },
  { // Entry 197
    0x1.p-1074,
    0x1.0p-1074,
    HUGE_VAL
  },
  { // Entry 198
    -0x1.p-1074,
    -0x1.0p-1074,
    HUGE_VAL
  },
  { // Entry 199
    0x1.p-1074,
    0x1.0p-1074,
    -HUGE_VAL
  },
  { // Entry 200
    -0x1.p-1074,
    -0x1.0p-1074,
    -HUGE_VAL
  },
  { // Entry 201
    0.0,
    0.0,
    HUGE_VAL
  },
  { // Entry 202
    -0.0,
    -0.0,
    HUGE_VAL
  },
  { // Entry 203
    0.0,
    0.0,
    -HUGE_VAL
  },
  { // Entry 204
    -0.0,
    -0.0,
    -HUGE_VAL
  },
  { // Entry 205
    0.0,
    0x1.fffffffffffffp1023,
    0x1.fffffffffffffp1023
  },
  { // Entry 206
    0.0,
    0x1.fffffffffffffp1023,
    -0x1.fffffffffffffp1023
  },
  { // Entry 207
    -0.0,
    -0x1.fffffffffffffp1023,
    0x1.fffffffffffffp1023
  },
  { // Entry 208
    -0.0,
    -0x1.fffffffffffffp1023,
    -0x1.fffffffffffffp1023
  },
  { // Entry 209
    0.0,
    0x1.fffffffffffffp1023,
    0x1.0p-1022
  },
  { // Entry 210
    0.0,
    0x1.fffffffffffffp1023,
    -0x1.0p-1022
  },
  { // Entry 211
    -0.0,
    -0x1.fffffffffffffp1023,
    0x1.0p-1022
  },
  { // Entry 212
    -0.0,
    -0x1.fffffffffffffp1023,
    -0x1.0p-1022
  },
  { // Entry 213
    0.0,
    0x1.fffffffffffffp1023,
    0x1.0p-1074
  },
  { // Entry 214
    0.0,
    0x1.fffffffffffffp1023,
    -0x1.0p-1074
  },
  { // Entry 215
    -0.0,
    -0x1.fffffffffffffp1023,
    0x1.0p-1074
  },
  { // Entry 216
    -0.0,
    -0x1.fffffffffffffp1023,
    -0x1.0p-1074
  },
  { // Entry 217
    0x1.p-1022,
    0x1.0p-1022,
    0x1.fffffffffffffp1023
  },
  { // Entry 218
    -0x1.p-1022,
    -0x1.0p-1022,
    0x1.fffffffffffffp1023
  },
  { // Entry 219
    0x1.p-1022,
    0x1.0p-1022,
    -0x1.fffffffffffffp1023
  },
  { // Entry 220
    -0x1.p-1022,
    -0x1.0p-1022,
    -0x1.fffffffffffffp1023
  },
  { // Entry 221
    0x1.p-1074,
    0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 222
    -0x1.p-1074,
    -0x1.0p-1074,
    0x1.fffffffffffffp1023
  },
  { // Entry 223
    0x1.p-1074,
    0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 224
    -0x1.p-1074,
    -0x1.0p-1074,
    -0x1.fffffffffffffp1023
  },
  { // Entry 225
    0.0,
    0.0,
    0x1.fffffffffffffp1023
  },
  { // Entry 226
    -0.0,
    -0.0,
    0x1.fffffffffffffp1023
  },
  { // Entry 227
    0.0,
    0.0,
    -0x1.fffffffffffffp1023
  },
  { // Entry 228
    -0.0,
    -0.0,
    -0x1.fffffffffffffp1023
  },
  { // Entry 229
    0.0,
    0x1.0p-1022,
    0x1.0p-1022
  },
  { // Entry 230
    0.0,
    0x1.0p-1022,
    -0x1.0p-1022
  },
  { // Entry 231
    -0.0,
    -0x1.0p-1022,
    0x1.0p-1022
  },
  { // Entry 232
    -0.0,
    -0x1.0p-1022,
    -0x1.0p-1022
  },
  { // Entry 233
    0x1.p-1074,
    0x1.0p-1022,
    0x1.ffffffffffffep-1023
  },
  { // Entry 234
    0x1.p-1074,
    0x1.0p-1022,
    -0x1.ffffffffffffep-1023
  },
  { // Entry 235
    -0x1.p-1074,
    -0x1.0p-1022,
    0x1.ffffffffffffep-1023
  },
  { // Entry 236
    -0x1.p-1074,
    -0x1.0p-1022,
    -0x1.ffffffffffffep-1023
  },
  { // Entry 237
    0.0,
    0x1.0p-1022,
    0x1.0p-1074
  },
  { // Entry 238
    0.0,
    0x1.0p-1022,
    -0x1.0p-1074
  },
  { // Entry 239
    -0.0,
    -0x1.0p-1022,
    0x1.0p-1074
  },
  { // Entry 240
    -0.0,
    -0x1.0p-1022,
    -0x1.0p-1074
  },
  { // Entry 241
    0x1.ffffffffffffe0p-1023,
    0x1.ffffffffffffep-1023,
    0x1.0p-1022
  },
  { // Entry 242
    0x1.ffffffffffffe0p-1023,
    0x1.ffffffffffffep-1023,
    -0x1.0p-1022
  },
  { // Entry 243
    -0x1.ffffffffffffe0p-1023,
    -0x1.ffffffffffffep-1023,
    0x1.0p-1022
  },
  { // Entry 244
    -0x1.ffffffffffffe0p-1023,
    -0x1.ffffffffffffep-1023,
    -0x1.0p-1022
  },
  { // Entry 245
    0x1.p-1074,
    0x1.0p-1074,
    0x1.0p-1022
  },
  { // Entry 246
    0x1.p-1074,
    0x1.0p-1074,
    -0x1.0p-1022
  },
  { // Entry 247
    -0x1.p-1074,
    -0x1.0p-1074,
    0x1.0p-1022
  },
  { // Entry 248
    -0x1.p-1074,
    -0x1.0p-1074,
    -0x1.0p-1022
  },
  { // Entry 249
    0.0,
    0.0,
    0x1.0p-1022
  },
  { // Entry 250
    0.0,
    0.0,
    -0x1.0p-1022
  },
  { // Entry 251
    -0.0,
    -0.0,
    0x1.0p-1022
  },
  { // Entry 252
    -0.0,
    -0.0,
    -0x1.0p-1022
  },
  { // Entry 253
    0.0,
    0x1.0p-1074,
    0x1.0p-1074
  },
  { // Entry 254
    0.0,
    0x1.0p-1074,
    -0x1.0p-1074
  },
  { // Entry 255
    -0.0,
    -0x1.0p-1074,
    0x1.0p-1074
  },
  { // Entry 256
    -0.0,
    -0x1.0p-1074,
    -0x1.0p-1074
  },
  { // Entry 257
    0.0,
    0.0,
    0x1.0p-1074
  },
  { // Entry 258
    0.0,
    0.0,
    -0x1.0p-1074
  },
  { // Entry 259
    -0.0,
    -0.0,
    0x1.0p-1074
  },
  { // Entry 260
    -0.0,
    -0.0,
    -0x1.0p-1074
  },
  { // Entry 261
    -0x1.8fd90479094320p-964,
    -0x1.398dd069017ffp759,
    -0x1.b148e36fdec2fp-964
  }
};
