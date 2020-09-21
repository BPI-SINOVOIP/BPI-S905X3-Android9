/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tradefed.testtype;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;

import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;

import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/** Unit tests for {@link JacocoCodeCoverageTest}. */
public class JacocoCodeCoverageTestTest {

    private static final ImmutableCollection<File> executionFiles = ImmutableList
            .of(new File("hello.exec"));

    private IBuildInfo mockBuildInfo() {
        IBuildInfo mockBuildInfo = mock(IBuildInfo.class);

        when(mockBuildInfo.getFile("jacocoant.jar")).thenReturn(new File("jacocoant.jar"));

        when(mockBuildInfo.getFiles())
                .thenReturn(Arrays.asList(
                new VersionedFile(new File("hello-allclasses.jar"), "1.0"),
                new VersionedFile(new File("hi-allclasses.jar"), "1.0")));

        return mockBuildInfo;
    }

    @Test
    public void testGetReportFormat_noOptionSpecified() {
        JacocoCodeCoverageTest jacocoCodeCoverageTest = new JacocoCodeCoverageTest();

        List<JacocoCodeCoverageReportFormat> reportFormats = jacocoCodeCoverageTest
                .getReportFormat();

        assertEquals(ImmutableList.of(JacocoCodeCoverageReportFormat.HTML), reportFormats);
    }

    @Test
    public void testGetReportFormat_optionsSpecified() {
        JacocoCodeCoverageTest jacocoCodeCoverageTest = new JacocoCodeCoverageTest();

        jacocoCodeCoverageTest.setReportFormat(Arrays.asList(JacocoCodeCoverageReportFormat.HTML,
                JacocoCodeCoverageReportFormat.CSV, JacocoCodeCoverageReportFormat.XML));

        List<JacocoCodeCoverageReportFormat> reportFormats = jacocoCodeCoverageTest
                .getReportFormat();

        assertEquals(
                ImmutableList.of(JacocoCodeCoverageReportFormat.HTML,
                        JacocoCodeCoverageReportFormat.CSV, JacocoCodeCoverageReportFormat.XML),
                reportFormats);
    }

    @Test
    public void testGenerateCoverageReport_success() throws IOException {
        IBuildInfo mockBuildInfo = mockBuildInfo();

        File tempBuildXml = FileUtil.createTempFile("build", ".xml");

        JacocoCodeCoverageTest jacocoCodeCoverageTest = spy(new JacocoCodeCoverageTest());
        jacocoCodeCoverageTest.setClassFilesFilters(Arrays.asList(".*hello-allclasses.*\\.jar"));
        jacocoCodeCoverageTest.setReportFormat(Arrays.asList(JacocoCodeCoverageReportFormat.HTML));
        jacocoCodeCoverageTest.setJacocoAnt("jacocoant.jar");

        CommandResult result = new CommandResult(CommandStatus.SUCCESS);

        doReturn(result)
                .when(jacocoCodeCoverageTest)
                .runTimedCmd(anyLong(), any(String[].class));

        doReturn(mockBuildInfo).when(jacocoCodeCoverageTest).getBuild();

        doReturn(tempBuildXml).when(jacocoCodeCoverageTest).getBuildXml(eq(executionFiles),
                eq(Arrays.asList(new File("hello-allclasses.jar"))), any(File.class),
                eq(new File("jacocoant.jar")), eq(JacocoCodeCoverageReportFormat.HTML));

        File reportDir = null;
        long reportTimeout = jacocoCodeCoverageTest.getReportTimeout();
        try {
            reportDir = jacocoCodeCoverageTest.generateCoverageReport(executionFiles,
                    JacocoCodeCoverageReportFormat.HTML);
            verify(jacocoCodeCoverageTest, times(1)).runTimedCmd(reportTimeout, new String[] {
                    "ant", "-verbose", "-f", tempBuildXml.getAbsolutePath()
            });
            assertNotNull(reportDir);
        } finally {
            FileUtil.recursiveDelete(reportDir);
            FileUtil.deleteFile(tempBuildXml);
        }
    }

