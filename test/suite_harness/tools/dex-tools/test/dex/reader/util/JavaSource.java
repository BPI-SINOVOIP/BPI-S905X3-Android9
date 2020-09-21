/*
 * Copyright (C) 2009 The Android Open Source Project
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

package dex.reader.util;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.URI;

import javax.tools.SimpleJavaFileObject;

/**
 * {@code JavaSource} represents an in-memory java source.
 */
public class JavaSource extends SimpleJavaFileObject {
    private String src;
    private final String sourceName;

    public JavaSource(String sourceName, String sourceCode) {
        super(URI.create("string:///" + sourceName.replace(".", "/") + ".java"),
                Kind.SOURCE);
        this.sourceName = sourceName;
        this.src = sourceCode;
    }
    
    @Override
    public String getName() {
       return sourceName;
    }
    
    public CharSequence getCharContent(boolean ignoreEncodingErrors) {
        return src;
    }

    public OutputStream openOutputStream() {
        throw new IllegalStateException();
    }

    public InputStream openInputStream() {
        return new ByteArrayInputStream(src.getBytes());
    }
}