#!/usr/bin/env python
#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

arm = """
Build fingerprint: 'Android/aosp_arm/generic_arm:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
Revision: '0'
ABI: 'arm'
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
    r0 00000000  r1 00002dd9  r2 00000006  r3 00000000
    r4 f710edd8  r5 00000006  r6 00000000  r7 0000010c
    r8 f71b9df4  r9 ab0b5028  sl f7175695  fp f710edd0
    ip 00002dd9  sp f710ed18  lr f7175ef1  pc f719a4e0  cpsr 60070010
    d0  ffffffffffffffff  d1  0000000000000031
    d2  0000000000000037  d3  0000000000000033
    d4  0000000000000000  d5  0000000000000000
    d6  0000000000000000  d7  0000000000000000
    d8  0000000000000000  d9  0000000000000000
    d10 0000000000000000  d11 0000000000000000
    d12 0000000000000000  d13 0000000000000000
    d14 0000000000000000  d15 0000000000000000
    d16 0000000000000000  d17 0000000000000fff
    d18 0000000000000000  d19 0000000000000000
    d20 0000000000000000  d21 0000000000000000
    d22 0000000000000000  d23 0000000000000000
    d24 0000000000000000  d25 0000000000000000
    d26 0000000000000000  d27 0000000000000000
    d28 0000000000000000  d29 0000000000000000
    d30 0000000000000000  d31 0000000000000000
    scr 00000000

backtrace:
    #00 pc 000374e0  /system/lib/libc.so (tgkill+12)
    #01 pc 00012eed  /system/lib/libc.so (pthread_kill+52)
    #02 pc 00013997  /system/lib/libc.so (raise+10)
    #03 pc 0001047d  /system/lib/libc.so (__libc_android_abort+36)
    #04 pc 0000eb1c  /system/lib/libc.so (abort+4)
    #05 pc 00000c6f  /system/xbin/crasher
    #06 pc 000126b3  /system/lib/libc.so (__pthread_start(void*)+30)
    #07 pc 000107fb  /system/lib/libc.so (__start_thread+6)
"""

arm64 = """
Build fingerprint: 'Android/aosp_arm64/generic_arm64:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
Revision: '0'
ABI: 'arm64'
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
    x0   0000000000000000  x1   0000000000002df1  x2   0000000000000006  x3   000000559dc73040
    x4   ffffffffffffffff  x5   0000000000000005  x6   0000000000000001  x7   0000000000000020
    x8   0000000000000083  x9   0000005563d21000  x10  0101010101010101  x11  0000000000000001
    x12  0000000000000001  x13  0000005563d21000  x14  0000005563d21000  x15  0000000000000000
    x16  0000005563d32f20  x17  0000000000000001  x18  0000000000000000  x19  000000559dc73040
    x20  0000007f844dcbb0  x21  0000007f84639000  x22  0000000000000000  x23  0000000000000006
    x24  0000007f845b2000  x25  0000007ff8f33bc0  x26  0000007f843df000  x27  000000559dc730c0
    x28  0000007f84639788  x29  0000007f844dc9c0  x30  0000007f845b38c4
    sp   0000007f844dc9c0  pc   0000007f845f28e0
    v0   2f2f2f2f2f2f2f2f  v1   5f6474656e62696c  v2   000000000000006f  v3   0000000000000000
    v4   8020080280200800  v5   0000000000000000  v6   0000000000000000  v7   8020080280200802
    v8   0000000000000000  v9   0000000000000000  v10  0000000000000000  v11  0000000000000000
    v12  0000000000000000  v13  0000000000000000  v14  0000000000000000  v15  0000000000000000
    v16  4010040140100401  v17  0000aaa800000000  v18  8020080280200800  v19  0000000000000000
    v20  0000000000000000  v21  0000000000000000  v22  0000000000000000  v23  0000000000000000
    v24  0000000000000000  v25  0000000000000000  v26  0000000000000000  v27  0000000000000000
    v28  0000000000000000  v29  0000000000000000  v30  0000000000000000  v31  0000000000000000

backtrace:
    #00 pc 00000000000588e0  /system/lib64/libc.so (tgkill+8)
    #01 pc 00000000000198c0  /system/lib64/libc.so (pthread_kill+160)
    #02 pc 000000000001ab34  /system/lib64/libc.so (raise+28)
    #03 pc 00000000000148bc  /system/lib64/libc.so (abort+60)
    #04 pc 00000000000016e0  /system/xbin/crasher64
    #05 pc 00000000000017f0  /system/xbin/crasher64
    #06 pc 0000000000018958  /system/lib64/libc.so (__pthread_start(void*)+52)
    #07 pc 0000000000014e90  /system/lib64/libc.so (__start_thread+16)
"""

