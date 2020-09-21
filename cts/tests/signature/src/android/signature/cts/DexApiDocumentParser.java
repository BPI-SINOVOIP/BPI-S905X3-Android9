/*
 * Copyright (C) 2018 The Android Open Source Project
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
package android.signature.cts;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Spliterator;
import java.util.function.Consumer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;
import java.text.ParseException;

/**
 * Parses an API definition given as a text file with DEX signatures of class
 * members. Constructs a {@link DexApiDocumentParser.DexMember} for every class
 * member.
 *
 * <p>The definition file is converted into a {@link Stream} of
 * {@link DexApiDocumentParser.DexMember}.
 */
public class DexApiDocumentParser {

    public Stream<DexMember> parseAsStream(InputStream inputStream) {
        BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
        return StreamSupport.stream(new DexApiSpliterator(reader), false);
    }

    private static class DexApiSpliterator implements Spliterator<DexMember> {
        private final BufferedReader mReader;
        private int mLineNum;

        private static final Pattern REGEX_CLASS = Pattern.compile("^L[^->]*;$");
        private static final Pattern REGEX_FIELD = Pattern.compile("^(L[^->]*;)->(.*):(.*)$");
        private static final Pattern REGEX_METHOD =
                Pattern.compile("^(L[^->]*;)->(.*)(\\(.*\\).*)$");

        DexApiSpliterator(BufferedReader reader) {
            mReader = reader;
            mLineNum = 0;
        }

        @Override
        public boolean tryAdvance(Consumer<? super DexMember> action) {
            DexMember nextMember = null;
            try {
                nextMember = next();
            } catch (IOException | ParseException ex) {
                throw new RuntimeException(ex);
            }
            if (nextMember == null) {
                return false;
            }
            action.accept(nextMember);
            return true;
        }

        @Override
        public Spliterator<DexMember> trySplit() {
            return null;
        }

        @Override
        public long estimateSize() {
            return Long.MAX_VALUE;
        }

        @Override
        public int characteristics() {
            return ORDERED | DISTINCT | NONNULL | IMMUTABLE;
        }

        /**
         * Parses lines of DEX signatures from `mReader`. The following three
         * line formats are supported:
         * 1) [class descriptor]
         *      - e.g. Lcom/example/MyClass;
         *      - these lines are ignored
         * 2) [class descriptor]->[field name]:[field type]
         *      - e.g. Lcom/example/MyClass;->myField:I
         *      - these lines are parsed as field signatures
         * 3) [class descriptor]->[method name]([method parameter types])[method return type]
         *      - e.g. Lcom/example/MyClass;->myMethod(Lfoo;Lbar;)J
         *      - these lines are parsed as method signatures
         */
        private DexMember next() throws IOException, ParseException {
            while (true) {
                // Read the next line from the input.
                String line = mReader.readLine();
                if (line == null) {
                    // End of stream.
                    return null;
                }

                // Increment the line number.
                mLineNum = mLineNum + 1;

                // Match line against regex patterns.
                Matcher matchClass = REGEX_CLASS.matcher(line);
                Matcher matchField = REGEX_FIELD.matcher(line);
                Matcher matchMethod = REGEX_METHOD.matcher(line);

                // Check that *exactly* one pattern matches.
                int matchCount = (matchClass.matches() ? 1 : 0) + (matchField.matches() ? 1 : 0) +
                        (matchMethod.matches() ? 1 : 0);
                if (matchCount == 0) {
                    throw new ParseException("Could not parse: \"" + line + "\"", mLineNum);
                } else if (matchCount > 1) {
                    throw new ParseException("Ambiguous parse: \"" + line + "\"", mLineNum);
                }

                // Extract information from the line.
                if (matchClass.matches()) {
                    // We ignore lines describing a class because classes are
                    // not being hidden.
                } else if (matchField.matches()) {
                    return new DexField(
                            matchField.group(1), matchField.group(2), matchField.group(3));
                } else if (matchMethod.matches()) {
                    return new DexMethod(
                            matchMethod.group(1),matchMethod.group(2), matchMethod.group(3));
                } else {
                    throw new IllegalStateException();
                }
            }
        }
    }
}
