package autotest.moblab.wizard;

import com.google.gwt.json.client.JSONObject;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.SimplePanel;
import com.google.gwt.user.client.ui.Widget;

import autotest.moblab.rpc.OperationStatus;
import autotest.moblab.wizard.ConfigWizard.Mode;

import java.util.HashMap;

/**
 * The base class for cards that can be used with {@code ConfigWizard}. A card supports different
 * modes - currently view mode and edit mode. Each mode has its own UI and title. A card is used to
 * view and configure a piece of information.
 */
public abstract class WizardCard {
  private static final OperationStatus STATUS_OK = new OperationStatus(true);
  private String editTitle;
  private String viewTitle;
  private ConfigWizard.Mode currentMode;
  private SimplePanel pnlCard;
  private CardDataStatusListener listener;

  public WizardCard() {
    currentMode = ConfigWizard.Mode.View;
    pnlCard = new SimplePanel();
    pnlCard.setStyleName("wizard-card-panel");
  }

  /**
   * Resets the UI for re-display.
   */
  protected void resetUI() {}

  /**
   * Resets card data.
   */
  public void resetData() {}

  /**
   * Switches to a mode and update the UI.
   *
   * @param mode the mode to switch to.
   *
   * @return the root UI widget for the new mode.
   */
  public Widget switchToMode(ConfigWizard.Mode mode) {
    currentMode = mode;
    updateModeUI();
    return pnlCard;
  }

  /**
   * Updates the card UI based on the current mode.
   */
  protected void updateModeUI() {}

  public ConfigWizard.Mode getMode() {
    return currentMode;
  }

  /**
   * Returns if the card can go next. This is used to enable and disable the "next" button.
   */
  public boolean canGoNext() {
    return true;
  }

  protected void setCardContentWidget(Widget widget) {
    pnlCard.setWidget(widget);
  }

  // Asks the card to validate the data.
  public void validate(CardValidationCallback callback) {
    if (callback != null) {
      callback.onValidationStatus(STATUS_OK);
    }
    return;
  }

  /**
   * @return the editTitle
   */
  public String getEditTitle() {
    return editTitle;
  }

  /**
   * @param editTitle the editTitle to set
   */
  public void setEditTitle(String editTitle) {
    this.editTitle = editTitle;
  }

  public void setViewTitle(String viewTitle) {
    this.viewTitle = viewTitle;
  }

  public String getViewTitle() {
    return viewTitle;
  }

  public void setDataStatusListener(CardDataStatusListener listener) {
    this.listener = listener;
  }

  protected void fireDataStatusChanged() {
    if (listener != null) {
      listener.onDataStatusChange();
    }
  }

  /**
   * Callback interface to support asynchronous validation. Asynchronous is necessary for server
   * side validation.
   */
  public interface CardValidationCallback {
    public void onValidationStatus(OperationStatus status);
  }

  /**
   * Listener on card data status changed.
   */
  public interface CardDataStatusListener {
    public void onDataStatusChange();
  }

  /**
   * A dummy card for testing purpose.
   */
  public static class DummyCard extends WizardCard {
    public DummyCard() {
      setViewTitle("Dummy view");
      setEditTitle("Dummy Edit");
    }

    @Override
    protected void updateModeUI() {
      Mode mode = getMode();
      switch (mode) {
        case Edit:
          setCardContentWidget(new Label("Edit content"));
          break;
        default:
          setCardContentWidget(new Label("View content"));
      }
    }
  }

  /**
   * Collects the configuration data and fills it in a map.
   */
  public void collectConfigData(@SuppressWarnings("unused") HashMap<String, JSONObject> map) {}
}
