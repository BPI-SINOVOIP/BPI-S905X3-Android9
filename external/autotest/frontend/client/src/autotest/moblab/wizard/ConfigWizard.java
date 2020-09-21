package autotest.moblab.wizard;

import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.VersionInfo;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.FlowPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.InlineLabel;
import com.google.gwt.user.client.ui.SimplePanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.Widget;

import autotest.common.JsonRpcCallback;
import autotest.common.ui.NotifyManager;
import autotest.moblab.rpc.MoblabRpcHelper;

import java.util.Collection;

/**
 * The configuration wizard for Moblab. A wizard supports multiple modes -
 * the view mode and the edit mode. Each mode has its own UI. A wizard has
 * a list of cards. Each card is responsible for the configuration of one
 * piece of information.
 */
public class ConfigWizard extends Composite {
  // All the wizard cards.
  private WizardCard[] cards;

  // The wizard header widget
  private Widget headerContainer;

  // The wizard footer widget
  private Widget footerContainer;

  // The container panel for wizard content widget.
  private SimplePanel contentContainer;

  // The current mode of the wizard - edit vs view
  private Mode mode;

  // The view mode controller.
  private ConfigViewMode viewMode;

  // The edit mode controller.
  private ConfigEditMode editMode;

  // Constructor.
  public ConfigWizard() {
    cards = new WizardCard[] {};

    VerticalPanel pnlWizard = new VerticalPanel();
    pnlWizard.setStyleName("wizard-layout");

    headerContainer = createWizardHeader();
    pnlWizard.add(headerContainer);

    contentContainer = new SimplePanel();
    contentContainer.setStyleName("wizard-panel");
    pnlWizard.add(contentContainer);

    footerContainer = createWizardFooter();
    pnlWizard.add(footerContainer);

    initWidget(pnlWizard);

    setStyleName("config-wizard");

    viewMode = new ConfigViewMode(this);
    editMode = new ConfigEditMode(this);

    // Default to view mode.
    setMode(Mode.View);
  }

  /**
   * Creates the wizard header widget.
   */
  protected Widget createWizardHeader() {
    FlowPanel headerPanel = new FlowPanel();
    headerPanel.setStyleName("wizard-header");
    Button btnStartConfig = new Button("Configure");
    headerPanel.add(btnStartConfig);
    btnStartConfig.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        onStartEdit();
      }
    });

    Button btnReboot = new Button("Reboot");
    headerPanel.add(btnReboot);
    btnReboot.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        if (Window.confirm("Are you sure you want to reboot the Moblab device?")) {
          MoblabRpcHelper.rebootMoblab(new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
              NotifyManager.getInstance().showMessage("Reboot command has been issued.");
            }
          });
        }
      }
    });
    return headerPanel;
  }

  /**
   * Creates the wizard header widget.
   */
  protected Widget createWizardFooter() {
    final FlexTable layoutTable = new FlexTable();
    MoblabRpcHelper.fetchMoblabBuildVersionInfo(new MoblabRpcCallbacks.FetchVersionInfoCallback() {
      @Override
      public void onVersionInfoFetched(VersionInfo info) {
        int row = 0;
        layoutTable.setWidget(row, 0, new Label("Version"));
        layoutTable.setWidget(row, 1, new Label(info.getVersion()));
        row++;

        layoutTable.setWidget(row, 0, new Label("Update"));
        FlowPanel updatePanel = new FlowPanel();
        updatePanel.add(new InlineLabel(info.getUpdateString()));
        Button btnUpdate = new Button(info.getUpdateAction());
        btnUpdate.addClickHandler(new ClickHandler() {
          @Override
          public void onClick(ClickEvent event) {
            String windowText = "If an update is available, the device will be "
              + "rebooted and all running jobs will be halted. "
              + "This may take 5-10 minutes based on network speed. Proceed?";
            if (Window.confirm(windowText)) {
              MoblabRpcHelper.updateMoblab(new JsonRpcCallback() {
                @Override
                public void onSuccess(JSONValue result) {
                  String messageText = "Device is rebooting";
                  NotifyManager.getInstance().showMessage(messageText);
                }
              });
            }
          }
        });
        updatePanel.add(btnUpdate);
        layoutTable.setWidget(row, 1, updatePanel);
        row++;

        layoutTable.setWidget(row, 0, new Label("Track"));
        layoutTable.setWidget(row, 1, new Label(info.getReleaseTrack()));
        row++;
        layoutTable.setWidget(row, 0, new Label("Description"));
        layoutTable.setWidget(row, 1, new Label(info.getReleaseDescription()));
        row++;
        layoutTable.setWidget(row, 0, new Label("Moblab Identification"));
        layoutTable.setWidget(row, 1, new Label(
            info.getMoblabIdentification()));
        row++;
        layoutTable.setWidget(row, 0, new Label("Moblab Serial Number"));
        layoutTable.setWidget(row, 1, new Label(info.getMoblabSerialNumber()));
      }
    });
    return layoutTable;
  }

  public WizardCard[] getCards() {
    return cards;
  }

  public void setCards(WizardCard[] cards) {
    if (cards != null) {
      this.cards = cards;
    } else {
      this.cards = new WizardCard[] {};
    }
    for (WizardCard card : cards) {
      card.setDataStatusListener(editMode);
    }
    updateModeUI();
  }

  public void setCards(Collection<WizardCard> cards) {
    WizardCard[] cardArray = null;
    if (cards != null) {
      cardArray = (WizardCard[]) cards.toArray();
    }
    setCards(cardArray);
    updateModeUI();
  }

  public Mode getMode() {
    return mode;
  }

  public void onFinishEdit() {
    setMode(Mode.View);
  }

  public void onCancelEdit() {
    setMode(Mode.View);
  }

  public void onStartEdit() {
    setMode(Mode.Edit);
  }

  private void setMode(Mode mode) {
    this.mode = mode;
    headerContainer.setVisible(Mode.View == mode);
    footerContainer.setVisible(Mode.View == mode);
    updateModeUI();
  }

  private void updateModeUI() {
    switch(this.mode) {
      case Edit:
        contentContainer.setWidget(editMode.display());
        viewMode.hide();
        break;
      case View:
        contentContainer.setWidget(viewMode.display());
        editMode.hide();
        break;
    }
  }

  public enum Mode {
    View,
    Edit
  }

  public interface ConfigWizardMode {
    public void hide();
    public Widget display();
  }
}
