package org.robolectric;

import com.example.objects.AnyObject;
import com.example.objects.Dummy;
import java.util.HashMap;
import java.util.Map;
import javax.annotation.Generated;
import org.robolectric.annotation.processing.shadows.ShadowAnything;
import org.robolectric.annotation.processing.shadows.ShadowDummy;
import org.robolectric.internal.ShadowProvider;
import org.robolectric.shadow.api.Shadow;

@Generated("org.robolectric.annotation.processing.RobolectricProcessor")
@SuppressWarnings({"unchecked","deprecation"})
public class Shadows implements ShadowProvider {
  private static final Map<String, String> SHADOW_MAP = new HashMap<>(2);

  static {
    SHADOW_MAP.put("com.example.objects.AnyObject", "org.robolectric.annotation.processing.shadows.ShadowAnything");
    SHADOW_MAP.put("com.example.objects.Dummy", "org.robolectric.annotation.processing.shadows.ShadowDummy");
  }

  public static ShadowAnything shadowOf(AnyObject actual) {
    return (ShadowAnything) Shadow.extract(actual);
  }
  
  public static ShadowDummy shadowOf(Dummy actual) {
    return (ShadowDummy) Shadow.extract(actual);
  }

  @Override
  public void reset() {
    ShadowAnything.anotherResetter();
    ShadowDummy.resetter_method();
  }

  @Override
  public Map<String, String> getShadowMap() {
    return SHADOW_MAP;
  }

  @Override
  public String[] getProvidedPackageNames() {
    return new String[] {};
  }
}
