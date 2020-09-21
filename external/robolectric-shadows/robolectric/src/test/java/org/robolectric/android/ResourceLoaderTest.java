package org.robolectric.android;

import static org.assertj.core.api.Assertions.assertThat;

import android.content.res.Configuration;
import android.content.res.Resources;
import android.os.Build;
import android.os.Build.VERSION_CODES;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import java.util.Locale;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.R;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.res.ResName;
import org.robolectric.res.ResourceTable;

@RunWith(RobolectricTestRunner.class)
public class ResourceLoaderTest {

  @Test
  @Config(qualifiers="w0dp")
  public void checkDefaultBooleanValue() throws Exception {
	  assertThat(RuntimeEnvironment.application.getResources().getBoolean(R.bool.different_resource_boolean)).isEqualTo(false);
  }

  @Test
  @Config(qualifiers="w820dp")
  public void checkQualifiedBooleanValue() throws Exception {
	  assertThat(RuntimeEnvironment.application.getResources().getBoolean(R.bool.different_resource_boolean)).isEqualTo(true);
  }
  
  @Test
  public void checkForPollution1() throws Exception {
    checkForPollutionHelper();
  }

  @Test
  public void checkForPollution2() throws Exception {
    checkForPollutionHelper();
  }

  private void checkForPollutionHelper() {
    assertThat(RuntimeEnvironment.getQualifiers())
        .isEqualTo("en-rUS-ldltr-sw320dp-w320dp-h470dp-normal-notlong-notround-port-notnight-mdpi-finger-keyssoft-nokeys-navhidden-nonav-v" + Build.VERSION.RESOURCES_SDK_INT);

    View view = LayoutInflater.from(RuntimeEnvironment.application).inflate(R.layout.different_screen_sizes, null);
    TextView textView = view.findViewById(android.R.id.text1);
    assertThat(textView.getText().toString()).isEqualTo("default");
    RuntimeEnvironment.setQualifiers("fr-land"); // testing if this pollutes the other test
    Configuration configuration = Resources.getSystem().getConfiguration();
    if (RuntimeEnvironment.getApiLevel() <= VERSION_CODES.JELLY_BEAN) {
      configuration.locale = new Locale("fr", "FR");
    } else {
      configuration.setLocale(new Locale("fr", "FR"));
    }
    configuration.orientation = Configuration.ORIENTATION_LANDSCAPE;
    Resources.getSystem().updateConfiguration(configuration, null);
  }

  @Test
  public void shouldMakeInternalResourcesAvailable() throws Exception {
    ResourceTable resourceProvider = RuntimeEnvironment.getSystemResourceTable();
    ResName internalResource = new ResName("android", "string", "badPin");
    Integer resId = resourceProvider.getResourceId(internalResource);
    assertThat(resId).isNotNull();
    assertThat(resourceProvider.getResName(resId)).isEqualTo(internalResource);

    Class<?> internalRIdClass = Robolectric.class.getClassLoader().loadClass("com.android.internal.R$" + internalResource.type);
    int internalResourceId;
    internalResourceId = (Integer) internalRIdClass.getDeclaredField(internalResource.name).get(null);
    assertThat(resId).isEqualTo(internalResourceId);

    assertThat(RuntimeEnvironment.application.getResources().getString(resId)).isEqualTo("The old PIN you typed isn't correct.");
  }
}