    @Test
    public void testGenerateCoverageReport_missingExecFiles() throws IOException {
        JacocoCodeCoverageTest jacocoCodeCoverageTest = spy(new JacocoCodeCoverageTest());

        String expectedStderr = "Could not find any exec file";

        try {
            jacocoCodeCoverageTest.generateCoverageReport(new ArrayList<>(),
                    JacocoCodeCoverageReportFormat.HTML);

            fail("Should throw illegal argument exception in case of no exec files");

        } catch (IllegalArgumentException e) {
            // Test passes if generateCoverageReport throws IllegalArgumentException
            assertTrue(e.getMessage().contains(expectedStderr));
        }
    }

    @Test
    public void testGenerateCoverageReport_missingClassFiles() {
        IBuildInfo mockBuildInfo = mockBuildInfo();

        JacocoCodeCoverageTest jacocoCodeCoverageTest = spy(new JacocoCodeCoverageTest());
        jacocoCodeCoverageTest.setJacocoAnt("jacocoant.jar");
        jacocoCodeCoverageTest.setClassFilesFilters(Arrays.asList(".*new-allclasses.*\\.jar"));
        jacocoCodeCoverageTest.setReportFormat(Arrays.asList(JacocoCodeCoverageReportFormat.HTML));

        doReturn(mockBuildInfo).when(jacocoCodeCoverageTest).getBuild();

        String expectedStderr = "No class files found";

        try {

            jacocoCodeCoverageTest.generateCoverageReport(executionFiles,
                    JacocoCodeCoverageReportFormat.HTML);

            fail("Should throw io exception in case class files are not found");

        } catch (IOException e) {
            // Test passes if generateCoverageReport throws IOException
            assertTrue(e.getMessage().contains(expectedStderr));
        }
    }

    @Test
    public void testGenerateCoverageReport_unsuccessful() {

        IBuildInfo mockBuildInfo = mockBuildInfo();

        JacocoCodeCoverageTest jacocoCodeCoverageTest = spy(JacocoCodeCoverageTest.class);

        jacocoCodeCoverageTest.setClassFilesFilters(Arrays.asList(".*hello-allclasses.*\\.jar"));
        jacocoCodeCoverageTest.setJacocoAnt("jacocoant.jar");

        String stderr = "something went wrong";

        CommandResult fakeCommandResult = new CommandResult(CommandStatus.FAILED);
        fakeCommandResult.setStderr(stderr);

        doReturn(fakeCommandResult).when(jacocoCodeCoverageTest).runTimedCmd(anyLong(),
                any(String[].class));

        doReturn(mockBuildInfo).when(jacocoCodeCoverageTest).getBuild();

        try {
            jacocoCodeCoverageTest.generateCoverageReport(executionFiles,
                    JacocoCodeCoverageReportFormat.HTML);
            fail("Should throw io exception in case ant command fails");
        } catch (IOException e) {
            // Test passes if generateCoverageReport throws IOException
            assertTrue(e.getMessage().contains(stderr));
        }
    }

    @Test
    public void testGetFileTags() {
        String actualExecFileset = new JacocoCodeCoverageTest().getFileTags(executionFiles);

        String expectedExecFilesetString = "<file file=\""
                + new File("hello.exec").getAbsolutePath() + "\" />\n";

        assertEquals(expectedExecFilesetString, actualExecFileset);
    }

