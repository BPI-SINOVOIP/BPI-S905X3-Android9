package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;
import static org.robolectric.Shadows.shadowOf;

import android.view.MenuItem;
import android.view.View;
import android.widget.PopupMenu;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;

@RunWith(RobolectricTestRunner.class)
public class ShadowPopupMenuTest {

  private PopupMenu popupMenu;
  private ShadowPopupMenu shadowPopupMenu;

  @Before
  public void setUp() {
    View anchorView = new View(RuntimeEnvironment.application);
    popupMenu = new PopupMenu(RuntimeEnvironment.application, anchorView);
    shadowPopupMenu = shadowOf(popupMenu);
  }

  @Test
  public void testIsShowing_returnsFalseUponCreation() throws Exception {
    assertThat(shadowPopupMenu.isShowing()).isFalse();
  }

  @Test
  public void testIsShowing_returnsTrueIfShown() throws Exception {
    popupMenu.show();
    assertThat(shadowPopupMenu.isShowing()).isTrue();
  }

  @Test
  public void testIsShowing_returnsFalseIfShownThenDismissed() throws Exception {
    popupMenu.show();
    popupMenu.dismiss();
    assertThat(shadowPopupMenu.isShowing()).isFalse();
  }

  @Test
  public void getLatestPopupMenu_returnsNullUponCreation() throws Exception {
    assertThat(ShadowPopupMenu.getLatestPopupMenu()).isNull();
  }

  @Test
  public void getLatestPopupMenu_returnsLastMenuShown() throws Exception {
    popupMenu.show();
    assertThat(ShadowPopupMenu.getLatestPopupMenu()).isEqualTo(popupMenu);
  }

  @Test
  public void getOnClickListener_returnsOnClickListener() throws Exception {
    assertThat(shadowOf(popupMenu).getOnMenuItemClickListener()).isNull();

    PopupMenu.OnMenuItemClickListener listener = new PopupMenu.OnMenuItemClickListener() {
      @Override
      public boolean onMenuItemClick(MenuItem menuItem) {
        return false;
      }
    };
    popupMenu.setOnMenuItemClickListener(listener);

    assertThat(shadowOf(popupMenu).getOnMenuItemClickListener()).isEqualTo(listener);
  }
}
