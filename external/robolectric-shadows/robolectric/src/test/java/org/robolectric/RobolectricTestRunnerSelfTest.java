package org.robolectric;

import static org.assertj.core.api.Assertions.assertThat;

import android.app.Application;
import android.content.res.Resources;
import android.os.Build;
import org.assertj.core.api.Assertions;
import org.hamcrest.CoreMatchers;
import org.junit.AfterClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

@RunWith(RobolectricTestRunner.class)
@Config(application = RobolectricTestRunnerSelfTest.MyTestApplication.class)
public class RobolectricTestRunnerSelfTest {

  @Test
  public void shouldInitializeAndBindApplicationButNotCallOnCreate() {
    assertThat(RuntimeEnvironment.application).as("application")
      .isNotNull()
      .isInstanceOf(MyTestApplication.class);
    assertThat(((MyTestApplication) RuntimeEnvironment.application).onCreateWasCalled).as("onCreate called").isTrue();
    assertThat(RuntimeEnvironment.getAppResourceTable()).as("Application resource loader").isNotNull();
  }

  @Test
  public void shouldSetUpSystemResources() {
    Resources systemResources = Resources.getSystem();
    Resources appResources = RuntimeEnvironment.application.getResources();

    assertThat(systemResources).as("system resources").isNotNull();

    assertThat(systemResources.getString(android.R.string.copy)).as("system resource")
        .isEqualTo(appResources.getString(android.R.string.copy));

    assertThat(appResources.getString(R.string.howdy)).as("app resource")
      .isNotNull();
    try {
      systemResources.getString(R.string.howdy);
      Assertions.failBecauseExceptionWasNotThrown(Resources.NotFoundException.class);
    } catch (Resources.NotFoundException e) {
    }
  }

  @Test
  public void setStaticValue_shouldIgnoreFinalModifier() {
    ReflectionHelpers.setStaticField(Build.class, "MODEL", "expected value");

    assertThat(Build.MODEL).isEqualTo("expected value");
  }

  @Test
  @Config(qualifiers = "fr")
  public void internalBeforeTest_testValuesResQualifiers() {
    assertThat(RuntimeEnvironment.getQualifiers()).contains("fr");
  }

  @Test
  public void testMethod_shouldBeInvoked_onMainThread() {
    assertThat(RuntimeEnvironment.isMainThread()).isTrue();
  }

  @Test(timeout = 1000)
  public void whenTestHarnessUsesDifferentThread_shouldStillReportAsMainThread() {
    assertThat(RuntimeEnvironment.isMainThread()).isTrue();
  }

  @Test
  @Config(sdk = Build.VERSION_CODES.KITKAT)
  public void testVersionConfiguration() {
    assertThat(Build.VERSION.SDK_INT)
        .isEqualTo(Build.VERSION_CODES.KITKAT);
    assertThat(Build.VERSION.RELEASE).isEqualTo("4.4");
  }

  @Test public void hamcrestMatchersDontBlowUpDuringLinking() throws Exception {
    org.junit.Assert.assertThat(true, CoreMatchers.is(true));
  }

  @AfterClass
  public static void resetStaticState_shouldBeCalled_onMainThread() {
    assertThat(onTerminateCalledFromMain).isTrue();
  }

  private static Boolean onTerminateCalledFromMain = null;

  public static class MyTestApplication extends Application {
    private boolean onCreateWasCalled;

    @Override
    public void onCreate() {
      this.onCreateWasCalled = true;
    }
    
    @Override
    public void onTerminate() {
      onTerminateCalledFromMain = Boolean.valueOf(RuntimeEnvironment.isMainThread());
    }
  }
}