mips = """
Build fingerprint: 'Android/aosp_mips/generic_mips:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
Revision: '0'
ABI: 'mips'
pid: 958, tid: 960, name: crasher  >>> crasher <<<
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
 zr 00000000  at 802babc0  v0 00000000  v1 77b99dd0
 a0 000003be  a1 000003c0  a2 00000006  a3 00000000
 t0 00000000  t1 9e7f5440  t2 00000020  t3 ffffff18
 t4 77a9c000  t5 00000001  t6 00000000  t7 00000000
 s0 000003c0  s1 77b99dd8  s2 00000000  s3 00000006
 s4 77db2028  s5 000003be  s6 77c39fa8  s7 77b99dd0
 t8 00000000  t9 77c89e80  k0 00000000  k1 00000000
 gp 77cce350  sp 77b99c78  s8 77db2020  ra 77c3b48c
 hi 00000000  lo 00000008 bva 7fff7008 epc 77c89e94

backtrace:
    #00 pc 00067e94  /system/lib/libc.so (tgkill+20)
    #01 pc 0001948c  /system/lib/libc.so (pthread_kill+244)
    #02 pc 0001b0e8  /system/lib/libc.so (raise+60)
    #03 pc 00012908  /system/lib/libc.so (abort+104)
    #04 pc 000012a4  /system/xbin/crasher
    #05 pc 00018008  /system/lib/libc.so (__pthread_start(void*)+96)
    #06 pc 00013198  /system/lib/libc.so (__start_thread+36)
"""

mips64 = """
Build fingerprint: 'Android/aosp_mips64/generic_mips64:5.1.51/AOSP/agampe05040015:userdebug/test-keys'
Revision: '1'
ABI: 'mips64'
pid: 342, tid: 342, name: crasher64  >>> crasher64 <<<
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
 zr 0000000000000000  at 0000000000000001  v0 0000000000000000  v1 000000ffec1c6528
 a0 0000000000000156  a1 0000000000000156  a2 0000000000000006  a3 0000000000000000
 a4 000000000000ffff  a5 fffffffffffffffc  a6 0000000000000000  a7 0000000000000001
 t0 0000000000000001  t1 0000000000000000  t2 0000000000000001  t3 0000000000000001
 s0 0000000000000002  s1 000000ffec1c6538  s2 000000ffec1c6478  s3 0000000000000006
 s4 0000000000100000  s5 000000fff1d44f98  s6 000000fff186c488  s7 0000000000000000
 t8 ffffffffffff0000  t9 000000ffec01c2a0  k0 0000000000000000  k1 0000000000000000
 gp 000000ffec0a6680  sp 000000ffff8c7150  s8 0000000000100206  ra 000000ffec016684
 hi 0000000000000000  lo 0000000000000000 bva 000000ffffffe010 epc 000000ffec01c2a8

backtrace:
    #00 pc 00000000000832a8  /system/lib64/libc.so (tgkill+8)
    #01 pc 000000000007d684  /system/lib64/libc.so (pthread_kill+116)
    #02 pc 000000000002dd78  /system/lib64/libc.so (raise+56)
    #03 pc 000000000002684c  /system/lib64/libc.so (abort+92)
    #04 pc 000000000000199c  /system/xbin/crasher64
    #05 pc 000000000002595c  /system/lib64/libc.so (__libc_init+140)
    #06 pc 0000000000000fd4  /system/xbin/crasher64
    #07 pc 0000000000000f80  /system/xbin/crasher64
"""

