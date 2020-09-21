/*
 * Copyright 2016, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.jf.smalidea.debugging.value;

import com.intellij.openapi.project.Project;
import com.sun.jdi.PrimitiveValue;
import org.jf.smalidea.psi.impl.SmaliMethod;

public class LazyPrimitiveValue<T extends PrimitiveValue> extends LazyValue<T> implements PrimitiveValue {
    public LazyPrimitiveValue(SmaliMethod method, Project project, int registerNumber, String type) {
        super(method, project, registerNumber, type);
    }

    @Override public boolean booleanValue() {
        return getValue().booleanValue();
    }

    @Override public byte byteValue() {
        return getValue().byteValue();
    }

    @Override public char charValue() {
        return getValue().charValue();
    }

    @Override public double doubleValue() {
        return getValue().doubleValue();
    }

    @Override public float floatValue() {
        return getValue().floatValue();
    }

    @Override public int intValue() {
        return getValue().intValue();
    }

    @Override public long longValue() {
        return getValue().longValue();
    }

    @Override public short shortValue() {
        return getValue().shortValue();
    }

    @Override public String toString() {
        return getValue().toString();
    }
}
