package autotest.moblab.wizard;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.Label;

import autotest.common.ui.NotifyManager;
import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.MoblabRpcHelper;
import autotest.moblab.rpc.NetworkInfo;

/**
 * Wizard card for network info configuration.
 */
public class NetworkInfoCard extends FlexWizardCard {
  // The cached network info.
  private NetworkInfo networkInfo;

  // Button for network validation.
  private Button btnValidate;

  public NetworkInfoCard() {
    super();
    setViewTitle("Moblab Network Information");
    setEditTitle("Validate Moblab Network Information");

    btnValidate = new Button("Query");
    btnValidate.addClickHandler(new ClickHandler() {
      @Override
      public void onClick(ClickEvent event) {
        validateNetworkInfo(true);
      }
    });
  }

  @Override
  public boolean canGoNext() {
    return networkInfo != null && networkInfo.isConnectedToGoogle();
  }

  @Override
  protected void updateModeUI() {
    if (networkInfo != null) {
      resetUI();
      int row = 0;
      layoutTable.setWidget(row, 0, new Label("Server IP"));
      if (networkInfo.getServerIps().length > 0) {
        layoutTable.setWidget(row, 1, new Label(networkInfo.getServerIps()[0]));
      } else {
        layoutTable.setWidget(row, 1, new Label("Unknown"));
      }
      row++;
      layoutTable.setWidget(row, 0, new Label("Connected to Internet"));
      layoutTable.setWidget(row, 1,
          new Label(networkInfo.isConnectedToGoogle() ? "Yes" : "No"));
      row++;
      if (getMode() == ConfigWizard.Mode.Edit) {
        layoutTable.setWidget(row, 0, btnValidate);
      }
    } else {
      validateNetworkInfo(false);
    }
  }

  protected void validateNetworkInfo(final boolean notify) {
    MoblabRpcHelper.fetchNetworkInfo(new MoblabRpcCallbacks.FetchNetworkInfoRpcCallback() {
      @Override
      public void onNetworkInfoFetched(NetworkInfo info) {
        networkInfo = info;
        if (notify) {
          NotifyManager.getInstance().showMessage("Retrieved netowrk information.");
        }
        updateModeUI();
        fireDataStatusChanged();
      }
    });
  }

  @Override
  public void resetData() {
    networkInfo = null;
    super.resetData();
  }
}
