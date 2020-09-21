package autotest.moblab;

import autotest.common.ui.TabView;
import autotest.moblab.rpc.ConnectedDutInfo;
import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.MoblabRpcHelper;
import com.google.gwt.event.dom.client.ChangeEvent;
import com.google.gwt.event.dom.client.ChangeHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.HasVerticalAlignment;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.TextArea;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;

/**
 * Implement a tab that makes it easier to add/remove/maintain duts.
 */
public class DutManagementView extends TabView {

  private FlexTable dutInfoTable;
  private VerticalPanel dutSetupPanel;
  private ListBox options;
  private TextArea informationArea;
  private Button actionButton;
  private CheckBox poolCheckBox;
  private TextBox poolLabelName;
  private Label poolLabel;
  private TextBox attributeName;
  private TextBox attributeValue;
  private HorizontalPanel labelActionRow;
  private HorizontalPanel attribActionRow;
  private HorizontalPanel attribValueActionRow;

  private static final int DHCP_IP_COLUMN = 0;
  private static final int DHCP_MAC_COLUMN = 1;
  private static final int SELECTION_COLUMN = 2;
  private static final int LABELS_COLUMN = 3;

  @Override
  public String getElementId() {
    return "dut_manage";
  }

  @Override
  public void refresh() {
    super.refresh();
    dutInfoTable.removeAllRows();
    poolCheckBox.setValue(false);
    poolLabelName.setText("");
    attributeName.setText("");
    attributeValue.setValue("");

    labelActionRow.setVisible(false);
    attribActionRow.setVisible(false);

    loadData();
  }

  @Override
  public void initialize() {
    super.initialize();
    // Main table of connected DUT information.
    dutInfoTable = new FlexTable();

    // The row of controls underneath the main data table.
    dutSetupPanel = new VerticalPanel();

    // List of actions to be applied to connected DUT's.
    options = new ListBox();
    options.addItem("Add Selected DUT's");
    options.addItem("Remove Selected DUT's");
    options.addItem("Add Label Selected DUT's");
    options.addItem("Remove Label Selected DUT's");
    options.addItem("Add Attribute to Selected DUT's");
    options.addItem("Remove Attribute from Selected DUT's");
    options.setStyleName("dut_manage_action_row");
    options.addChangeHandler(new ChangeHandler() {
      @Override
      public void onChange(ChangeEvent event) {
        if (options.getSelectedIndex() == 2 || options.getSelectedIndex() == 3) {
          poolCheckBox.setValue(false);
          poolLabelName.setText("");
          labelActionRow.setVisible(true);
          attribActionRow.setVisible(false);
        } else if (options.getSelectedIndex() == 4 || options.getSelectedIndex() == 5) {
          attributeName.setText("");
          attributeValue.setValue("");
          labelActionRow.setVisible(false);
          attribActionRow.setVisible(true);
          if (options.getSelectedIndex() == 4) {
            attribValueActionRow.setVisible(true);
          } else {
            attribValueActionRow.setVisible(false);
          }
        } else {
          labelActionRow.setVisible(false);
          attribActionRow.setVisible(false);
        }
      }
    });

    // Logging area at the end of the screen that gives status messages about bulk
    // actions requested.
    informationArea = new TextArea();
    informationArea.setVisibleLines(10);
    informationArea.setCharacterWidth(80);
    informationArea.setReadOnly(true);

    // Apply button, each action needs to be applied after selecting the devices and
    // the action to be performed.
    actionButton = new Button("Apply", new ClickHandler() {
      public void onClick(ClickEvent event) {
        ((Button)event.getSource()).setEnabled(false);
        int action = options.getSelectedIndex();
        try {
          for (int i = 1; i < dutInfoTable.getRowCount(); i++) {
            if (((CheckBox)dutInfoTable.getWidget(i, SELECTION_COLUMN)).getValue()) {
              if (action == 0) {
                  addDut(i);
              } else if (action == 1) {
                removeDut(i);
              } else if (action == 2) {
                addLabel(i, getPoolLabelString());
              } else if (action == 3) {
                removeLabel(i, getPoolLabelString());
              } else if (action == 4) {
                addAttribute(i, attributeName.getText(), attributeValue.getText());
              } else if (action == 5) {
                removeAttribute(i, attributeName.getText());
              }
            }
          }
        } finally {
          ((Button)event.getSource()).setEnabled(true);
        }
      }});


    // For adding and removing labels a text input of the label is required.
    poolCheckBox = new CheckBox();

    // Pools are just special labels, this is just a helper to so users get
    // it correct more of the time.
    poolCheckBox.setText("Is pool label ?");
    poolCheckBox.setStyleName("dut_manage_action_row_item");

    // The text label explaining the text box is for entering the label.
    poolLabel = new Label();
    poolLabel.setText("Label name:");
    poolLabel.setStyleName("dut_manage_action_row_item");

    // The text entry of the label to add or remove.
    poolLabelName = new TextBox();
    poolLabelName.setStyleName("dut_manage_action_row_item");

    labelActionRow = new HorizontalPanel();
    labelActionRow.add(poolCheckBox);
    labelActionRow.add(poolLabel);
    labelActionRow.add(poolLabelName);

    // The name/value pair required to set an attribute
    Label attributeNameLabel = new Label();
    attributeNameLabel.setText("Attribute:");
    attributeNameLabel.setStyleName("dut_manage_action_row_item");
    attributeName = new TextBox();
    attributeName.setStyleName("dut_manage_action_row_item");

    Label attributeValueLabel = new Label();
    attributeValueLabel.setText("Value:");
    attributeValueLabel.setStyleName("dut_manage_action_row_item");
    attributeValue = new TextBox();
    attributeValue.setStyleName("dut_manage_action_row_item");

    attribActionRow = new HorizontalPanel();
    attribActionRow.add(attributeNameLabel);
    attribActionRow.add(attributeName);
    attribValueActionRow = new HorizontalPanel();
    attribValueActionRow.add(attributeValueLabel);
    attribValueActionRow.add(attributeValue);
    attribActionRow.add(attribValueActionRow);

   // Assemble the display panels in the correct order.
    dutSetupPanel.add(dutInfoTable);
    HorizontalPanel actionRow = new HorizontalPanel();
    actionRow.setStyleName("dut_manage_action_row");
    actionRow.setVerticalAlignment(HasVerticalAlignment.ALIGN_MIDDLE);
    actionRow.add(options);
    actionRow.add(labelActionRow);
    actionRow.add(attribActionRow);
    actionRow.add(actionButton);
    dutSetupPanel.add(actionRow);
    dutSetupPanel.add(informationArea);
    addWidget(dutSetupPanel, "view_dut_manage");
  }

