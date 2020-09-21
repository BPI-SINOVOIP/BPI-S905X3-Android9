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
package com.android.tradefed.testtype.host;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.result.InputStreamSource;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.LogDataType;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.ByteString;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TemporaryFolder;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.stubbing.Answer;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

/** Unit tests for {@link CoverageMeasurementForwarder}. */
@RunWith(JUnit4.class)
public final class CoverageMeasurementForwarderTest {

    private static final String ARTIFACT_NAME1 = "fooTest.exec";
    private static final String ARTIFACT_NAME2 = "barTest.exec";
    private static final String NONEXISTANT_ARTIFACT = "missingArtifact.exec";

    private static final ByteString MEASUREMENT1 =
            ByteString.copyFromUtf8("Mi estas kovrado mezurado");
    private static final ByteString MEASUREMENT2 =
            ByteString.copyFromUtf8("Mi estas ankau kovrado mezurado");

    @Mock IBuildInfo mMockBuildInfo;
    @Mock ITestInvocationListener mMockListener;

    @Rule public TemporaryFolder mFolder = new TemporaryFolder();

    /** The {@link CoverageMeasurementForwarder} under test. */
    private CoverageMeasurementForwarder mForwarder;

    private Map<String, ByteString> mCapturedLogs = new HashMap<>();

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        Answer<Void> logCapturingListener =
                invocation -> {
                    Object[] args = invocation.getArguments();
                    try (InputStream stream = ((InputStreamSource) args[2]).createInputStream()) {
                        mCapturedLogs.put((String) args[0], ByteString.readFrom(stream));
                    }
                    return null;
                };
        doAnswer(logCapturingListener)
                .when(mMockListener)
                .testLog(anyString(), eq(LogDataType.COVERAGE), any(InputStreamSource.class));

        doAnswer(invocation -> mockBuildArtifact(MEASUREMENT1))
                .when(mMockBuildInfo)
                .getFile(ARTIFACT_NAME1);
        doAnswer(invocation -> mockBuildArtifact(MEASUREMENT2))
                .when(mMockBuildInfo)
                .getFile(ARTIFACT_NAME2);

        mForwarder = new CoverageMeasurementForwarder();
        mForwarder.setBuild(mMockBuildInfo);
    }

    @Test
    public void testForwardSingleArtifact() {
        mForwarder.setCoverageMeasurements(ImmutableList.of(ARTIFACT_NAME1));
        mForwarder.run(mMockListener);

        assertThat(mCapturedLogs).containsExactly(ARTIFACT_NAME1, MEASUREMENT1);
    }

    @Test
    public void testForwardMultipleArtifacts() {
        mForwarder.setCoverageMeasurements(ImmutableList.of(ARTIFACT_NAME1, ARTIFACT_NAME2));
        mForwarder.run(mMockListener);

        assertThat(mCapturedLogs)
                .containsExactly(ARTIFACT_NAME1, MEASUREMENT1, ARTIFACT_NAME2, MEASUREMENT2);
    }

    @Test
    public void testNoSuchArtifact() {
        mForwarder.setCoverageMeasurements(ImmutableList.of(NONEXISTANT_ARTIFACT));
        try {
            mForwarder.run(mMockListener);
            fail("Should have thrown an exception.");
        } catch (RuntimeException e) {
            // Expected
        }
        assertThat(mCapturedLogs).isEmpty();
    }

    private File mockBuildArtifact(ByteString contents) throws IOException {
        File ret = mFolder.newFile();
        try (OutputStream out = new FileOutputStream(ret)) {
            contents.writeTo(out);
        }
        return ret;
    }
}
