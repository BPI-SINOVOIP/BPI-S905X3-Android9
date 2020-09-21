/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.util;

import com.android.ddmlib.Log;

import java.util.ArrayList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class QuotationAwareTokenizer {
    private static final String LOG_TAG = "TOKEN";

    /**
     * Tokenizes the string, splitting on specified delimiter.  Does not split between consecutive,
     * unquoted double-quote marks.
     * <p/>
     * How the tokenizer works:
     * <ol>
     *     <li> Split the string into "characters" where each "character" is either an escaped
     *          character like \" (that is, "\\\"") or a single real character like f (just "f").
     *     <li> For each "character"
     *     <ol>
     *         <li> If it's a space, finish a token unless we're being quoted
     *         <li> If it's a quotation mark, flip the "we're being quoted" bit
     *         <li> Otherwise, add it to the token being built
     *     </ol>
     *     <li> At EOL, we typically haven't added the final token to the (tokens) {@link ArrayList}
     *     <ol>
     *         <li> If the last "character" is an escape character, throw an exception; that's not
     *              valid
     *         <li> If we're in the middle of a quotation, throw an exception; that's not valid
     *         <li> Otherwise, add the final token to (tokens)
     *     </ol>
     *     <li> Return a String[] version of (tokens)
     * </ol>
     *
     * @param line A {@link String} to be tokenized
     * @return A tokenized version of the string
     * @throws IllegalArgumentException if the line cannot be parsed
     */
    public static String[] tokenizeLine(String line, String delim) throws IllegalArgumentException {
        if (line == null) {
            throw new IllegalArgumentException("line is null");
        }

        ArrayList<String> tokens = new ArrayList<String>();
        StringBuilder token = new StringBuilder();
        // This pattern matches an escaped character or a character.  Escaped char takes precedence
        final Pattern charPattern = Pattern.compile("\\\\.|.");
        final Matcher charMatcher = charPattern.matcher(line);
        String aChar = "";
        boolean quotation = false;

        Log.d(LOG_TAG, String.format("Trying to tokenize the line '%s'", line));
        while (charMatcher.find()) {
            aChar = charMatcher.group();

            if (delim.equals(aChar)) {
                if (quotation) {
                    // inside a quotation; treat spaces as part of the token
                    token.append(aChar);
                } else {
                    if (token.length() > 0) {
                        // this is the end of a non-empty token; dump it in our list of tokens,
                        // clear our temp storage, and keep rolling
                        Log.d(LOG_TAG, String.format("Finished token '%s'", token.toString()));
                        tokens.add(token.toString());
                        token.delete(0, token.length());
                    }
                    // otherwise, this is the non-first in a sequence of spaces; ignore.
                }
            } else if ("\"".equals(aChar)) {
                // unescaped quotation mark; flip quotation state
                Log.v(LOG_TAG, "Flipped quotation state");
                quotation ^= true;
            } else {
                // default case: add the character to the token being built
                token.append(aChar);
            }
        }

        if (quotation || "\\".equals(aChar)) {
            // We ended in a quotation or with an escape character; this is not valid
            throw new IllegalArgumentException("Unexpected EOL in a quotation or after an escape " +
                    "character");
        }

        // Add the final token to the tokens array.
        if (token.length() > 0) {
            Log.v(LOG_TAG, String.format("Finished final token '%s'", token.toString()));
            tokens.add(token.toString());
            token.delete(0, token.length());
        }

        String[] tokensArray = new String[tokens.size()];
        return tokens.toArray(tokensArray);
    }

    /**
     * Tokenizes the string, splitting on spaces.  Does not split between consecutive,
     * unquoted double-quote marks.
     * <p>
     * See also {@link #tokenizeLine(String, String)}
     */
    public static String[] tokenizeLine(String line) throws IllegalArgumentException {
        return tokenizeLine(line, " ");
    }

    /**
     * Perform the reverse of {@link #tokenizeLine(String)}. <br/>
     * Given array of tokens, combine them into a single line.
     *
     * @param tokens
     * @return A {@link String} created from all the tokens.
     */
    public static String combineTokens(String... tokens) {
        final Pattern wsPattern = Pattern.compile("\\s");
        StringBuilder sb = new StringBuilder();
        for (int i=0; i < tokens.length; i++) {
            final String token = tokens[i];
            final Matcher wsMatcher = wsPattern.matcher(token);
            if (wsMatcher.find()) {
                sb.append('"');
                sb.append(token);
                sb.append('"');
            } else {
                sb.append(token);
            }
            if (i < (tokens.length - 1)) {
                // don't output space after last token
                sb.append(' ');
            }
        }
        return sb.toString();
    }
}
