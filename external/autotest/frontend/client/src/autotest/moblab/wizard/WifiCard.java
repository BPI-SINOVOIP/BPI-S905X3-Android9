package autotest.moblab.wizard;

import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.TextBox;

import autotest.common.Utils;
import autotest.moblab.rpc.WifiInfo;
import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.MoblabRpcHelper;

import java.util.HashMap;

/**
 * Wizard card for wifi configuration.
 */
public class WifiCard extends FlexWizardCard {
  /**
   * The cached wifi information.
   */
  private WifiInfo wifiInfo;

  public WifiCard() {
    super();
    setViewTitle("DUT Wifi Configuration");
    setEditTitle("Configure DUT Wifi Access Configuration");
  }

  @Override
  protected void updateModeUI() {
    if (wifiInfo != null) {
      resetUI();
      int row = 0;

      // Row for AP.
      row++;
      layoutTable.setWidget(row, 0, new Label("Wifi Access Point Name: "));
      layoutTable.setWidget(row, 1, createValueFieldWidget(WifiInfo.JSON_FIELD_AP_NAME,
          wifiInfo.getApName()));

      // Row for wifi key.
      row++;
      layoutTable.setWidget(row, 0, new Label("Wifi Access Point Password: "));
      layoutTable.setWidget(row, 1, createStringValueFieldWidget(
          WifiInfo.JSON_FIELD_AP_PASS, wifiInfo.getApPass(), true));

    } else {
      MoblabRpcHelper.fetchWifiInfo(new MoblabRpcCallbacks.FetchWifiInfoCallback() {
        @Override
        public void onWifiInfoFetched(WifiInfo info) {
          wifiInfo = info;
          updateModeUI();
        }
      });
    }
  }

  @Override
  public void validate(final CardValidationCallback callback) {
    wifiInfo.setApName(getStringValueFieldValue(WifiInfo.JSON_FIELD_AP_NAME));
    wifiInfo.setApPass(getStringValueFieldValue(WifiInfo.JSON_FIELD_AP_PASS));
    WifiCard.super.validate(callback);
    return;
  }

  /**
   * Gets the string input field value.
   */
  protected String getStringValueFieldValue(String fieldId) {
    TextBox textBox = getValueFieldEditor(fieldId);
    String value = textBox.getValue();
    if (value != null) {
      value = value.trim();
    }

    if (value == null) {
      return null;
    }
    if (value.isEmpty()) {
      return null;
    }
    return value;
  }

  @Override
  public void resetData() {
    wifiInfo = null;
    super.resetData();
  }

  @Override
  public void collectConfigData(@SuppressWarnings("unused") HashMap<String, JSONObject> map) {
    if (map != null && wifiInfo != null) {
      map.put(MoblabRpcHelper.RPC_PARAM_WIFI_INFO, wifiInfo.toJson());
    }
  }
}
