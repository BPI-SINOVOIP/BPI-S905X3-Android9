package autotest.moblab.wizard;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.SimplePanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.Widget;

import autotest.common.ui.NotifyManager;
import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.MoblabRpcHelper;
import autotest.moblab.rpc.OperationStatus;

import java.util.HashMap;

/**
 * A wizard editing mode widget contains a list of configuration steps.
 * Each step show a validation status, a title for current card, a wizard
 * card widget, and a set of navigation buttons are displayed.
 *
 * -------------------------------------------------------------------------
 * |Validation Error Message (optional)                                    |
 * -------------------------------------------------------------------------
 * | Title                                                                 |
 * -------------------------------------------------------------------------
 * |Card Configuration Widget                                              |
 * -------------------------------------------------------------------------
 * |          Back             Cancel            Next (Finish)             |
 * -------------------------------------------------------------------------
 */
public class ConfigEditMode
    implements ConfigWizard.ConfigWizardMode, WizardCard.CardDataStatusListener {
  // Constants for invalid card index.
  private static final int INVALID_CARD_INDEX = -1;

  private ConfigWizard wizard;

  // Validation status
  private Label lblValidationStatus;

  // Title
  private Label lblCardTitle;

  // Wizard card widget container.
  private SimplePanel pnlCard;

  // Wizard navigation buttons.
  private Button btnBack;
  private Button btnNext;
  private Button btnCancel;

  // If the mode is visible.
  private boolean visible;

  // The current card index.
  private int currentCardIndex = INVALID_CARD_INDEX;

  public ConfigEditMode(ConfigWizard wizard) {
    this.wizard = wizard;
  }

  private Widget reloadModeWidget() {
    VerticalPanel pnlLayout = new VerticalPanel();
    pnlLayout.setStyleName("wizard-edit-panel");
    // status
    lblValidationStatus = new Label();
    lblValidationStatus.setStyleName("card-edit-status");
    pnlLayout.add(lblValidationStatus);

    // card title
    lblCardTitle = new Label();
    lblCardTitle.setStyleName("card-edit-title");
    pnlLayout.add(lblCardTitle);

    // card content
    pnlCard = new SimplePanel();
    pnlLayout.add(pnlCard);

    // navigation buttons
    HorizontalPanel pnlActions = new HorizontalPanel();
    btnBack = new Button("Back");
    btnNext = new Button("Next");
    btnCancel = new Button("Cancel");
    pnlActions.add(btnBack);
    pnlActions.add(btnCancel);
    pnlActions.add(btnNext);
    pnlLayout.add(pnlActions);

    btnBack.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        onGoBack();
      }
    });

    btnNext.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        onGoNext();
      }
    });

    btnCancel.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        onCancel();
      }
    });

    return pnlLayout;
  }

  private boolean isAtFirstCard() {
    return currentCardIndex == 0;
  }

  private boolean isAtLastCard() {
    return currentCardIndex == wizard.getCards().length - 1;
  }

  @Override
  public Widget display() {
    visible = true;
    Widget widget = reloadModeWidget();
    if (wizard.getCards().length == 0) {
      throw new RuntimeException("Empty wizard.");
    }
    currentCardIndex = 0;

    updateUIAtCurrentCard(null);
    return widget;
  }

  @Override
  public void hide() {
    visible = false;
  }

  private void updateUIAtCurrentCard(OperationStatus status) {
    // Updates the status panel.
    if (status != null && !status.isOk()) {
      if (status.getDetails() != null) {
        logError("Error:" + status.getDetails());
      } else {
        logError("Error: Unknown!");
      }
    } else {
      clearError();
    }

    // Update card title.
    int step = currentCardIndex + 1;
    WizardCard current = getCurrentCard();
    lblCardTitle.setText("Step " + step + " - " + current.getEditTitle());

    // Update the navigation buttons.
    if (isAtLastCard()) {
      btnNext.setText("Finish");
    } else {
      btnNext.setText("Next");
    }
    btnNext.setEnabled(current.canGoNext());
    btnBack.setEnabled(!isAtFirstCard());

    // Updates the card container with current card widget.
    pnlCard.setWidget(current.switchToMode(ConfigWizard.Mode.Edit));
  }

  /**
   * Clears the error message by hiding the widget.
   */
  protected void clearError() {
    lblValidationStatus.setVisible(false);
  }

  protected void logError(String message) {
    if (message != null) {
      lblValidationStatus.setText(message);
      lblValidationStatus.setVisible(true);
    }
  }

  protected void onGoNext() {
    clearError();
    getCurrentCard().validate(new WizardCard.CardValidationCallback() {
      @Override
      public void onValidationStatus(OperationStatus status) {
        if (status != null && status.isOk()) {
          if (!isAtLastCard()) {
            currentCardIndex++;
          } else {
            if (Window.confirm("Are you sure you want to commit the changes?")) {
              onFinish();
            }
          }
        }
        updateUIAtCurrentCard(status);
      }
    });
  }

  private WizardCard getCurrentCard() {
    return wizard.getCards()[currentCardIndex];
  }

  protected void onGoBack() {
    clearError();
    if (isAtFirstCard()) {
      throw new RuntimeException("Could not go back from the first card.");
    }
    currentCardIndex--;
    updateUIAtCurrentCard(null);
  }

  protected void onFinish() {
    HashMap<String, JSONObject> map = new HashMap<String, JSONObject>();
    for (WizardCard card : wizard.getCards()) {
      card.collectConfigData(map);
    }
    MoblabRpcHelper.submitWizardConfigData(map,
        new MoblabRpcCallbacks.SubmitWizardConfigInfoCallback() {
          @Override
          public void onWizardConfigInfoSubmitted(OperationStatus status) {
            if (status.isOk()) {
              reset();
              wizard.onFinishEdit();
              NotifyManager.getInstance().showMessage(
                  "Configuration is submitted. Some services are being restarted "
                  + "for new change to take effect.");
            } else {
              String details = status.getDetails();
              if (details == null) {
                details = "Operation Failed: erver side error.";
              }
              NotifyManager.getInstance().showError(status.getDetails());
            }
          }
        });
  }

  protected void onCancel() {
    reset();
    wizard.onCancelEdit();
  }

  protected void reset() {
    clearError();
    for (WizardCard card : wizard.getCards()) {
      card.resetData();
    }
  }

  @Override
  public void onDataStatusChange() {
    if (visible) {
      updateUIAtCurrentCard(null);
    }
  }
}
