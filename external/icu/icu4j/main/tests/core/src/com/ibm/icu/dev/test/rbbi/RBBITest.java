// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
/*
 *******************************************************************************
 * Copyright (C) 1996-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 *******************************************************************************
 */
package com.ibm.icu.dev.test.rbbi;

//Regression testing of RuleBasedBreakIterator
//
//  TODO:  These tests should be mostly retired.
//          Much of the test data that was originally here was removed when the RBBI rules
//            were updated to match the Unicode boundary TRs, and the data was found to be invalid.
//          Much of the remaining data has been moved into the rbbitst.txt test data file,
//            which is common between ICU4C and ICU4J.  The remaining test data should also be moved,
//            or simply retired if it is no longer interesting.
import java.text.CharacterIterator;
import java.util.ArrayList;
import java.util.List;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import com.ibm.icu.dev.test.TestFmwk;
import com.ibm.icu.text.BreakIterator;
import com.ibm.icu.text.RuleBasedBreakIterator;
import com.ibm.icu.util.ULocale;

@RunWith(JUnit4.class)
public class RBBITest extends TestFmwk {
    public RBBITest() {
    }



    @Test
   public void TestThaiDictionaryBreakIterator() {
       int position;
       int index;
       int result[] = { 1, 2, 5, 10, 11, 12, 11, 10, 5, 2, 1, 0 };
       char ctext[] = {
               0x0041, 0x0020,
               0x0E01, 0x0E32, 0x0E23, 0x0E17, 0x0E14, 0x0E25, 0x0E2D, 0x0E07,
               0x0020, 0x0041
               };
       String text = new String(ctext);

       ULocale locale = ULocale.createCanonical("th");
       BreakIterator b = BreakIterator.getWordInstance(locale);

       b.setText(text);

       index = 0;
       // Test forward iteration
       while ((position = b.next())!= BreakIterator.DONE) {
           if (position != result[index++]) {
               errln("Error with ThaiDictionaryBreakIterator forward iteration test at " + position + ".\nShould have been " + result[index-1]);
           }
       }

       // Test backward iteration
       while ((position = b.previous())!= BreakIterator.DONE) {
           if (position != result[index++]) {
               errln("Error with ThaiDictionaryBreakIterator backward iteration test at " + position + ".\nShould have been " + result[index-1]);
           }
       }

       //Test invalid sequence and spaces
       char text2[] = {
               0x0E01, 0x0E39, 0x0020, 0x0E01, 0x0E34, 0x0E19, 0x0E01, 0x0E38, 0x0E49, 0x0E07, 0x0020, 0x0E1B,
               0x0E34, 0x0E49, 0x0E48, 0x0E07, 0x0E2D, 0x0E22, 0x0E39, 0x0E48, 0x0E43, 0x0E19,
               0x0E16, 0x0E49, 0x0E33
       };
       int expectedWordResult[] = {
               2, 3, 6, 10, 11, 15, 17, 20, 22
       };
       int expectedLineResult[] = {
               3, 6, 11, 15, 17, 20, 22
       };
       BreakIterator brk = BreakIterator.getWordInstance(new ULocale("th"));
       brk.setText(new String(text2));
       position = index = 0;
       while ((position = brk.next()) != BreakIterator.DONE && position < text2.length) {
           if (position != expectedWordResult[index++]) {
               errln("Incorrect break given by thai word break iterator. Expected: " + expectedWordResult[index-1] + " Got: " + position);
           }
       }

       brk = BreakIterator.getLineInstance(new ULocale("th"));
       brk.setText(new String(text2));
       position = index = 0;
       while ((position = brk.next()) != BreakIterator.DONE && position < text2.length) {
           if (position != expectedLineResult[index++]) {
               errln("Incorrect break given by thai line break iterator. Expected: " + expectedLineResult[index-1] + " Got: " + position);
           }
       }
       // Improve code coverage
       if (brk.preceding(expectedLineResult[1]) != expectedLineResult[0]) {
           errln("Incorrect preceding position.");
       }
       if (brk.following(expectedLineResult[1]) != expectedLineResult[2]) {
           errln("Incorrect following position.");
       }
       int []fillInArray = new int[2];
       if (((RuleBasedBreakIterator)brk).getRuleStatusVec(fillInArray) != 1 || fillInArray[0] != 0) {
           errln("Error: Since getRuleStatusVec is not supported in DictionaryBasedBreakIterator, it should return 1 and fillInArray[0] == 0.");
       }
   }


