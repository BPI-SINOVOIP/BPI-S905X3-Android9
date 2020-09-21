/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package org.apache.harmony.tests.java.io;

import java.io.ByteArrayInputStream;
import java.io.CharArrayReader;
import java.io.IOException;
import java.io.PipedInputStream;
import java.io.PipedOutputStream;
import java.io.Reader;
import java.io.StreamTokenizer;
import java.io.StringBufferInputStream;

import tests.support.Support_StringReader;

public class StreamTokenizerTest extends junit.framework.TestCase {
    Support_StringReader r;

    StreamTokenizer st;

    String testString;

    /**
     * java.io.StreamTokenizer#StreamTokenizer(java.io.InputStream)
     */
    @SuppressWarnings("deprecation")
    public void test_ConstructorLjava_io_InputStream() throws IOException {
        st = new StreamTokenizer(new StringBufferInputStream(
                "/comments\n d 8 'h'"));

        assertEquals("the next token returned should be the letter d",
                StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals("the next token returned should be the letter d",
                "d", st.sval);

        assertEquals("the next token returned should be the digit 8",
                StreamTokenizer.TT_NUMBER, st.nextToken());
        assertEquals("the next token returned should be the digit 8",
                8.0, st.nval);

        assertEquals("the next token returned should be the quote character",
                39, st.nextToken());
        assertEquals("the next token returned should be the quote character",
                "h", st.sval);
    }

    /**
     * java.io.StreamTokenizer#StreamTokenizer(java.io.Reader)
     */
    public void test_ConstructorLjava_io_Reader() throws IOException {
        setTest("/testing\n d 8 'h' ");
        assertEquals("the next token returned should be the letter d skipping the comments",
                StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals("the next token returned should be the letter d",
                "d", st.sval);

        assertEquals("the next token returned should be the digit 8",
                StreamTokenizer.TT_NUMBER, st.nextToken());
        assertEquals("the next token returned should be the digit 8",
                8.0, st.nval);

        assertEquals("the next token returned should be the quote character",
                39, st.nextToken());
        assertEquals("the next token returned should be the quote character",
                "h", st.sval);
    }

    /**
     * java.io.StreamTokenizer#commentChar(int)
     */
    public void test_commentCharI() throws IOException {
        setTest("*comment \n / 8 'h' ");
        st.ordinaryChar('/');
        st.commentChar('*');
        assertEquals("nextToken() did not return the character / skiping the comments starting with *",
                47, st.nextToken());
        assertTrue("the next token returned should be the digit 8", st
                .nextToken() == StreamTokenizer.TT_NUMBER
                && st.nval == 8.0);
        assertTrue("the next token returned should be the quote character",
                st.nextToken() == 39 && st.sval.equals("h"));
    }

    /**
     * java.io.StreamTokenizer#eolIsSignificant(boolean)
     */
    public void test_eolIsSignificantZ() throws IOException {
        setTest("d 8\n");
        // by default end of line characters are not significant
        assertTrue("nextToken did not return d",
                st.nextToken() == StreamTokenizer.TT_WORD
                        && st.sval.equals("d"));
        assertTrue("nextToken did not return 8",
                st.nextToken() == StreamTokenizer.TT_NUMBER
                        && st.nval == 8.0);
        assertTrue("nextToken should be the end of file",
                st.nextToken() == StreamTokenizer.TT_EOF);
        setTest("d\n");
        st.eolIsSignificant(true);
        // end of line characters are significant
        assertTrue("nextToken did not return d",
                st.nextToken() == StreamTokenizer.TT_WORD
                        && st.sval.equals("d"));
        assertTrue("nextToken is the end of line",
                st.nextToken() == StreamTokenizer.TT_EOL);
    }

    /**
     * java.io.StreamTokenizer#lineno()
     */
    public void test_lineno() throws IOException {
        setTest("d\n 8\n");
        assertEquals("the lineno should be 1", 1, st.lineno());
        st.nextToken();
        st.nextToken();
        assertEquals("the lineno should be 2", 2, st.lineno());
        st.nextToken();
        assertEquals("the next line no should be 3", 3, st.lineno());
    }

    /**
     * java.io.StreamTokenizer#lowerCaseMode(boolean)
     */
    public void test_lowerCaseModeZ() throws Exception {
        // SM.
        setTest("HELLOWORLD");
        st.lowerCaseMode(true);

        st.nextToken();
        assertEquals("sval not converted to lowercase.", "helloworld", st.sval
        );
    }

    /**
     * java.io.StreamTokenizer#nextToken()
     */
    @SuppressWarnings("deprecation")
    public void test_nextToken() throws IOException {
        // SM.
        setTest("\r\n/* fje fje 43.4 f \r\n f g */  456.459 \r\n"
                + "Hello  / 	\r\n \r\n \n \r \257 Hi \'Hello World\'");
        st.ordinaryChar('/');
        st.slashStarComments(true);
        st.nextToken();
        assertTrue("Wrong Token type1: " + (char) st.ttype,
                st.ttype == StreamTokenizer.TT_NUMBER);
        st.nextToken();
        assertTrue("Wrong Token type2: " + st.ttype,
                st.ttype == StreamTokenizer.TT_WORD);
        st.nextToken();
        assertTrue("Wrong Token type3: " + st.ttype, st.ttype == '/');
        st.nextToken();
        assertTrue("Wrong Token type4: " + st.ttype,
                st.ttype == StreamTokenizer.TT_WORD);
        st.nextToken();
        assertTrue("Wrong Token type5: " + st.ttype,
                st.ttype == StreamTokenizer.TT_WORD);
        st.nextToken();
        assertTrue("Wrong Token type6: " + st.ttype, st.ttype == '\'');
        assertTrue("Wrong Token type7: " + st.ttype, st.sval
                .equals("Hello World"));
        st.nextToken();
        assertTrue("Wrong Token type8: " + st.ttype, st.ttype == -1);

        final PipedInputStream pin = new PipedInputStream();
        PipedOutputStream pout = new PipedOutputStream(pin);
        pout.write("hello\n\r\r".getBytes("UTF-8"));
        StreamTokenizer s = new StreamTokenizer(pin);
        s.eolIsSignificant(true);
        assertTrue("Wrong token 1,1",
                s.nextToken() == StreamTokenizer.TT_WORD
                        && s.sval.equals("hello"));
        assertTrue("Wrong token 1,2", s.nextToken() == '\n');
        assertTrue("Wrong token 1,3", s.nextToken() == '\n');
        assertTrue("Wrong token 1,4", s.nextToken() == '\n');
        pout.close();
        assertTrue("Wrong token 1,5",
                s.nextToken() == StreamTokenizer.TT_EOF);
        StreamTokenizer tokenizer = new StreamTokenizer(
                new Support_StringReader("\n \r\n#"));
        tokenizer.ordinaryChar('\n'); // make \n ordinary
        tokenizer.eolIsSignificant(true);
        assertTrue("Wrong token 2,1", tokenizer.nextToken() == '\n');
        assertTrue("Wrong token 2,2", tokenizer.nextToken() == '\n');
        assertEquals("Wrong token 2,3", '#', tokenizer.nextToken());
    }

    /**
     * java.io.StreamTokenizer#ordinaryChar(int)
     */
    public void test_ordinaryCharI() throws IOException {
        // SM.
        setTest("Ffjein 893");
        st.ordinaryChar('F');
        st.nextToken();
        assertTrue("OrdinaryChar failed." + (char) st.ttype,
                st.ttype == 'F');
    }

    /**
     * java.io.StreamTokenizer#ordinaryChars(int, int)
     */
    public void test_ordinaryCharsII() throws IOException {
        // SM.
        setTest("azbc iof z 893");
        st.ordinaryChars('a', 'z');
        assertEquals("OrdinaryChars failed.", 'a', st.nextToken());
        assertEquals("OrdinaryChars failed.", 'z', st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#parseNumbers()
     */
    public void test_parseNumbers() throws IOException {
        // SM
        setTest("9.9 678");
        assertTrue("Base behavior failed.",
                st.nextToken() == StreamTokenizer.TT_NUMBER);
        st.ordinaryChars('0', '9');
        assertEquals("setOrdinary failed.", '6', st.nextToken());
        st.parseNumbers();
        assertTrue("parseNumbers failed.",
                st.nextToken() == StreamTokenizer.TT_NUMBER);
    }

    /**
     * java.io.StreamTokenizer#pushBack()
     */
    public void test_pushBack() throws IOException {
        // SM.
        setTest("Hello 897");
        st.nextToken();
        st.pushBack();
        assertTrue("PushBack failed.",
                st.nextToken() == StreamTokenizer.TT_WORD);
    }

    /**
     * java.io.StreamTokenizer#quoteChar(int)
     */
    public void test_quoteCharI() throws IOException {
        // SM
        setTest("<Hello World<    HelloWorldH");
        st.quoteChar('<');
        assertEquals("QuoteChar failed.", '<', st.nextToken());
        assertEquals("QuoteChar failed.", "Hello World", st.sval);
        st.quoteChar('H');
        st.nextToken();
        assertEquals("QuoteChar failed for word.", "elloWorld", st.sval
        );
    }

    /**
     * java.io.StreamTokenizer#resetSyntax()
     */
    public void test_resetSyntax() throws IOException {
        // SM
        setTest("H 9\' ello World");
        st.resetSyntax();
        assertTrue("resetSyntax failed1." + (char) st.ttype,
                st.nextToken() == 'H');
        assertTrue("resetSyntax failed1." + (char) st.ttype,
                st.nextToken() == ' ');
        assertTrue("resetSyntax failed2." + (char) st.ttype,
                st.nextToken() == '9');
        assertTrue("resetSyntax failed3." + (char) st.ttype,
                st.nextToken() == '\'');
    }

    /**
     * java.io.StreamTokenizer#slashSlashComments(boolean)
     */
    public void test_slashSlashCommentsZ() throws IOException {
        // SM.
        setTest("// foo \r\n /fiji \r\n -456");
        st.ordinaryChar('/');
        st.slashSlashComments(true);
        assertEquals("Test failed.", '/', st.nextToken());
        assertTrue("Test failed.",
                st.nextToken() == StreamTokenizer.TT_WORD);
    }

    /**
     * java.io.StreamTokenizer#slashSlashComments(boolean)
     */
    public void test_slashSlashComments_withSSOpen() throws IOException {
        Reader reader = new CharArrayReader("t // t t t".toCharArray());

        StreamTokenizer st = new StreamTokenizer(reader);
        st.slashSlashComments(true);

        assertEquals(StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals(StreamTokenizer.TT_EOF, st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#slashSlashComments(boolean)
     */
    public void test_slashSlashComments_withSSOpen_NoComment() throws IOException {
        Reader reader = new CharArrayReader("// t".toCharArray());

        StreamTokenizer st = new StreamTokenizer(reader);
        st.slashSlashComments(true);
        st.ordinaryChar('/');

        assertEquals(StreamTokenizer.TT_EOF, st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#slashSlashComments(boolean)
     */
    public void test_slashSlashComments_withSSClosed() throws IOException {
        Reader reader = new CharArrayReader("// t".toCharArray());

        StreamTokenizer st = new StreamTokenizer(reader);
        st.slashSlashComments(false);
        st.ordinaryChar('/');

        assertEquals('/', st.nextToken());
        assertEquals('/', st.nextToken());
        assertEquals(StreamTokenizer.TT_WORD, st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#slashStarComments(boolean)
     */
    public void test_slashStarCommentsZ() throws IOException {
        setTest("/* foo \r\n /fiji \r\n*/ -456");
        st.ordinaryChar('/');
        st.slashStarComments(true);
        assertTrue("Test failed.",
                st.nextToken() == StreamTokenizer.TT_NUMBER);
    }

    /**
     * java.io.StreamTokenizer#slashStarComments(boolean)
     */
    public void test_slashStarComments_withSTOpen() throws IOException {
        Reader reader = new CharArrayReader("t /* t */ t".toCharArray());

        StreamTokenizer st = new StreamTokenizer(reader);
        st.slashStarComments(true);

        assertEquals(StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals(StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals(StreamTokenizer.TT_EOF, st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#slashStarComments(boolean)
     */
    public void test_slashStarComments_withSTClosed() throws IOException {
        Reader reader = new CharArrayReader("t /* t */ t".toCharArray());

        StreamTokenizer st = new StreamTokenizer(reader);
        st.slashStarComments(false);

        assertEquals(StreamTokenizer.TT_WORD, st.nextToken());
        assertEquals(StreamTokenizer.TT_EOF, st.nextToken());
    }

    /**
     * java.io.StreamTokenizer#toString()
     */
    public void test_toString() throws IOException {
        setTest("ABC Hello World");
        st.nextToken();
        assertTrue("toString failed." + st.toString(),
                st.toString().equals(
                        "Token[ABC], line 1"));

        // Regression test for HARMONY-4070
        byte[] data = new byte[] { (byte) '-' };
        StreamTokenizer tokenizer = new StreamTokenizer(
                new ByteArrayInputStream(data));
        tokenizer.nextToken();
        String result = tokenizer.toString();
        assertEquals("Token['-'], line 1", result);
    }

    /**
     * java.io.StreamTokenizer#whitespaceChars(int, int)
     */
    public void test_whitespaceCharsII() throws IOException {
        setTest("azbc iof z 893");
        st.whitespaceChars('a', 'z');
        assertTrue("OrdinaryChar failed.",
                st.nextToken() == StreamTokenizer.TT_NUMBER);
    }

    /**
     * java.io.StreamTokenizer#wordChars(int, int)
     */
    public void test_wordCharsII() throws IOException {
        setTest("A893 -9B87");
        st.wordChars('0', '9');
        assertTrue("WordChar failed1.",
                st.nextToken() == StreamTokenizer.TT_WORD);
        assertEquals("WordChar failed2.", "A893", st.sval);
        assertTrue("WordChar failed3.",
                st.nextToken() == StreamTokenizer.TT_NUMBER);
        st.nextToken();
        assertEquals("WordChar failed4.", "B87", st.sval);

        setTest("    Hello World");
        st.wordChars(' ', ' ');
        st.nextToken();
        assertEquals("WordChars failed for whitespace.", "Hello World", st.sval
        );

        setTest("    Hello World\r\n  \'Hello World\' Hello\' World");
        st.wordChars(' ', ' ');
        st.wordChars('\'', '\'');
        st.nextToken();
        assertTrue("WordChars failed for whitespace: " + st.sval, st.sval
                .equals("Hello World"));
        st.nextToken();
        assertTrue("WordChars failed for quote1: " + st.sval, st.sval
                .equals("\'Hello World\' Hello\' World"));
    }

    private void setTest(String s) {
        testString = s;
        r = new Support_StringReader(testString);
        st = new StreamTokenizer(r);
    }

    protected void setUp() {
    }

    protected void tearDown() {
    }
}
