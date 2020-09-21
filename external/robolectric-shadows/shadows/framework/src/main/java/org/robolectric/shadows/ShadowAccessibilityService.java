package org.robolectric.shadows;

import android.accessibilityservice.AccessibilityService;
import java.util.ArrayList;
import java.util.List;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

/**
 * Shadow of AccessibilityService that saves global actions to a list.
 */
@Implements(AccessibilityService.class)
public class ShadowAccessibilityService extends ShadowService {

  private final List<Integer> globalActionsPerformed = new ArrayList<>();

    @Implementation
    public final boolean performGlobalAction(int action) {
      globalActionsPerformed.add(action);
      return true;
    }

    @Implementation
    public List<Integer> getGlobalActionsPerformed() {
      return globalActionsPerformed;
    }
}
