package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.Assert.assertSame;
import static org.robolectric.Shadows.shadowOf;

import android.view.Gravity;
import android.widget.LinearLayout;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;

@RunWith(RobolectricTestRunner.class)
public class ShadowLinearLayoutTest {
  private LinearLayout linearLayout;
  private ShadowLinearLayout shadow;

  @Before
  public void setup() throws Exception {
    linearLayout = new LinearLayout(RuntimeEnvironment.application);
    shadow = shadowOf(linearLayout);
  }

  @Test
  public void getLayoutParams_shouldReturnTheSameLinearLayoutParamsFromTheSetter() throws Exception {
    LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(1, 2);
    linearLayout.setLayoutParams(params);

    assertSame(params, linearLayout.getLayoutParams());
  }

  @Test
  public void canAnswerOrientation() throws Exception {
    assertThat(linearLayout.getOrientation()).isEqualTo(LinearLayout.HORIZONTAL);
    linearLayout.setOrientation(LinearLayout.VERTICAL);
    assertThat(linearLayout.getOrientation()).isEqualTo(LinearLayout.VERTICAL);
    linearLayout.setOrientation(LinearLayout.HORIZONTAL);
    assertThat(linearLayout.getOrientation()).isEqualTo(LinearLayout.HORIZONTAL);
  }

  @Test
  public void canAnswerGravity() throws Exception {
    assertThat(shadow.getGravity()).isEqualTo(Gravity.TOP | Gravity.START);
    linearLayout.setGravity(Gravity.CENTER_VERTICAL); // Only affects horizontal.
    assertThat(shadow.getGravity()).isEqualTo(Gravity.CENTER_VERTICAL | Gravity.START);
    linearLayout.setGravity(Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL); // Affects both directions.
    assertThat(shadow.getGravity()).isEqualTo(Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
  }
}
