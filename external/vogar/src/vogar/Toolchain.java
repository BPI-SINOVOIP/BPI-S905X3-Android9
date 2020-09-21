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

package vogar;

import vogar.Dexer;

public enum Toolchain {
    // .dex: desugar + javac + dx
    DX,
    // .dex: desugar + javac + d8
    D8,
    // .class: javac
    JAVAC;

    public Dexer getDexer() {
        if (this == Toolchain.DX) {
            return Dexer.DX;
        } else if (this == Toolchain.D8) {
            return Dexer.D8;
        }
        throw new IllegalStateException("No dexer for toolchain " + this);
    }
}
