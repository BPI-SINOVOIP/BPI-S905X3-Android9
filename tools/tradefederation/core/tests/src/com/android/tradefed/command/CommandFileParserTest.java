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

package com.android.tradefed.command;

import com.android.tradefed.command.CommandFileParser.CommandLine;
import com.android.tradefed.config.ConfigurationException;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Unit tests for {@link CommandFileParser}
 */
public class CommandFileParserTest extends TestCase {
    /**
     * Uses a <File, String> map to provide {@link CommandFileParser} with mock data
     * for the mapped Files.
     */
    private static class MockCommandFileParser extends CommandFileParser {
        private final Map<File, String> mDataMap;

        MockCommandFileParser(Map<File, String> dataMap) {
            this.mDataMap = dataMap;
        }

        @Override
        BufferedReader createCommandFileReader(File file) {
            return new BufferedReader(new StringReader(mDataMap.get(file)));
        }
    }

    /** the {@link CommandFileParser} under test, with all dependencies mocked out */
    private CommandFileParser mCommandFile;
    private String mMockFileData = "";
    private static final File MOCK_FILE_PATH = new File("/path/to/");
    private File mMockFile = new File(MOCK_FILE_PATH, "original.txt");

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mCommandFile = new CommandFileParser() {
            @Override
            BufferedReader createCommandFileReader(File file) {
               return new BufferedReader(new StringReader(mMockFileData));
            }
        };
    }

    /**
     * Test parsing a command file with a comment and a single config.
     */
    public void testParse_singleConfig() throws Exception {
        // inject mock file data
        mMockFileData = "  #Comment followed by blank line\n \n--foo  config";
        List<String> expectedArgs = Arrays.asList("--foo", "config");

        assertParsedData(expectedArgs);
    }

    @SafeVarargs
    private final void assertParsedData(List<String>... expectedCommands) throws IOException,
            ConfigurationException {
        assertParsedData(mCommandFile, mMockFile, expectedCommands);
    }

    @SafeVarargs
    private final void assertParsedData(CommandFileParser parser, List<String>... expectedCommands)
            throws IOException, ConfigurationException {
        assertParsedData(parser, mMockFile, expectedCommands);
    }

    @SafeVarargs
    private final void assertParsedData(CommandFileParser parser, File file,
            List<String>... expectedCommands) throws IOException, ConfigurationException {
        List<CommandLine> data = parser.parseFile(file);
        assertEquals(expectedCommands.length, data.size());

        for (int i = 0; i < expectedCommands.length; i++) {
            assertEquals(expectedCommands[i].size(), data.get(i).size());

            for(int j = 0; j < expectedCommands[i].size(); j++) {
                assertEquals(expectedCommands[i].get(j), data.get(i).get(j));
            }
        }
    }

    /**
     * Make sure that a config with a quoted argument is parsed properly.
     * <p/>
     * Whitespace inside of the quoted section should be preserved. Also, embedded escaped quotation
     * marks should be ignored.
     */
    public void testParseFile_quotedConfig() throws IOException, ConfigurationException  {
        // inject mock file data
        mMockFileData = "--foo \"this is a config\" --bar \"escap\\\\ed \\\" quotation\"";
        List<String> expectedArgs = Arrays.asList(
                "--foo", "this is a config", "--bar", "escap\\\\ed \\\" quotation"
        );

        assertParsedData(expectedArgs);
    }

    /**
     * Test the data where the configuration ends inside a quotation.
     */
    public void testParseFile_endOnQuote() throws IOException {
        // inject mock file data
        mMockFileData = "--foo \"this is truncated";

        try {
            mCommandFile.parseFile(mMockFile);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test the scenario where the configuration ends inside a quotation.
     */
    public void testRun_endWithEscape() throws IOException {
        // inject mock file data
        mMockFileData = "--foo escape\\";
        try {
            mCommandFile.parseFile(mMockFile);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    // Macro-related tests
    public void testSimpleMacro() throws IOException, ConfigurationException {
        mMockFileData = "MACRO TeSt = verify\nTeSt()";
        List<String> expectedArgs = Arrays.asList("verify");

        assertParsedData(expectedArgs);
    }

    /**
     * Ensure that when a macro is overwritten, the most recent value should be used.
     * <p />
     * Note that a message should also be logged; no good way to verify that currently
     */
    public void testOverwriteMacro() throws IOException, ConfigurationException {
        mMockFileData = "MACRO test = value 1\nMACRO test = value 2\ntest()";
        List<String> expectedArgs = Arrays.asList("value", "2");

        assertParsedData(expectedArgs);
    }

    /**
     * Ensure that parsing of quoted tokens continues to work
     */
    public void testSimpleMacro_quotedTokens() throws IOException, ConfigurationException {
        mMockFileData = "MACRO test = \"verify varify vorify\"\ntest()";
        List<String> expectedArgs = Arrays.asList("verify varify vorify");

        assertParsedData(expectedArgs);
    }

    /**
     * Ensure that parsing of names with embedded underscores works properly.
     */
    public void testSimpleMacro_underscoreName() throws IOException, ConfigurationException {
        mMockFileData = "MACRO under_score = verify\nunder_score()";
        List<String> expectedArgs = Arrays.asList("verify");

        assertParsedData(expectedArgs);
    }

    /**
     * Ensure that parsing of names with embedded hyphens works properly.
     */
    public void testSimpleMacro_hyphenName() throws IOException, ConfigurationException {
        mMockFileData = "MACRO hyphen-nated = verify\nhyphen-nated()";
        List<String> expectedArgs = Arrays.asList("verify");

        assertParsedData(expectedArgs);
    }

    /**
     * Test the scenario where a macro call doesn't resolve.
     */
    public void testUndefinedMacro() throws IOException {
        mMockFileData = "test()";
        try {
            mCommandFile.parseFile(mMockFile);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Test the scenario where a syntax problem causes a macro call to not resolve.
     */
    public void testUndefinedMacro_defSyntaxError() throws IOException {
        mMockFileData = "MACRO test = \n" +
                "test()";
        try {
            mCommandFile.parseFile(mMockFile);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Simple test for LONG MACRO parsing
     */
    public void testSimpleLongMacro() throws IOException, ConfigurationException {
        mMockFileData = "LONG MACRO test\n" +
                "verify\n" +
                "END MACRO\n" +
                "test()";
        List<String> expectedArgs = Arrays.asList("verify");

        assertParsedData(expectedArgs);
    }

    /**
     * Ensure that when a long macro is overwritten, the most recent value should be used.
     * <p />
     * Note that a message should also be logged; no good way to verify that currently
     */


    /**
     * Simple test for LONG MACRO parsing with multi-line expansion
     */
    public void testSimpleLongMacro_multiline() throws IOException, ConfigurationException {
        mMockFileData = "LONG MACRO test\n" +
                "one two three\n" +
                "a b c\n" +
                "do re mi\n" +
                "END MACRO\n" +
                "test()";
        List<String>  expectedArgs1 = Arrays.asList("one", "two", "three");
        List<String>  expectedArgs2 = Arrays.asList("a", "b", "c");
        List<String>  expectedArgs3 = Arrays.asList("do", "re", "mi");
        assertParsedData(expectedArgs1, expectedArgs2, expectedArgs3);

    }

    /**
     * Simple test for LONG MACRO parsing with multi-line expansion
     */
    public void testLongMacro_withComment() throws IOException, ConfigurationException {
        mMockFileData = "LONG MACRO test\n" +
                "\n" +  // blank line
                "one two three\n" +
                "#a b c\n" +
                "do re mi\n" +
                "END MACRO\n" +
                "test()";
        List<String>  expectedArgs1 = Arrays.asList("one", "two", "three");
        List<String>  expectedArgs2 = Arrays.asList("do", "re", "mi");
        assertParsedData(expectedArgs1, expectedArgs2);
    }

    /**
     * Test the scenario where the configuration ends inside of a LONG MACRO definition.
     */
    public void testLongMacroSyntaxError_eof() throws IOException {
        mMockFileData = "LONG MACRO test\n" +
                "verify\n" +
                // "END MACRO\n" (this is the syntax error)
                "test()";

        try {
            mCommandFile.parseFile(mMockFile);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /**
     * Verifies that the line location data in CommandLine (file, line number) is accurate.
     */
    public void testCommandLineLocation() throws IOException, ConfigurationException {
        mMockFileData = "# This is a comment\n" +
                "# This is another comment\n" +
                "this --is-a-cmd\n" +
                "one --final-cmd\n" +
                "# More comments\n" +
                "two --final-command";

        Set<CommandLine> expectedSet = ImmutableSet.<CommandLine>builder()
                .add(new CommandLine(Arrays.asList("this", "--is-a-cmd"), mMockFile, 3))
                .add(new CommandLine(Arrays.asList("one", "--final-cmd"), mMockFile, 4))
                .add(new CommandLine(Arrays.asList("two", "--final-command"), mMockFile, 6))
                .build();


        Set<CommandLine> parsedSet = ImmutableSet.<CommandLine>builder()
                .addAll(mCommandFile.parseFile(mMockFile))
                .build();

        assertEquals(expectedSet, parsedSet);
    }

    /**
     * Verifies that the line location data in CommandLine (file, line number) is accurate for
     * commands defined in included files.
     */
    public void testCommandLineLocation_withInclude() throws IOException, ConfigurationException {
        final File mockFile = new File(MOCK_FILE_PATH, "file.txt");
        final String mockFileData = "# This is a comment\n" +
                "# This is another comment\n" +
                "INCLUDE include.txt\n" +
                "this --is-a-cmd\n" +
                "one --final-cmd";

        final File mockIncludedFile = new File(MOCK_FILE_PATH, "include.txt");
        final String mockIncludedFileData = "# This is a comment\n" +
                "# This is another comment\n" +
                "inc --is-a-cmd\n" +
                "# More comments\n" +
                "inc --final-cmd";

        Set<CommandLine> expectedSet = ImmutableSet.<CommandLine>builder()
                .add(new CommandLine(Arrays.asList("this", "--is-a-cmd"), mockFile, 4))
                .add(new CommandLine(Arrays.asList("one", "--final-cmd"), mockFile, 5))
                .add(new CommandLine(Arrays.asList("inc", "--is-a-cmd"), mockIncludedFile, 3))
                .add(new CommandLine(Arrays.asList("inc", "--final-cmd"), mockIncludedFile, 5))
                .build();


        CommandFileParser commandFileParser = new MockCommandFileParser(
                ImmutableMap.<File, String>builder()
                .put(mockFile, mockFileData)
                .put(mockIncludedFile, mockIncludedFileData)
                .build());

        Set<CommandLine> parsedSet = ImmutableSet.<CommandLine>builder()
                .addAll(commandFileParser.parseFile(mockFile))
                .build();

        assertEquals(expectedSet, parsedSet);
    }

    /**
     * Verify that the INCLUDE directive is behaving properly
     */
    public void testMacroParserInclude() throws Exception {
        final String mockFileData = "INCLUDE somefile.txt\n";
        final String mockIncludedFileData = "--foo bar\n";
        List<String>  expectedArgs = Arrays.asList("--foo", "bar");

        CommandFileParser commandFile = new CommandFileParser() {
            private boolean showInclude = true;
            @Override
            BufferedReader createCommandFileReader(File file) {
                if (showInclude) {
                    showInclude = false;
                    return new BufferedReader(new StringReader(mockFileData));
                } else {
                    return new BufferedReader(new StringReader(mockIncludedFileData));
                }
            }
        };

        assertParsedData(commandFile, expectedArgs);
        assertEquals(1, commandFile.getIncludedFiles().size());
        assertTrue(commandFile.getIncludedFiles().iterator().next().endsWith("somefile.txt"));
    }

    /**
     * Verify that the INCLUDE directive works when used for two files
     */
    public void testMacroParserInclude_twice() throws Exception {
        final String mockFileData = "INCLUDE somefile.txt\n" +
                "INCLUDE otherfile.txt\n";
        final String mockIncludedFileData1 = "--foo bar\n";
        final String mockIncludedFileData2 = "--baz quux\n";
        List<String>  expectedArgs1 = Arrays.asList("--foo", "bar");
        List<String>  expectedArgs2 = Arrays.asList("--baz", "quux");

        CommandFileParser commandFile = new CommandFileParser() {
            private int phase = 0;
            @Override
            BufferedReader createCommandFileReader(File file) {
                if(phase == 0) {
                    phase++;
                    return new BufferedReader(new StringReader(mockFileData));
                } else if (phase == 1) {
                    phase++;
                    return new BufferedReader(new StringReader(mockIncludedFileData1));
                } else {
                    return new BufferedReader(new StringReader(mockIncludedFileData2));
                }
            }
        };
        assertParsedData(commandFile, expectedArgs1, expectedArgs2);
    }

    /**
     * Verify that a file is only ever included once, regardless of how many INCLUDE directives for
     * that file show up
     */
    public void testMacroParserInclude_repeat() throws Exception {
        final String mockFileData = "INCLUDE somefile.txt\n" +
                "INCLUDE somefile.txt\n";
        final String mockIncludedFileData1 = "--foo bar\n";
        List<String>  expectedArgs1 = Arrays.asList("--foo", "bar");

        CommandFileParser commandFile = new CommandFileParser() {
            private int phase = 0;
            @Override
            BufferedReader createCommandFileReader(File file) {
                if(phase == 0) {
                    phase++;
                    return new BufferedReader(new StringReader(mockFileData));
                } else {
                    return new BufferedReader(new StringReader(mockIncludedFileData1));
                }
            }
        };
        assertParsedData(commandFile, expectedArgs1);
    }

    /**
     * Verify that the path of the file referenced by an INCLUDE directive is considered relative to
     * the location of the referencing file.
     */
    public void testMacroParserInclude_parentDir() throws Exception {
        // When we pass an unqualified filename, expect it to be taken relative to mMockFile's
        // parent directory
        final String includeFileName = "somefile.txt";
        final File expectedFile = new File(MOCK_FILE_PATH, includeFileName);

        final String mockFileData = String.format("INCLUDE %s\n", includeFileName);
        final String mockIncludedFileData = "--foo bar\n";
        List<String>  expectedArgs = Arrays.asList("--foo", "bar");

        CommandFileParser commandFile = new CommandFileParser() {
            @Override
            BufferedReader createCommandFileReader(File file) {
                if (mMockFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockFileData));
                } else if (expectedFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockIncludedFileData));
                } else {
                    fail(String.format("Received unexpected request for contents of file %s",
                            file));
                    // shouldn't actually reach here
                    throw new RuntimeException();
                }
            }
        };
        assertParsedData(commandFile, expectedArgs);
    }

    /**
     * Verify that INCLUDEing an absolute path works, even if a parent directory is specified.
     * <p />
     * This verifies the fix for a bug that existed because {@code File("/tmp", "/usr/bin")} creates
     * the path {@code /tmp/usr/bin}, rather than {@code /usr/bin} (which might be expected since
     * the child is actually an absolute path on its own.
     */
    public void testMacroParserInclude_absoluteInclude() throws Exception {
        // When we pass an unqualified filename, expect it to be taken relative to mMockFile's
        // parent directory
        final String includeFileName = "/usr/bin/somefile.txt";
        final File expectedFile = new File(includeFileName);

        final String mockFileData = String.format("INCLUDE %s\n", includeFileName);
        final String mockIncludedFileData = "--foo bar\n";
        List<String>  expectedArgs = Arrays.asList("--foo", "bar");

        CommandFileParser commandFile = new CommandFileParser() {
            @Override
            BufferedReader createCommandFileReader(File file) {
                if (mMockFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockFileData));
                } else if (expectedFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockIncludedFileData));
                } else {
                    fail(String.format("Received unexpected request for contents of file %s",
                            file));
                    // shouldn't actually reach here
                    throw new RuntimeException();
                }
            }
        };
        assertParsedData(commandFile, expectedArgs);
    }

    /**
     * Verify that if the original file is relative to no directory (aka ./), that the referenced
     * file is also relative to no directory.
     */
    public void testMacroParserInclude_noParentDir() throws Exception {
        final File mockFile = new File("original.txt");
        final String includeFileName = "somefile.txt";
        final File expectedFile = new File(includeFileName);

        final String mockFileData = String.format("INCLUDE %s\n", includeFileName);
        final String mockIncludedFileData = "--foo bar\n";
        List<String>  expectedArgs = Arrays.asList("--foo", "bar");

        CommandFileParser commandFile = new CommandFileParser() {
            @Override
            BufferedReader createCommandFileReader(File file) {
                if (mockFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockFileData));
                } else if (expectedFile.equals(file)) {
                    return new BufferedReader(new StringReader(mockIncludedFileData));
                } else {
                    fail(String.format("Received unexpected request for contents of file %s",
                            file));
                    // shouldn't actually reach here
                    throw new RuntimeException();
                }
            }
        };
        assertParsedData(commandFile, mockFile, expectedArgs);
        assertEquals(1, commandFile.getIncludedFiles().size());
        assertTrue(commandFile.getIncludedFiles().iterator().next().endsWith(includeFileName));
    }

    /**
     * A testcase to make sure that the internal bitmask and mLines stay in sync
     * <p>
     * This tickles a bug in the current implementation (before I fix it).  The problem is here,
     * where the inputBitmask is only set to false conditionally, but inputBitmaskCount is
     * decremented unconditionally:
     * <code>inputBitmask.set(inputIdx, sawMacro);
     * --inputBitmaskCount;</code>
     */
    public void testMacroParserSync_suffix() throws IOException, ConfigurationException {
        mMockFileData = "MACRO alpha = one beta()\n" +
                "MACRO beta = two\n" +
                "alpha()\n";
        List<String>  expectedArgs = Arrays.asList("one", "two");
        // When the bug manifests, the result is {"one", "alpha()"}

        assertParsedData(expectedArgs);
    }

    /**
     * A testcase to make sure that the internal bitmask and mLines stay in sync
     * <p>
     * This tests a case related to the _suffix test above.
     */
    public void testMacroParserSync_prefix() throws IOException, ConfigurationException {
        mMockFileData = "MACRO alpha = beta() two\n" +
                "MACRO beta = one\n" +
                "alpha()\n";
        List<String>  expectedArgs = Arrays.asList("one", "two");

        assertParsedData(expectedArgs);
    }

    /**
     * Another test to verify a bugfix.  Long Macro expansion can cause the inputBitmask and its
     * cached form, inputBitmaskCount, to get out of sync.
     * <p />
     * The bug is that the cached value isn't incremented when a long macro is expanded (which means
     * that it may not account for the extra lines that it needs to process).  This manifests as a
     * partially-completed long macro expansion.
     * <p />
     * In this test, when the bug manifests, the first Command to be added will be
     * {@code ["one", "hbar()", "z", "x"} instead of the correct {@code ["one", "quux", "z", "x"]}.
     */
    public void testLongMacroSync() throws IOException, ConfigurationException {
        mMockFileData =
                "MACRO hbar = quux\n" +
                "LONG MACRO bar\n" +
                "hbar() z\n" +
                "END MACRO\n" +
                "LONG MACRO foo\n" +
                "bar() x\n" +
                "END MACRO\n" +
                "LONG MACRO test\n" +
                "one foo()\n" +
                "END MACRO\n" +
                "test()\n" +
                "hbar()\n";

        List<String> expectedArgs1 = Arrays.asList("one", "quux", "z", "x");
        List<String> expectedArgs2 = Arrays.asList("quux");

        assertParsedData(expectedArgs1, expectedArgs2);
    }
}
