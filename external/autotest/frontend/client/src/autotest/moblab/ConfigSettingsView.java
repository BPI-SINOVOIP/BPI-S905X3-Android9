package autotest.moblab;

import autotest.common.JsonRpcCallback;
import autotest.common.SimpleCallback;
import autotest.common.Utils;
import autotest.common.ui.TabView;
import autotest.moblab.rpc.MoblabRpcHelper;
import autotest.common.ui.NotifyManager;

import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.ui.Anchor;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.PopupPanel;
import com.google.gwt.user.client.ui.VerticalPanel;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map.Entry;
import java.util.Set;

public class ConfigSettingsView extends TabView {
    private static final Set<String> GOOGLE_STORAGE_CONFIG_KEY = new HashSet<String>();
    static {
      GOOGLE_STORAGE_CONFIG_KEY.add("image_storage_server");
      GOOGLE_STORAGE_CONFIG_KEY.add("results_storage_server");
      GOOGLE_STORAGE_CONFIG_KEY.add("canary_channel_server");
      GOOGLE_STORAGE_CONFIG_KEY.add("chromeos_image_archive");
    }

    private Button submitButton;
    private Button resetButton;
    private HashMap<String, HashMap<String, TextBox> > configValueTextBoxes;
    private FlexTable configValueTable;
    private PopupPanel resetConfirmPanel;
    private Button resetConfirmButton;
    private PopupPanel submitConfirmPanel;
    private Button submitConfirmButton;

    @Override
    public void refresh() {
        super.refresh();
        configValueTable.removeAllRows();
        fetchConfigData(new SimpleCallback() {
            @Override
            public void doCallback(Object source) {
                loadData((JSONValue) source);
            }
        });
        resetConfirmPanel.hide();
    }

    @Override
    public String getElementId() {
        return "config_settings";
    }

    private PopupPanel getAlertPanel(String alertMessage, Button confirmButton){
        PopupPanel alertPanel = new PopupPanel(true);
        VerticalPanel alertInnerPanel = new VerticalPanel();
        alertInnerPanel.add(new Label(alertMessage));
        alertInnerPanel.add(confirmButton);
        alertPanel.setWidget(alertInnerPanel);
        return alertPanel;
    }

    @Override
    public void initialize() {
        super.initialize();
        configValueTable = new FlexTable();

        resetConfirmButton = new Button("Confirm Reset", new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                rpcCallReset();
                resetConfirmPanel.hide();
            }
        });

        resetConfirmPanel =getAlertPanel(
                "Restoring Default Settings requires rebooting the MobLab. Are you sure?",
                resetConfirmButton);

        submitConfirmButton = new Button("Confirm Save", new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                JSONObject configValues = new JSONObject();
                for (Entry<String, HashMap<String, TextBox> > sections : configValueTextBoxes.entrySet()) {
                    JSONArray sectionValue = new JSONArray();
                    for (Entry<String, TextBox> configValue : sections.getValue().entrySet()) {
                        JSONArray configValuePair = new JSONArray();
                        configValuePair.set(0, new JSONString(configValue.getKey()));
                        configValuePair.set(1, new JSONString(configValue.getValue().getText()));
                        sectionValue.set(sectionValue.size(), configValuePair);
                    }
                    configValues.put(sections.getKey(), sectionValue);
                }
                rpcCallSubmit(configValues);
                submitConfirmPanel.hide();
            }
        });

        submitConfirmPanel = getAlertPanel(
                "Saving settings requires restarting services on Moblab. Are you sure?",
                submitConfirmButton);

        submitButton = new Button("Submit", new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                submitConfirmPanel.center();
            }
        });

        resetButton = new Button("Restore Defaults", new ClickHandler() {
            @Override
            public void onClick(ClickEvent event) {
                resetConfirmPanel.center();
            }
        });

        addWidget(configValueTable, "view_config_values");
        addWidget(submitButton, "view_submit");
        addWidget(resetButton, "view_reset");
    }

    private void fetchConfigData(final SimpleCallback callBack) {
        MoblabRpcHelper.fetchConfigData(callBack);
    }

    private void loadData(JSONValue result) {
        configValueTextBoxes = new HashMap<String, HashMap<String, TextBox> >();
        JSONObject resultObject = result.isObject();
        for (String section : resultObject.keySet()) {
            JSONArray sectionArray = resultObject.get(section).isArray();
            HashMap<String, TextBox> sectionKeyValues = new HashMap<String, TextBox>();

            Label sectionLabel = new Label(section);
            sectionLabel.addStyleName("field-name");
            configValueTable.setWidget(configValueTable.getRowCount(), 0, sectionLabel);

            for (int i = 0; i < sectionArray.size(); i++) {
                JSONArray configPair = sectionArray.get(i).isArray();
                String configKey = configPair.get(0).isString().stringValue();
                String configValue = configPair.get(1).isString().stringValue();

                TextBox configInput = new TextBox();
                configInput.setVisibleLength(100);

                int row = configValueTable.getRowCount();
                configValueTable.setWidget(row, 0, new Label(configKey));
                configValueTable.setWidget(row, 1, configInput);
                configInput.setText(configValue);
                if (GOOGLE_STORAGE_CONFIG_KEY.contains(configKey)) {
                  Anchor a = Utils.createGoogleStorageHttpUrlLink("link", configValue);
                  if (a != null) {
                    configValueTable.setWidget(row, 2, a);
                  }
                }
                sectionKeyValues.put(configKey, configInput);
            }

            if (sectionArray.size() == 0) {
                configValueTable.setText(configValueTable.getRowCount(), 0,
                                         "No config values in this section.");
            }

            configValueTextBoxes.put(section, sectionKeyValues);
            // Add an empty row after each section.
            configValueTable.setText(configValueTable.getRowCount(), 0, "");
        }
    }

    public void rpcCallSubmit(JSONObject configValues) {
        MoblabRpcHelper.submitConfigData(configValues, new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                NotifyManager.getInstance().showMessage("Setup completed.");
            }
        });
    }

    public void rpcCallReset() {
        MoblabRpcHelper.resetConfigData(new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                NotifyManager.getInstance().showMessage("Reset completed.");
            }
        });
    }

}