  private String getPoolLabelString() {
    StringBuilder builder = new StringBuilder(poolLabelName.getText());
    if (poolCheckBox.getValue()) {
      builder.insert(0, "pool:");
    }
    return builder.toString();
  }

  private void loadData() {
    MoblabRpcHelper.fetchDutInformation(new MoblabRpcCallbacks.FetchConnectedDutInfoCallback() {
      @Override
      public void onFetchConnectedDutInfoSubmitted(ConnectedDutInfo info) {
        addTableHeader();
        // The header is row 0
        int row = 1;
        for (final String dutIpAddress : info.getConnectedIpsToMacAddress().keySet()) {
          addRowStyles(row);
          String labelString;
          if (info.getConfiguredIpsToLabels().keySet().contains(dutIpAddress)) {
            labelString = info.getConfiguredIpsToLabels().get(dutIpAddress);
          } else {
            boolean sshOk =
                info.getConnectedIpsToSshConnection().get(dutIpAddress);
            labelString = sshOk ? "DUT Not Configured in Autotest" :
                "Unable to connect to DUT over SSH, check device is powered " +
                "on, connected and has a test image on it.";
          }
          addRow(row, dutIpAddress, info.getConnectedIpsToMacAddress().get(dutIpAddress), labelString);
          row++;
        }
        for (final String dutIpAddress : info.getConfiguredIpsToLabels().keySet()) {
          if (!info.getConnectedIpsToMacAddress().keySet().contains(dutIpAddress)) {
            // Device is in AFE but not detected in the DHCP table.
            addRowStyles(row);
            addRow(row, dutIpAddress, "",
                "DUT Configured in Autotest but does not appear to be attached.");
            row++;
          }
        }
      }
    });
  }

  /**
   * Add the correct css styles for each data row.
   * @param row index of the row to apply the styles for, first data row is 1.
   */
  private void addRowStyles(int row) {
    dutInfoTable.getCellFormatter().addStyleName(row, DHCP_IP_COLUMN,"ip_cell");
    dutInfoTable.getCellFormatter().addStyleName(row,DHCP_MAC_COLUMN,"mac_cell");
    dutInfoTable.getCellFormatter().addStyleName(row,SELECTION_COLUMN,"selection_cell");
    dutInfoTable.getCellFormatter().addStyleName(row,LABELS_COLUMN,"labels_cell");
  }

