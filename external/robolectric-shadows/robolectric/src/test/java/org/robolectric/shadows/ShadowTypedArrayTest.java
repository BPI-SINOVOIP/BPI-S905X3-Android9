package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;

import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.ColorDrawable;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.R;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.res.AttributeResource;

@RunWith(RobolectricTestRunner.class)
public class ShadowTypedArrayTest {
  private Context context;

  @Before
  public void setUp() throws Exception {
    context = RuntimeEnvironment.application;
  }

  @Test
  public void getResources() throws Exception {
    assertNotNull(context.obtainStyledAttributes(new int[]{}).getResources());
  }

  @Test
  public void getInt_shouldReturnDefaultValue() throws Exception {
    assertThat(context.obtainStyledAttributes(new int[]{android.R.attr.alpha}).getInt(0, -1)).isEqualTo(-1);
  }

  @Test
  public void getInteger_shouldReturnDefaultValue() throws Exception {
    assertThat(context.obtainStyledAttributes(new int[]{android.R.attr.alpha}).getInteger(0, -1)).isEqualTo(-1);
  }

  @Test
  public void getInt_withFlags_shouldReturnValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.gravity, "top|left")
            .build(),
        new int[]{android.R.attr.gravity});
    assertThat(typedArray.getInt(0, -1)).isEqualTo(0x33);
  }

  @Test
  public void getResourceId_shouldReturnDefaultValue() throws Exception {
    assertThat(context.obtainStyledAttributes(new int[]{android.R.attr.alpha}).getResourceId(0, -1)).isEqualTo(-1);
  }

  @Test
  public void getResourceId_shouldReturnActualValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.id, "@+id/snippet_text")
            .build(),
        new int[]{android.R.attr.id});
    assertThat(typedArray.getResourceId(0, -1)).isEqualTo(R.id.snippet_text);
  }

  @Test
  public void getFraction_shouldReturnDefaultValue() throws Exception {
    assertThat(context.obtainStyledAttributes(new int[]{android.R.attr.width}).getDimension(0, -1f)).isEqualTo(-1f);
  }

  @Test
  public void getFraction_shouldReturnGivenValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.width, "50%")
            .build(),
        new int[]{android.R.attr.width});
    assertThat(typedArray.getFraction(0, 100, 1, -1)).isEqualTo(50f);
  }

  @Test
  public void getDimension_shouldReturnDefaultValue() throws Exception {
    assertThat(context.obtainStyledAttributes(new int[]{android.R.attr.width}).getDimension(0, -1f)).isEqualTo(-1f);
  }

  @Test
  public void getDimension_shouldReturnGivenValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.width, "50dp")
            .build(),
        new int[]{android.R.attr.width});
    assertThat(typedArray.getDimension(0, -1)).isEqualTo(50f);
  }

  @Test
  public void getDrawable_withExplicitColorValue_shouldReturnColorDrawable() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.background, "#ff777777")
            .build(),
        new int[]{android.R.attr.background});
    assertThat(typedArray.getDrawable(0)).isEqualTo(new ColorDrawable(0xff777777));
  }

  @Test
  public void getTextArray_whenNoSuchAttribute_shouldReturnNull() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(android.R.attr.keycode, "@array/greetings")
            .build(),
        new int[]{R.attr.animalStyle});
    assertNull(typedArray.getTextArray(0));
  }

  @Test
  public void getTextArray_shouldReturnValues() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(R.attr.responses, "@array/greetings")
            .build(),
        new int[]{R.attr.responses});
    assertThat(typedArray.getTextArray(0)).containsExactly("hola", "Hello");
  }

  @Test public void hasValue_withValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(R.attr.responses, "@array/greetings")
            .build(),
        new int[]{R.attr.responses});
    assertThat(typedArray.hasValue(0)).isTrue();
  }

  @Test public void hasValue_withoutValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        null,
        new int[]{R.attr.animalStyle});
    assertThat(typedArray.hasValue(0)).isFalse();
  }

  @Test public void hasValue_withNullValue() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(R.attr.animalStyle, AttributeResource.NULL_VALUE)
            .build(),
        new int[]{R.attr.animalStyle});
    assertThat(typedArray.hasValue(0)).isFalse();
  }

  @Test public void shouldEnumeratePresentValues() throws Exception {
    TypedArray typedArray = context.obtainStyledAttributes(
        Robolectric.buildAttributeSet()
            .addAttribute(R.attr.responses, "@array/greetings")
            .addAttribute(R.attr.aspectRatio, "1")
            .build(),
        new int[]{R.attr.scrollBars, R.attr.responses, R.attr.isSugary});
    assertThat(typedArray.getIndexCount()).isEqualTo(1);
    assertThat(typedArray.getIndex(0)).isEqualTo(1);
  }
}
