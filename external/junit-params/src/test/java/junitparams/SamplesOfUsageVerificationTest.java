package junitparams;

import static org.junit.Assert.assertEquals;

import org.junit.Test;
import org.junit.runner.JUnitCore;
import org.junit.runner.Result;

import junitparams.usage.SamplesOfUsageTest;

public class SamplesOfUsageVerificationTest {

    @Test
    public void verifyNoTestsIgnoredInSamplesOfUsageTest() {
        Result result = JUnitCore.runClasses(SamplesOfUsageTest.class);

        assertEquals(0, result.getFailureCount());
        // Android-changed: 5 tests are ignored, see the @Ignore annotated methods in
        // SamplesOfUsageTest for more details.
        assertEquals(5, result.getIgnoreCount());
    }

}