    @Test
    public void testGetBuildXml() throws IOException {
        List<File> allclassesJars = Arrays.asList(new File("hello-allclasses.jar"),
                new File("hi-allclasses.jar"));

        JacocoCodeCoverageTest jacocoCodeCoverageTest = new JacocoCodeCoverageTest();
        jacocoCodeCoverageTest.setReportFormat(Arrays.asList(JacocoCodeCoverageReportFormat.CSV));

        File buildXml = jacocoCodeCoverageTest.getBuildXml(executionFiles, allclassesJars,
                new File("report.xml"), new File("jacocoant.jar"),
                JacocoCodeCoverageReportFormat.CSV);
        String actualContent = FileUtil.readStringFromFile(buildXml);
        StringBuilder expectedContent = new StringBuilder();
        expectedContent.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        expectedContent.append("<project xmlns:jacoco=\"antlib:org.jacoco.ant\"\n");
        expectedContent.append(
                "        name=\"Report generation using jacocoant\" default=\"generatereport\">\n");
        expectedContent.append("\n");
        expectedContent.append("    <taskdef uri=\"antlib:org.jacoco.ant\" ");
        expectedContent.append("resource=\"org/jacoco/ant/antlib.xml\">\n");
        expectedContent.append("        <classpath>\n");
        expectedContent.append("            <pathelement path=\"");
        expectedContent.append(new File("jacocoant.jar").getAbsolutePath());
        expectedContent.append("\" />\n");
        expectedContent.append("        </classpath>\n");
        expectedContent.append("    </taskdef>\n");
        expectedContent.append("\n");
        expectedContent.append("    <target name=\"generatereport\">\n");
        expectedContent.append("\n");
        expectedContent.append("        <jacoco:report>\n");
        expectedContent.append("\n");
        expectedContent.append("            <executiondata>\n");
        expectedContent.append("                <file file=\"");
        expectedContent.append(new File("hello.exec").getAbsolutePath());
        expectedContent.append("\" />\n");
        expectedContent.append("\n");
        expectedContent.append("            </executiondata>\n");
        expectedContent.append("\n");
        expectedContent.append("            <structure name=\"Jacoco ant\">\n");
        expectedContent.append("                <classfiles>\n");
        expectedContent.append("                    <file file=\"");
        expectedContent.append(new File("hello-allclasses.jar").getAbsolutePath());
        expectedContent.append("\" />\n");
        expectedContent.append("<file file=\"");
        expectedContent.append(new File("hi-allclasses.jar").getAbsolutePath());
        expectedContent.append("\" />\n");
        expectedContent.append("\n");
        expectedContent.append("                </classfiles>\n");
        expectedContent.append("            </structure>\n");
        expectedContent.append("\n");
        expectedContent.append("            <csv destfile=\"");
        expectedContent.append(new File("report.xml").getAbsolutePath());
        expectedContent.append("\" />\n");
        expectedContent.append("\n");
        expectedContent.append("        </jacoco:report>\n");
        expectedContent.append("\n");
        expectedContent.append("    </target>\n");
        expectedContent.append("\n");
        expectedContent.append("</project>\n");
        assertEquals(expectedContent.toString(), actualContent);
        FileUtil.deleteFile(buildXml);
    }

    @Test
    public void testIsClassFile_success() {
        JacocoCodeCoverageTest jacocoCodeCoverageTest = new JacocoCodeCoverageTest();
        jacocoCodeCoverageTest.setClassFilesFilters(
                Arrays.asList(".*hello-allclasses.*\\.jar", ".*hi-allclasses.*\\.jar"));
        boolean flag = jacocoCodeCoverageTest.isClassFile(
                new File("foo/hello-allclasses_2456789.jar"));
        assertEquals(true, flag);
    }

    @Test
    public void testIsClassFile_fail() {
        JacocoCodeCoverageTest jacocoCodeCoverageTest = new JacocoCodeCoverageTest();
        jacocoCodeCoverageTest.setClassFilesFilters(
                Arrays.asList(".*hello-allclasses.*\\.jar", ".*hi-allclasses.*\\.jar"));
        boolean flag = jacocoCodeCoverageTest.isClassFile(
                new File("foo/nomatch-allclasses_2456789.jar"));
        assertEquals(false, flag);
    }

}
