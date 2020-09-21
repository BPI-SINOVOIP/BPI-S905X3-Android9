package org.robolectric.shadows;

import static org.robolectric.Shadows.shadowOf;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.widget.Adapter;
import android.widget.FrameLayout;
import com.android.internal.app.AlertController;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.RealObject;
import org.robolectric.shadow.api.Shadow;
import org.robolectric.util.ReflectionHelpers;

@SuppressWarnings({"UnusedDeclaration"})
@Implements(AlertDialog.class)
public class ShadowAlertDialog extends ShadowDialog {
  @RealObject
  private AlertDialog realAlertDialog;

  private CharSequence[] items;
  private DialogInterface.OnClickListener clickListener;
  private boolean isMultiItem;
  private boolean isSingleItem;
  private DialogInterface.OnMultiChoiceClickListener multiChoiceClickListener;
  private FrameLayout custom;

  /**
   * @return the most recently created {@code AlertDialog}, or null if none has been created during this test run
   */
  public static AlertDialog getLatestAlertDialog() {
    ShadowAlertDialog dialog = shadowOf(RuntimeEnvironment.application).getLatestAlertDialog();
    return dialog == null ? null : dialog.realAlertDialog;
  }

  public FrameLayout getCustomView() {
    if (custom == null) {
      custom = new FrameLayout(context);
    }
    return custom;
  }

  /**
   * Resets the tracking of the most recently created {@code AlertDialog}
   */
  public static void reset() {
    shadowOf(RuntimeEnvironment.application).setLatestAlertDialog(null);
  }

  /**
   * Simulates a click on the {@code Dialog} item indicated by {@code index}. Handles both multi- and single-choice dialogs, tracks which items are currently
   * checked and calls listeners appropriately.
   *
   * @param index the index of the item to click on
   */
  public void clickOnItem(int index) {
    Shadows.shadowOf(realAlertDialog.getListView()).performItemClick(index);
  }

  @Override public CharSequence getTitle() {
    return getShadowAlertController().getTitle();
  }

  /**
   * @return the items that are available to be clicked on
   */
  public CharSequence[] getItems() {
    Adapter adapter = getShadowAlertController().getAdapter();
    int count = adapter.getCount();
    CharSequence[] items = new CharSequence[count];
    for (int i = 0; i < items.length; i++) {
      items[i] = (CharSequence) adapter.getItem(i);
    }
    return items;
  }

  public Adapter getAdapter() {
    return getShadowAlertController().getAdapter();
  }

  /**
   * @return the message displayed in the dialog
   */
  public CharSequence getMessage() {
    return getShadowAlertController().getMessage();
  }

  @Override @Implementation
  public void show() {
    super.show();
    shadowOf(RuntimeEnvironment.application).setLatestAlertDialog(this);
  }

  /**
   * @return return the view set with {@link AlertDialog.Builder#setView(View)}
   */
  public View getView() {
    return getShadowAlertController().getView();
  }

  /**
   * @return return the view set with {@link AlertDialog.Builder#setCustomTitle(View)}
   */
  public View getCustomTitleView() {
    return getShadowAlertController().getCustomTitleView();
  }

  public ShadowAlertController getShadowAlertController() {
    AlertController alert = ReflectionHelpers.getField(realAlertDialog, "mAlert");
    return (ShadowAlertController) Shadow.extract(alert);
  }

  @Implements(AlertDialog.Builder.class)
  public static class ShadowBuilder {
  }
}