x86 = """
Build fingerprint: 'Android/aosp_x86_64/generic_x86_64:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
Revision: '0'
ABI: 'x86'
pid: 1566, tid: 1568, name: crasher  >>> crasher <<<
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
    eax 00000000  ebx 0000061e  ecx 00000620  edx 00000006
    esi f7679dd8  edi 00000000
    xcs 00000023  xds 0000002b  xes 0000002b  xfs 00000003  xss 0000002b
    eip f7758ea6  ebp 00000620  esp f7679c60  flags 00000282

backtrace:
    #00 pc 00076ea6  /system/lib/libc.so (tgkill+22)
    #01 pc 0001dc8b  /system/lib/libc.so (pthread_kill+155)
    #02 pc 0001f294  /system/lib/libc.so (raise+36)
    #03 pc 00017a04  /system/lib/libc.so (abort+84)
    #04 pc 00001099  /system/xbin/crasher
    #05 pc 0001cd58  /system/lib/libc.so (__pthread_start(void*)+56)
    #06 pc 00018169  /system/lib/libc.so (__start_thread+25)
    #07 pc 0000ed76  /system/lib/libc.so (__bionic_clone+70)
"""

x86_64 = """
Build fingerprint: 'Android/aosp_x86_64/generic_x86_64:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
Revision: '0'
ABI: 'x86_64'
pid: 1608, tid: 1610, name: crasher64  >>> crasher64 <<<
signal 6 (SIGABRT), code -6 (SI_TKILL), fault addr --------
    rax 0000000000000000  rbx 000000000000064a  rcx ffffffffffffffff  rdx 0000000000000006
    rsi 000000000000064a  rdi 0000000000000648
    r8  0000000000000001  r9  00007fe218110c98  r10 0000000000000008  r11 0000000000000206
    r12 0000000000000000  r13 0000000000000006  r14 00007fe218111ba0  r15 0000000000000648
    cs  0000000000000033  ss  000000000000002b
    rip 00007fe218201807  rbp 00007fe218111bb0  rsp 00007fe218111a18  eflags 0000000000000206

backtrace:
    #00 pc 0000000000077807  /system/lib64/libc.so (tgkill+7)
    #01 pc 000000000002243f  /system/lib64/libc.so (pthread_kill+143)
    #02 pc 0000000000023551  /system/lib64/libc.so (raise+17)
    #03 pc 000000000001ce6d  /system/lib64/libc.so (abort+61)
    #04 pc 0000000000001385  /system/xbin/crasher64
    #05 pc 00000000000014a8  /system/xbin/crasher64
    #06 pc 00000000000215ae  /system/lib64/libc.so (__pthread_start(void*)+46)
    #07 pc 000000000001d3eb  /system/lib64/libc.so (__start_thread+11)
    #08 pc 00000000000138f5  /system/lib64/libc.so (__bionic_clone+53)
"""

libmemunreachable = """
 Unreachable memory
  48 bytes in 2 unreachable allocations
  ABI: 'arm'

  24 bytes unreachable at a11e6748
   and 24 similar unreachable bytes in 1 allocation
   contents:
   a11e6748: 63 6f 6d 2e 61 6e 64 72 6f 69 64 2e 73 79 73 74 com.android.syst
   a11e6758: 65 6d 75 69 00 00 00 00                         emui....
          #00  pc 000076ae  /system/lib/libcutils.so (set_process_name+45)
          #01  pc 000989d6  /system/lib/libandroid_runtime.so (android_os_Process_setArgV0(_JNIEnv*, _jobject*, _jstring*)+125)
"""

# This is a long crash in ASAN format, which does not pad frame numbers. This should be used
# in a test to ensure that the stack is not split into two (see stack_core's test_long_asan_crash).
long_asan_crash = """
Build fingerprint: 'Android/aosp_arm/generic_arm:4.4.3.43.43.43/AOSP/enh06302258:eng/test-keys'
ABI: 'arm'

     #0 0x727d4dfdaf  (/system/lib/libclang_rt.asan-arm-android.so+0x31daf)

     #1 0x727d4e00af  (/system/lib/libclang_rt.asan-arm-android.so+0x320af)

     #2 0x72778db0cf  (/data/lib/libc.so+0x740cf)

     #3 0x725688a66f  (/does/not/matter/a.so+0x1066f)

     #4 0x72568a02af  (/does/not/matter/a.so+0x262af)

     #5 0x725689e313  (/does/not/matter/a.so+0x24313)

     #6 0x72568a95eb  (/does/not/matter/a.so+0x2f5eb)

     #7 0x725688de6f  (/does/not/matter/a.so+0x13e6f)

     #8 0x72778ceeff  (/does/not/matter/a.so+0x67eff)

     #9 0x7277884983  (/does/not/matter/a.so+0x1d983)

     #10 0x7277884983  (/does/not/matter/a.so+0x1d983)
"""