   // TODO: Move these test cases to rbbitst.txt if they aren't there already, then remove this test. It is redundant.
    @Test
    public void TestTailoredBreaks() {
        class TBItem {
            private int     type;
            private ULocale locale;
            private String  text;
            private int[]   expectOffsets;
            TBItem(int typ, ULocale loc, String txt, int[] eOffs) {
                type          = typ;
                locale        = loc;
                text          = txt;
                expectOffsets = eOffs;
            }
            private static final int maxOffsetCount = 128;
            private boolean offsetsMatchExpected(int[] foundOffsets, int foundOffsetsLength) {
                if ( foundOffsetsLength != expectOffsets.length ) {
                    return false;
                }
                for (int i = 0; i < foundOffsetsLength; i++) {
                    if ( foundOffsets[i] != expectOffsets[i] ) {
                        return false;
                    }
                }
                return true;
            }
            private String formatOffsets(int[] offsets, int length) {
                StringBuffer buildString = new StringBuffer(4*maxOffsetCount);
                for (int i = 0; i < length; i++) {
                    buildString.append(" " + offsets[i]);
                }
                return buildString.toString();
            }

            public void doTest() {
                BreakIterator brkIter;
                switch( type ) {
                    case BreakIterator.KIND_CHARACTER: brkIter = BreakIterator.getCharacterInstance(locale); break;
                    case BreakIterator.KIND_WORD:      brkIter = BreakIterator.getWordInstance(locale); break;
                    case BreakIterator.KIND_LINE:      brkIter = BreakIterator.getLineInstance(locale); break;
                    case BreakIterator.KIND_SENTENCE:  brkIter = BreakIterator.getSentenceInstance(locale); break;
                    default: errln("Unsupported break iterator type " + type); return;
                }
                brkIter.setText(text);
                int[] foundOffsets = new int[maxOffsetCount];
                int offset, foundOffsetsCount = 0;
                // do forwards iteration test
                while ( foundOffsetsCount < maxOffsetCount && (offset = brkIter.next()) != BreakIterator.DONE ) {
                    foundOffsets[foundOffsetsCount++] = offset;
                }
                if ( !offsetsMatchExpected(foundOffsets, foundOffsetsCount) ) {
                    // log error for forwards test
                    String textToDisplay = (text.length() <= 16)? text: text.substring(0,16);
                    errln("For type " + type + " " + locale + ", text \"" + textToDisplay + "...\"" +
                            "; expect " + expectOffsets.length + " offsets:" + formatOffsets(expectOffsets, expectOffsets.length) +
                            "; found " + foundOffsetsCount + " offsets fwd:" + formatOffsets(foundOffsets, foundOffsetsCount) );
                } else {
                    // do backwards iteration test
                    --foundOffsetsCount; // back off one from the end offset
                    while ( foundOffsetsCount > 0 ) {
                        offset = brkIter.previous();
                        if ( offset != foundOffsets[--foundOffsetsCount] ) {
                            // log error for backwards test
                            String textToDisplay = (text.length() <= 16)? text: text.substring(0,16);
                            errln("For type " + type + " " + locale + ", text \"" + textToDisplay + "...\"" +
                                    "; expect " + expectOffsets.length + " offsets:" + formatOffsets(expectOffsets, expectOffsets.length) +
                                    "; found rev offset " + offset + " where expect " + foundOffsets[foundOffsetsCount] );
                            break;
                        }
                    }
                }
            }
        }
        // KIND_SENTENCE "el"
        final String elSentText     = "\u0391\u03B2, \u03B3\u03B4; \u0395 \u03B6\u03B7\u037E \u0398 \u03B9\u03BA. " +
                                      "\u039B\u03BC \u03BD\u03BE! \u039F\u03C0, \u03A1\u03C2? \u03A3";
        final int[]  elSentTOffsets = { 8, 14, 20, 27, 35, 36 };
        final int[]  elSentROffsets = {        20, 27, 35, 36 };
        // KIND_CHARACTER "th"
        final String thCharText     = "\u0E01\u0E23\u0E30\u0E17\u0E48\u0E2D\u0E21\u0E23\u0E08\u0E19\u0E32 " +
                                      "(\u0E2A\u0E38\u0E0A\u0E32\u0E15\u0E34-\u0E08\u0E38\u0E11\u0E32\u0E21\u0E32\u0E28) " +
                                      "\u0E40\u0E14\u0E47\u0E01\u0E21\u0E35\u0E1B\u0E31\u0E0D\u0E2B\u0E32 ";
        final int[]  thCharTOffsets = { 1, 2, 3, 5, 6, 7, 8, 9, 10, 11,
                                        12, 13, 15, 16, 17, 19, 20, 22, 23, 24, 25, 26, 27, 28,
                                        29, 30, 32, 33, 35, 37, 38, 39, 40, 41 };
        //starting in Unicode 6.1, root behavior should be the same as Thai above
        //final int[]  thCharROffsets = { 1,    3, 5, 6, 7, 8, 9,     11,
        //                                12, 13, 15,     17, 19, 20, 22,     24,     26, 27, 28,
        //                                29,     32, 33, 35, 37, 38,     40, 41 };

        final TBItem[] tests = {
            new TBItem( BreakIterator.KIND_SENTENCE,  new ULocale("el"),          elSentText,   elSentTOffsets   ),
            new TBItem( BreakIterator.KIND_SENTENCE,  ULocale.ROOT,               elSentText,   elSentROffsets   ),
            new TBItem( BreakIterator.KIND_CHARACTER, new ULocale("th"),          thCharText,   thCharTOffsets   ),
            new TBItem( BreakIterator.KIND_CHARACTER, ULocale.ROOT,               thCharText,   thCharTOffsets   ),
        };
        for (int iTest = 0; iTest < tests.length; iTest++) {
            tests[iTest].doTest();
        }
    }