  /**
   * Insert or update the data in the table for a given row.
   * @param row  The row index to update, first data row is 1.
   * @param ipColumn String to be added into the first column.
   * @param macColumn String to be added to the second column.
   * @param labelsColumn String to be added to the fourth column.
   */
  private void addRow(int row, String ipColumn, String macColumn, String labelsColumn) {
    dutInfoTable.setWidget(row, DHCP_IP_COLUMN, new Label(ipColumn));
    dutInfoTable.setWidget(row, DHCP_MAC_COLUMN, new Label(macColumn));
    dutInfoTable.setWidget(row, SELECTION_COLUMN, new CheckBox());
    dutInfoTable.setWidget(row, LABELS_COLUMN, new Label(labelsColumn));
  }

  /**
   * Add the column headers with the correct css styling into the data table.
   */
  private void addTableHeader() {
    dutInfoTable.addStyleName("dut_info_table");
    dutInfoTable.getCellFormatter().addStyleName(0, DHCP_IP_COLUMN,
        "dut_manage_column_label_c");
    dutInfoTable.getCellFormatter().addStyleName(0, DHCP_MAC_COLUMN,
        "dut_manage_column_label_c");
    dutInfoTable.getCellFormatter().addStyleName(0, SELECTION_COLUMN,
        "dut_manage_column_label_c");
    dutInfoTable.getCellFormatter().addStyleName(0, LABELS_COLUMN,
        "dut_manage_column_label_c");
    dutInfoTable.setWidget(0, DHCP_IP_COLUMN, new Label("DCHP Lease Address"));
    dutInfoTable.setWidget(0, DHCP_MAC_COLUMN, new Label("DCHP MAC Address"));
    dutInfoTable.setWidget(0, LABELS_COLUMN, new Label("DUT Labels"));
  }

  /**
   * Make an RPC call to the autotest system to enroll the DUT listed at the given row number.
   * @param row_number the row number in the table that has details of the device to enroll.
   */
  private void addDut(int row_number) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.addMoblabDut(ipAddress, new LogAction(informationArea));
  }

  /**
   * Make an RPC to to the autotest system to delete information about the DUT listed at the given
   * row.
   * @param row_number the row number in the table that has details of the device to remove.
   */
  private void removeDut(int row_number) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.removeMoblabDut(ipAddress, new LogAction(informationArea));
  }

  /**
   * Make an RPC to to the autotest system to add a label to a DUT whoes details are in the given
   * row.
   * @param row_number row in the data table that has the information about the DUT
   * @param labelName the label string to be added.
   */
  private void addLabel(int row_number, String labelName) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.addMoblabLabel(ipAddress, labelName, new LogAction(informationArea));
  }

  /**
   * Make an RPC to to the autotest system to remove a label to a DUT whoes details are in the
   * given row.
   * @param row_number row in the data table that has the information about the DUT
   * @param labelName the label string to be removed.
   */
  private void removeLabel(int row_number, String labelName) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.removeMoblabLabel(ipAddress, labelName, new LogAction(informationArea));
  }

 /**
   * Make an RPC to to the autotest system to add an attribute to a DUT whoes details are in the
   * given row.
   * @param row_number row in the data table that has the information about the DUT
   * @param attributeName the attribute name to be set.
   * @param attributeValue the attribute value to be associated with the attributeName.
   */
  private void addAttribute(int row_number, String attributeName, String attributeValue) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.setMoblabAttribute(ipAddress, attributeName, attributeValue, new LogAction(informationArea));
  }

 /**
   * Make an RPC to to the autotest system to remove an attribute to a DUT whoes details are in the
   * given row.
   * @param row_number row in the data table that has the information about the DUT
   * @param attributeName the attribute name to be removed.
   */
  private void removeAttribute(int row_number, String attributeName) {
    String ipAddress = ((Label)dutInfoTable.getWidget(row_number, DHCP_IP_COLUMN)).getText();
    MoblabRpcHelper.removeMoblabAttribute(ipAddress, attributeName, new LogAction(informationArea));
  }

  /**
   * Call back that inserts a string from an completed RPC into the UI.
   */
  private static class LogAction implements MoblabRpcCallbacks.LogActionCompleteCallback {
    private TextArea informationArea;

    LogAction(TextArea informationArea){
      this.informationArea = informationArea;
    }
    @Override
    public void onLogActionComplete(boolean status, String information) {
      String currentText = informationArea.getText();
      informationArea
          .setText(new StringBuilder().append(information).append(
              "\n").append(currentText).toString());
    }
  }
}
