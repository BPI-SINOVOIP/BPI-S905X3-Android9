package org.robolectric.shadows;

import static android.os.Build.VERSION_CODES.KITKAT;
import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.robolectric.Shadows.shadowOf;

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.R;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
public class ShadowDialogTest {
  @Test
  public void shouldCallOnDismissListener() throws Exception {
    final List<String> transcript = new ArrayList<>();

    final Dialog dialog = new Dialog(RuntimeEnvironment.application);
    dialog.show();
    dialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
      @Override
      public void onDismiss(DialogInterface dialogInListener) {
        assertThat((Dialog) dialogInListener).isSameAs(dialog);
        transcript.add("onDismiss called!");
      }
    });

    dialog.dismiss();

    assertThat(transcript).containsExactly("onDismiss called!");
  }

  @Test
  public void setContentViewWithViewAllowsFindById() throws Exception {
    final int viewId = 1234;
    Context context = RuntimeEnvironment.application;
    final Dialog dialog = new Dialog(context);
    final View view = new View(context);
    view.setId(viewId);
    dialog.setContentView(view);

    assertSame(view, dialog.findViewById(viewId));
  }

  @Test
  public void shouldGetLayoutInflater() {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    assertNotNull(dialog.getLayoutInflater());
  }

  @Test
  public void shouldCallOnStartFromShow() {
    TestDialog dialog = new TestDialog(RuntimeEnvironment.application);
    dialog.show();

    assertTrue(dialog.onStartCalled);
  }

  @Test
  public void shouldSetCancelable() {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    ShadowDialog shadow = shadowOf(dialog);

    dialog.setCancelable(false);
    assertThat(shadow.isCancelable()).isFalse();
  }

  @Test
  public void shouldDismissTheRealDialogWhenCancelled() throws Exception {
    TestDialog dialog = new TestDialog(RuntimeEnvironment.application);
    dialog.cancel();
    assertThat(dialog.wasDismissed).isTrue();
  }

  @Test
  public void shouldDefaultCancelableToTrueAsTheSDKDoes() throws Exception {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    ShadowDialog shadow = shadowOf(dialog);

    assertThat(shadow.isCancelable()).isTrue();
  }

  @Test
  public void shouldOnlyCallOnCreateOnce() {
    final List<String> transcript = new ArrayList<>();

    Dialog dialog = new Dialog(RuntimeEnvironment.application) {
      @Override
      protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        transcript.add("onCreate called");
      }
    };

    dialog.show();
    assertThat(transcript).containsExactly("onCreate called");
    transcript.clear();

    dialog.dismiss();
    dialog.show();
    assertThat(transcript).isEmpty();
  }

  @Test
  public void show_setsLatestDialog() {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    assertNull(ShadowDialog.getLatestDialog());

    dialog.show();

    assertSame(dialog, ShadowDialog.getLatestDialog());
    assertNull(ShadowAlertDialog.getLatestAlertDialog());
  }

  @Test
  public void getLatestDialog_shouldReturnARealDialog() throws Exception {
    assertThat(ShadowDialog.getLatestDialog()).isNull();

    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    dialog.show();
    assertThat(ShadowDialog.getLatestDialog()).isSameAs(dialog);
  }

  @Test
  public void shouldKeepListOfOpenedDialogs() throws Exception {
    assertEquals(0, ShadowDialog.getShownDialogs().size());

    TestDialog dialog = new TestDialog(RuntimeEnvironment.application);
    dialog.show();

    assertEquals(1, ShadowDialog.getShownDialogs().size());
    assertEquals(dialog, ShadowDialog.getShownDialogs().get(0));

    TestDialog dialog2 = new TestDialog(RuntimeEnvironment.application);
    dialog2.show();

    assertEquals(2, ShadowDialog.getShownDialogs().size());
    assertEquals(dialog2, ShadowDialog.getShownDialogs().get(1));

    dialog.dismiss();

    assertEquals(2, ShadowDialog.getShownDialogs().size());

    ShadowDialog.reset();

    assertEquals(0, ShadowDialog.getShownDialogs().size());
  }

  @Test
  public void shouldPopulateListOfRecentDialogsInCorrectOrder() throws Exception {
    new NestingTestDialog().show();

    assertEquals(TestDialog.class, ShadowDialog.getLatestDialog().getClass());
  }

  @Test
  public void shouldFindViewsWithinAContentViewThatWasPreviouslySet() throws Exception {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    dialog.setContentView(dialog.getLayoutInflater().inflate(R.layout.main, null));
    assertThat(dialog.<TextView>findViewById(R.id.title)).isInstanceOf((Class<? extends TextView>) TextView.class);
  }

  @Test
  @Config(minSdk = KITKAT)
  public void show_shouldWorkWithAPI19() {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    dialog.show();
  }

  @Test
  public void canSetAndGetOnCancelListener() {
    Dialog dialog = new Dialog(RuntimeEnvironment.application);
    DialogInterface.OnCancelListener onCancelListener = new DialogInterface.OnCancelListener() {
      @Override
      public void onCancel(DialogInterface dialog) {

      }
    };
    dialog.setOnCancelListener(onCancelListener);
    assertThat(onCancelListener).isSameAs(shadowOf(dialog).getOnCancelListener());
  }

  private static class TestDialog extends Dialog {
    boolean onStartCalled = false;
    boolean wasDismissed = false;

    public TestDialog(Context context) {
      super(context);
    }

    @Override
    protected void onStart() {
      onStartCalled = true;
    }

    @Override public void dismiss() {
      super.dismiss();
      wasDismissed = true;
    }
  }

  private static class NestingTestDialog extends Dialog {
    public NestingTestDialog() {
      super(RuntimeEnvironment.application);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
      super.onCreate(savedInstanceState);
      new TestDialog(getContext()).show();
    }
  }
}
