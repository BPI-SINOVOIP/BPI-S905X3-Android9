; Copyright (C) 2008 The Android Open Source Project
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;      http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.

.source T_filled_new_array_range_2.java
.class public dot.junit.opcodes.filled_new_array_range.d.T_filled_new_array_range_2
.super java/lang/Object


.method public <init>()V
.limit regs 1

       invoke-direct {v0}, java/lang/Object/<init>()V
       return-void
.end method

.method public run(Ljava/lang/Object;Ljava/lang/Object;)[Ljava/lang/Object;
.limit regs 10
    filled-new-array/range {v8..v9}, [Ljava/lang/Object;
    move-result-object v0
    return-object v0
.end method