    /* Tests the method public Object clone() */
    @Test
    public void TestClone() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        try {
            rbbi.setText((CharacterIterator) null);
            if (((RuleBasedBreakIterator) rbbi.clone()).getText() != null)
                errln("RuleBasedBreakIterator.clone() was suppose to return "
                        + "the same object because fText is set to null.");
        } catch (Exception e) {
            errln("RuleBasedBreakIterator.clone() was not suppose to return " + "an exception.");
        }
    }

    /*
     * Tests the method public boolean equals(Object that)
     */
    @Test
    public void TestEquals() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        RuleBasedBreakIterator rbbi1 = new RuleBasedBreakIterator(".;");

        // TODO: Tests when "if (fRData != other.fRData && (fRData == null || other.fRData == null))" is true

        // Tests when "if (fText == null || other.fText == null)" is true
        rbbi.setText((CharacterIterator) null);
        if (rbbi.equals(rbbi1)) {
            errln("RuleBasedBreakIterator.equals(Object) was not suppose to return "
                    + "true when the other object has a null fText.");
        }

        // Tests when "if (fText == null && other.fText == null)" is true
        rbbi1.setText((CharacterIterator) null);
        if (!rbbi.equals(rbbi1)) {
            errln("RuleBasedBreakIterator.equals(Object) was not suppose to return "
                    + "false when both objects has a null fText.");
        }

        // Tests when an exception occurs
        if (rbbi.equals(0)) {
            errln("RuleBasedBreakIterator.equals(Object) was suppose to return " + "false when comparing to integer 0.");
        }
        if (rbbi.equals(0.0)) {
            errln("RuleBasedBreakIterator.equals(Object) was suppose to return " + "false when comparing to float 0.0.");
        }
        if (rbbi.equals("0")) {
            errln("RuleBasedBreakIterator.equals(Object) was suppose to return "
                    + "false when comparing to string '0'.");
        }
    }

    /*
     * Tests the method public int first()
     */
    @Test
    public void TestFirst() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        // Tests when "if (fText == null)" is true
        rbbi.setText((CharacterIterator) null);
        assertEquals("RuleBasedBreakIterator.first()", BreakIterator.DONE, rbbi.first());

        rbbi.setText("abc");
        assertEquals("RuleBasedBreakIterator.first()", 0, rbbi.first());
        assertEquals("RuleBasedBreakIterator.next()", 1, rbbi.next());
    }

    /*
     * Tests the method public int last()
     */
    @Test
    public void TestLast() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        // Tests when "if (fText == null)" is true
        rbbi.setText((CharacterIterator) null);
        if (rbbi.last() != BreakIterator.DONE) {
            errln("RuleBasedBreakIterator.last() was suppose to return "
                    + "BreakIterator.DONE when the object has a null fText.");
        }
    }

    /*
     * Tests the method public int following(int offset)
     */
    @Test
    public void TestFollowing() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        // Tests when "else if (offset < fText.getBeginIndex())" is true
        rbbi.setText("dummy");
        if (rbbi.following(-1) != 0) {
            errln("RuleBasedBreakIterator.following(-1) was suppose to return "
                    + "0 when the object has a fText of dummy.");
        }
    }

    /*
     * Tests the method public int preceding(int offset)
     */
    @Test
    public void TestPreceding() {
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        // Tests when "if (fText == null || offset > fText.getEndIndex())" is true
        rbbi.setText((CharacterIterator)null);
        if (rbbi.preceding(-1) != BreakIterator.DONE) {
            errln("RuleBasedBreakIterator.preceding(-1) was suppose to return "
                    + "0 when the object has a fText of null.");
        }

        // Tests when "else if (offset < fText.getBeginIndex())" is true
        rbbi.setText("dummy");
        if (rbbi.preceding(-1) != 0) {
            errln("RuleBasedBreakIterator.preceding(-1) was suppose to return "
                    + "0 when the object has a fText of dummy.");
        }
    }

    /* Tests the method public int current() */
    @Test
    public void TestCurrent(){
        RuleBasedBreakIterator rbbi = new RuleBasedBreakIterator(".;");
        // Tests when "(fText != null) ? fText.getIndex() : BreakIterator.DONE" is true and false
        rbbi.setText((CharacterIterator)null);
        if(rbbi.current() != BreakIterator.DONE){
            errln("RuleBasedBreakIterator.current() was suppose to return "
                    + "BreakIterator.DONE when the object has a fText of null.");
        }
        rbbi.setText("dummy");
        if(rbbi.current() != 0){
            errln("RuleBasedBreakIterator.current() was suppose to return "
                    + "0 when the object has a fText of dummy.");
        }
    }

    @Test
    public void TestBug7547() {
        try {
            new RuleBasedBreakIterator("");
            fail("TestBug7547: RuleBasedBreakIterator constructor failed to throw an exception with empty rules.");
        }
        catch (IllegalArgumentException e) {
            // expected exception with empty rules.
        }
        catch (Exception e) {
            fail("TestBug7547: Unexpected exception while creating RuleBasedBreakIterator: " + e);
        }
    }

    @Test
    public void TestBug12797() {
        String rules = "!!chain; !!forward; $v=b c; a b; $v; !!reverse; .*;";
        RuleBasedBreakIterator bi = new RuleBasedBreakIterator(rules);

        bi.setText("abc");
        bi.first();
        assertEquals("Rule chaining test", 3,  bi.next());
    }


    @Test
    public void TestBug12873() {
        // Bug with RuleBasedBreakIterator's internal structure for recording potential look-ahead
        // matches not being cloned when a break iterator is cloned. This resulted in usage
        // collisions if the original break iterator and its clone were used concurrently.

        // The Line Break rules for Regional Indicators make use of look-ahead rules, and
        // show the bug. 1F1E6 = \uD83C\uDDE6 = REGIONAL INDICATOR SYMBOL LETTER A
        // Regional indicators group into pairs, expect breaks after two code points, which
        // is after four 16 bit code units.

        final String dataToBreak = "\uD83C\uDDE6\uD83C\uDDE6\uD83C\uDDE6\uD83C\uDDE6\uD83C\uDDE6\uD83C\uDDE6";
        final RuleBasedBreakIterator bi = (RuleBasedBreakIterator)BreakIterator.getLineInstance();
        final AssertionError[] assertErr = new AssertionError[1];  // saves an error found from within a thread

        class WorkerThread implements Runnable {
            @Override
            public void run() {
                try {
                    RuleBasedBreakIterator localBI = (RuleBasedBreakIterator)bi.clone();
                    localBI.setText(dataToBreak);
                    for (int loop=0; loop<100; loop++) {
                        int nextExpectedBreak = 0;
                        for (int actualBreak = localBI.first(); actualBreak != BreakIterator.DONE;
                                actualBreak = localBI.next(), nextExpectedBreak+= 4) {
                            assertEquals("", nextExpectedBreak, actualBreak);
                        }
                        assertEquals("", dataToBreak.length()+4, nextExpectedBreak);
                    }
                } catch (AssertionError e) {
                    assertErr[0] = e;
                }
            }
        }

        List<Thread> threads = new ArrayList<Thread>();
        for (int n = 0; n<4; ++n) {
            threads.add(new Thread(new WorkerThread()));
        }
        for (Thread thread: threads) {
            thread.start();
        }
        for (Thread thread: threads) {
            try {
                thread.join();
            } catch (InterruptedException e) {
                fail(e.toString());
            }
        }

        // JUnit wont see failures from within the worker threads, so
        // check again if one occurred.
        if (assertErr[0] != null) {
            throw assertErr[0];
        }
    }

    @Test
    public void TestBreakAllChars() {
        // Make a "word" from each code point, separated by spaces.
        // For dictionary based breaking, runs the start-of-range
        // logic with all possible dictionary characters.
        StringBuilder sb = new StringBuilder();
        for (int c=0; c<0x110000; ++c) {
            sb.appendCodePoint(c);
            sb.appendCodePoint(c);
            sb.appendCodePoint(c);
            sb.appendCodePoint(c);
            sb.append(' ');
        }
        String s = sb.toString();

        for (int breakKind=BreakIterator.KIND_CHARACTER; breakKind<=BreakIterator.KIND_TITLE; ++breakKind) {
            RuleBasedBreakIterator bi =
                    (RuleBasedBreakIterator)BreakIterator.getBreakInstance(ULocale.ENGLISH, breakKind);
            bi.setText(s);
            int lastb = -1;
            for (int b = bi.first(); b != BreakIterator.DONE; b = bi.next()) {
                assertTrue("(lastb < b) : (" + lastb + " < " + b + ")", lastb < b);
            }
        }
    }

    @Test
    public void TestBug12918() {
        // This test triggered an assertion failure in ICU4C, in dictbe.cpp
        // The equivalent code in ICU4J is structured slightly differently,
        // and does not appear vulnerable to the same issue.
        //
        // \u3325 decomposes with normalization, then the CJK dictionary
        // finds a break within the decomposition.

        String crasherString = "\u3325\u4a16";
        BreakIterator iter = BreakIterator.getWordInstance(ULocale.ENGLISH);
        iter.setText(crasherString);
        iter.first();
        int pos = 0;
        int lastPos = -1;
        while((pos = iter.next()) != BreakIterator.DONE) {
            assertTrue("", pos > lastPos);
        }
    }

    @Test
    public void TestBug12519() {
        RuleBasedBreakIterator biEn = (RuleBasedBreakIterator)BreakIterator.getWordInstance(ULocale.ENGLISH);
        RuleBasedBreakIterator biFr = (RuleBasedBreakIterator)BreakIterator.getWordInstance(ULocale.FRANCE);
        assertEquals("", ULocale.ENGLISH, biEn.getLocale(ULocale.VALID_LOCALE));
        assertEquals("", ULocale.FRENCH, biFr.getLocale(ULocale.VALID_LOCALE));
        assertEquals("Locales do not participate in BreakIterator equality.", biEn, biFr);

        RuleBasedBreakIterator cloneEn = (RuleBasedBreakIterator)biEn.clone();
        assertEquals("", biEn, cloneEn);
        assertEquals("", ULocale.ENGLISH, cloneEn.getLocale(ULocale.VALID_LOCALE));

        RuleBasedBreakIterator cloneFr = (RuleBasedBreakIterator)biFr.clone();
        assertEquals("", biFr, cloneFr);
        assertEquals("", ULocale.FRENCH, cloneFr.getLocale(ULocale.VALID_LOCALE));
    }

    static class T13512Thread extends Thread {
        private String fText;
        public List fBoundaries;
        public List fExpectedBoundaries;

        T13512Thread(String text) {
            fText = text;
            fExpectedBoundaries = getBoundary(fText);
        }
        @Override
        public void run() {
            for (int i= 0; i<10000; ++i) {
                fBoundaries = getBoundary(fText);
                if (!fBoundaries.equals(fExpectedBoundaries)) {
                    break;
                }
            }
        }
        private static final BreakIterator BREAK_ITERATOR_CACHE = BreakIterator.getWordInstance(ULocale.ROOT);
        public static List<Integer> getBoundary(String toParse) {
            List<Integer> retVal = new ArrayList<Integer>();
            BreakIterator bi = (BreakIterator) BREAK_ITERATOR_CACHE.clone();
            bi.setText(toParse);
            for (int boundary=bi.first(); boundary != BreakIterator.DONE; boundary = bi.next()) {
                retVal.add(boundary);
            }
            return retVal;
        }
    }

    @Test
    public void TestBug13512() {
        String japanese = "コンピューターは、本質的には数字しか扱うことができません。コンピューターは、文字や記号などのそれぞれに番号を割り振る"
                + "ことによって扱えるようにします。ユニコードが出来るまでは、これらの番号を割り振る仕組みが何百種類も存在しました。どの一つをとっても、十分な"
                + "文字を含んではいませんでした。例えば、欧州連合一つを見ても、そのすべての言語をカバーするためには、いくつかの異なる符号化の仕"
                + "組みが必要でした。英語のような一つの言語に限っても、一つだけの符号化の仕組みでは、一般的に使われるすべての文字、句読点、技術"
                + "的な記号などを扱うには不十分でした。";

        String thai = "โดยพื้นฐานแล้ว, คอมพิวเตอร์จะเกี่ยวข้องกับเรื่องของตัวเลข. คอมพิวเตอร์จัดเก็บตัวอักษรและอักขระอื่นๆ"
                + " โดยการกำหนดหมายเลขให้สำหรับแต่ละตัว. ก่อนหน้าที่๊ Unicode จะถูกสร้างขึ้น, ได้มีระบบ encoding "
                + "อยู่หลายร้อยระบบสำหรับการกำหนดหมายเลขเหล่านี้. ไม่มี encoding ใดที่มีจำนวนตัวอักขระมากเพียงพอ: ยกตัวอย่างเช่น, "
                + "เฉพาะในกลุ่มสหภาพยุโรปเพียงแห่งเดียว ก็ต้องการหลาย encoding ในการครอบคลุมทุกภาษาในกลุ่ม. "
                + "หรือแม้แต่ในภาษาเดี่ยว เช่น ภาษาอังกฤษ ก็ไม่มี encoding ใดที่เพียงพอสำหรับทุกตัวอักษร, "
                + "เครื่องหมายวรรคตอน และสัญลักษณ์ทางเทคนิคที่ใช้กันอยู่ทั่วไป.\n" +
                "ระบบ encoding เหล่านี้ยังขัดแย้งซึ่งกันและกัน. นั่นก็คือ, ในสอง encoding สามารถใช้หมายเลขเดียวกันสำหรับตัวอักขระสองตัวที่แตกต่างกัน,"
                + "หรือใช้หมายเลขต่างกันสำหรับอักขระตัวเดียวกัน. ในระบบคอมพิวเตอร์ (โดยเฉพาะเซิร์ฟเวอร์) ต้องมีการสนับสนุนหลาย"
                + " encoding; และเมื่อข้อมูลที่ผ่านไปมาระหว่างการเข้ารหัสหรือแพล็ตฟอร์มที่ต่างกัน, ข้อมูลนั้นจะเสี่ยงต่อการผิดพลาดเสียหาย.";

        T13512Thread t1 = new T13512Thread(thai);
        T13512Thread t2 = new T13512Thread(japanese);
        try {
            t1.start(); t2.start();
            t1.join(); t2.join();
        } catch (Exception e) {
            fail(e.toString());
        }
        assertEquals("", t1.fExpectedBoundaries, t1.fBoundaries);
        assertEquals("", t2.fExpectedBoundaries, t2.fBoundaries);
    }
}
