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

import static com.android.tradefed.testtype.JacocoCodeCoverageReportFormat.HTML;

import com.android.tradefed.build.VersionedFile;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.StreamUtil;
import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.regex.Pattern;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * An {@link IRemoteTest} that generates a code coverage report by generating build.xml on the fly
 * and executing the ant command. This test type supports collecting coverage information from
 * classes that have already been instrumented by the Jacoco compiler.
 */
@OptionClass(alias = "jacoco-coverage")
public class JacocoCodeCoverageTest extends CodeCoverageTestBase<JacocoCodeCoverageReportFormat> {

    private static final String ANT_BUILD_XML_PATH = "/jacoco/ant-build.xml";

    @Option(name = "report-format",
            description = "Which output format(s) to use when generating the coverage report. "
                    + "May be repeated to output multiple formats. "
                    + "Defaults to HTML if unspecified.")
    private List<JacocoCodeCoverageReportFormat> mReportFormat = new ArrayList<>();

    @Option(name = "report-timeout",
            description = "Maximum time to wait for coverage report generation to complete",
            isTimeVal = true)
    private long mReportTimeout = 10 * 60 * 1000; // 10 Minutes

    @Option(name = "report-classes", mandatory = true, importance = Importance.ALWAYS,
            description = "Regex pattern used to select the instrumented class files "
                    + "that should be used to generate the report. May be repeated.")
    private List<String> mClassFilesFilter = new ArrayList<>();

    @Option(name = "jacocoant", mandatory = true, importance = Importance.ALWAYS,
            description = "The build artifact name for the specific jacoco ant jar "
                    + "that should be used to generate the report.")
    private String mJacocoAnt;

    @Option(name = "ant-path",
            description = "The location of the ant binary. Default use the one in $PATH")
    private String mAntBinary = "ant";

    void setReportFormat(List<JacocoCodeCoverageReportFormat> reportFormat) {
        mReportFormat = reportFormat;
    }

    void setClassFilesFilters(List<String> classFiles) {
        mClassFilesFilter = classFiles;
    }

    List<String> getClassFilesFilters() {
        return mClassFilesFilter;
    }

    void setJacocoAnt(String jacocoAnt) {
        mJacocoAnt = jacocoAnt;
    }

    long getReportTimeout() {
        return mReportTimeout;
    }

    @Override
    protected List<JacocoCodeCoverageReportFormat> getReportFormat() {
        return mReportFormat.isEmpty() ? Arrays.asList(HTML) : mReportFormat;
    }

    @Override
    protected File generateCoverageReport(Collection<File> executionFiles,
            JacocoCodeCoverageReportFormat format) throws IOException {

        if (executionFiles == null || executionFiles.isEmpty()) {
            throw new IllegalArgumentException("Could not find any exec file");
        }

        File jacocoFile = getBuild().getFile(mJacocoAnt);

        if (jacocoFile == null) {
            throw new IOException("Could not find jacocoant jar file " + mJacocoAnt
                    + ". Make sure the file is published as a build artifact, "
                    + "and that your build provider is configured to download it.");
        }

        Collection<VersionedFile> buildFiles = getBuild().getFiles();

        Stream<File> buildFilesStream = buildFiles.stream().map(VersionedFile::getFile);

        List<File> classFiles = buildFilesStream.filter(buildFile -> isClassFile(buildFile))
                .collect(Collectors.toList());

        if (classFiles == null || classFiles.isEmpty()) {
            throw new IOException("No class files found");
        }

        File reportDest = format.equals(HTML) ? FileUtil.createTempDir("report")
                : FileUtil.createTempFile("report", "." + format.getLogDataType().getFileExt());

        File buildXml = getBuildXml(executionFiles, classFiles, reportDest, jacocoFile, format);

        try {
            runAntBuild(buildXml);

            return reportDest;
        } catch (IOException e) {
            FileUtil.recursiveDelete(reportDest);
            throw e;
        } finally {
            FileUtil.deleteFile(buildXml);
        }
    }

    /** Calls {@link RunUtil#runTimedCmd(long, String[])}. Exposed for unit testing. */
    @VisibleForTesting
    CommandResult runTimedCmd(long timeout, String[] command) {
        return RunUtil.getDefault().runTimedCmd(timeout, command);
    }

    /**
     * Get build.xml from template and replace filesets to their corresponding value
     */
    File getBuildXml(Collection<File> executionFiles, List<File> allclassesJars, File reportDest,
            File jacocoFile, JacocoCodeCoverageReportFormat reportFormat) throws IOException {

        InputStream template = getClass().getResourceAsStream(ANT_BUILD_XML_PATH);
        if (template == null) {
            throw new IOException("Could not find " + ANT_BUILD_XML_PATH);
        }

        String text = StreamUtil.getStringFromStream(template);

        File buildXml = FileUtil.createTempFile("build", ".xml");

        text = text.replaceAll(Pattern.quote("${jacocoant}"), jacocoFile.getAbsolutePath());
        text = text.replaceAll(Pattern.quote("${classfileset}"), getFileTags(allclassesJars));
        text = text.replaceAll(Pattern.quote("${execfileset}"), getFileTags(executionFiles));
        text = text.replaceAll(Pattern.quote("${reportdestdirs}"),
                String.format(reportFormat.getAntTagFormat(), reportDest.getAbsolutePath()));

        FileUtil.writeToFile(text, buildXml);

        return buildXml;
    }

    /**
     * Generate file tag with given files
     */
    String getFileTags(Collection<File> files) {
        StringBuilder fileset = new StringBuilder();
        for (File file : files) {
            fileset.append("<file file=\"" + file.getAbsolutePath() + "\" />\n");
        }
        return fileset.toString();
    }

    /**
     * Test if build file matches any of the regex patterns provided to get classfile jars
     */
    @VisibleForTesting
    boolean isClassFile(File buildFile) {
        return getClassFilesFilters().stream()
                .anyMatch(classFileFilter -> Pattern.matches(classFileFilter, buildFile.getPath()));
    }

    /**
     * Run the ant build using the provided option
     */
    private void runAntBuild(File buildXml) throws IOException {
        // Run ant to generate the coverage report
        String[] cmd = {
                mAntBinary, "-verbose", "-f", buildXml.getAbsolutePath()
        };
        CommandResult result = runTimedCmd(mReportTimeout, cmd);

        if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
            CLog.e("Jacoco ant report generation failed [Status=%s]",
                    result.getStatus().toString());
            throw new IOException(result.getStderr());
        }
    }

}
