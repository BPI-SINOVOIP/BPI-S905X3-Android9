package autotest.moblab;

import autotest.common.ui.TabView;
import autotest.moblab.rpc.MoblabRpcCallbacks;
import autotest.moblab.rpc.MoblabRpcCallbacks.RunSuiteCallback;
import autotest.moblab.rpc.MoblabRpcHelper;
import autotest.moblab.rpc.ConnectedBoard;
import com.google.gwt.event.dom.client.ChangeEvent;
import com.google.gwt.event.dom.client.ChangeHandler;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.Window;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.HasVerticalAlignment;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.SimplePanel;
import com.google.gwt.user.client.ui.TextArea;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.Widget;
import java.util.Arrays;
import java.util.List;
import java.util.HashMap;


/**
 * Implement a tab to make a very easy way to run the most common moblab suites.
 */
public class SuiteRunnerView extends TabView {

  private VerticalPanel suiteRunnerMainPanel;
  private ListBox boardSelector;
  private ListBox buildSelector;
  private ListBox suiteSelector;
  private ListBox rwFirmwareSelector;
  private ListBox roFirmwareSelector;
  private ListBox poolSelector;
  private Button actionButton;
  private TextArea suiteArgsTextArea;
  private HorizontalPanel thirdOptionalLine;

  private TextBox bugIdTextBox;
  private HorizontalPanel fourthOptionalLine;

  private TextBox partIdTextBox;
  private HorizontalPanel fifthOptionalLine;

  private HashMap<String, String> modelBoardMap;

  private static List<String> suiteNames = Arrays.asList("bvt-cq",
      "bvt-inline", "cts_N", "gts",
      "hardware_storagequal", "hardware_memoryqual", "usb-camera");

  private static String TEST_LIST_PLACEHOLDER = "arm.CtsAnimationTestCases, x86.CtsDeqpTestCases";

  @Override
  public String getElementId() {
    return "suite_run";
  }

  @Override
  public void refresh() {
    super.refresh();
    boardSelector.clear();
    buildSelector.clear();
    suiteSelector.clear();
    rwFirmwareSelector.clear();
    roFirmwareSelector.clear();
    poolSelector.clear();
    suiteArgsTextArea.setText("");
    bugIdTextBox.setText("");
    partIdTextBox.setText("");

    buildSelector.addItem("Select the build");
    suiteSelector.addItem("Select the suite");
    poolSelector.addItem("Select the pool");

    rwFirmwareSelector.addItem("Select the RW Firmware (Faft only) (Optional)");
    roFirmwareSelector.addItem("Select the RO Firmware (Faft only) (Optional)");

    for (String suite : suiteNames) {
      suiteSelector.addItem(suite);
    }

    loadBoards();
    loadPools();
    addWidget(suiteRunnerMainPanel, "view_suite_run");
  };

  @Override
  public void initialize() {
    super.initialize();

    boardSelector = new ListBox();
    buildSelector = new ListBox();
    suiteSelector = new ListBox();
    rwFirmwareSelector = new ListBox();
    roFirmwareSelector = new ListBox();
    poolSelector = new ListBox();
    suiteArgsTextArea = new TextArea();
    suiteArgsTextArea.getElement().setPropertyString("placeholder", TEST_LIST_PLACEHOLDER);

    bugIdTextBox = new TextBox();
    partIdTextBox = new TextBox();

    boardSelector.addChangeHandler(new ChangeHandler() {
      @Override
      public void onChange(ChangeEvent event) {
        boardSelected();
      }
    });
    boardSelector.setStyleName("run_suite_selector");

    buildSelector.setEnabled(false);
    buildSelector.addChangeHandler(new ChangeHandler() {
      @Override
      public void onChange(ChangeEvent event) {
         buildSelected();
      }
    });
    buildSelector.setStyleName("run_suite_selector");

    suiteSelector.setEnabled(false);
    suiteSelector.addChangeHandler(new ChangeHandler() {
      @Override
      public void onChange(ChangeEvent event) {
        suiteSelected();
      }
    });
    suiteSelector.setStyleName("run_suite_selector");

    rwFirmwareSelector.setEnabled(false);
    rwFirmwareSelector.setStyleName("run_suite_selector");
    roFirmwareSelector.setEnabled(false);
    roFirmwareSelector.setStyleName("run_suite_selector");
    poolSelector.setEnabled(false);
    poolSelector.setStyleName("run_suite_selector");

    suiteArgsTextArea.setStyleName("run_suite_test_args");
    bugIdTextBox.setStyleName("run_suite_avl_args");
    partIdTextBox.setStyleName("run_suite_avl_args");

    HorizontalPanel firstLine = createHorizontalLineItem("Select board:", boardSelector);
    HorizontalPanel secondLine = createHorizontalLineItem("Select build:", buildSelector);
    HorizontalPanel thirdLine = createHorizontalLineItem("Select suite:", suiteSelector);
    thirdOptionalLine = createHorizontalLineItem("Only run specified tests (Optional):",
                                                 suiteArgsTextArea);
    thirdOptionalLine.setVisible(false);
    fourthOptionalLine = createHorizontalLineItem("AVL process bug ID (Optional):",
                                                 bugIdTextBox);
    fourthOptionalLine.setVisible(false);
    fifthOptionalLine = createHorizontalLineItem("AVL part number (Optional):",
                                                 partIdTextBox);
    fifthOptionalLine.setVisible(false);
    HorizontalPanel fourthLine = createHorizontalLineItem("RW Firmware (Optional):", rwFirmwareSelector);
    HorizontalPanel fifthLine = createHorizontalLineItem("RO Firmware (Optional):", roFirmwareSelector);
    HorizontalPanel sixthLine = createHorizontalLineItem("Pool (Optional):", poolSelector);

    actionButton = new Button("Run Suite", new ClickHandler() {
      public void onClick(ClickEvent event) {
        int boardSelection = boardSelector.getSelectedIndex();
        int buildSelection = buildSelector.getSelectedIndex();
        int suiteSelection = suiteSelector.getSelectedIndex();
        int poolSelection = poolSelector.getSelectedIndex();
        int rwFirmwareSelection = rwFirmwareSelector.getSelectedIndex();
        int roFirmwareSelection = roFirmwareSelector.getSelectedIndex();
        if (boardSelection != 0 && buildSelection != 0 && suiteSelection != 0) {
          String poolLabel = new String();
          if (poolSelection != 0) {
            poolLabel = poolSelector.getItemText(poolSelection);
          }
          String rwFirmware = new String();
          if (rwFirmwareSelection != 0) {
            rwFirmware = rwFirmwareSelector.getItemText(rwFirmwareSelection);
          }
          String roFirmware = new String();
          if (roFirmwareSelection != 0) {
            roFirmware = roFirmwareSelector.getItemText(roFirmwareSelection);
          }
          runSuite(getSelectedBoard(),
              boardSelector.getItemText(boardSelection),
              buildSelector.getItemText(buildSelection),
              suiteSelector.getItemText(suiteSelection),
              poolLabel,
              rwFirmware,
              roFirmware,
              suiteArgsTextArea.getText(),
              bugIdTextBox.getText(),
              partIdTextBox.getText());
        } else {
          Window.alert("You have to select a valid board, build and suite.");
        }
      }});

    actionButton.setEnabled(false);
    actionButton.setStyleName("run_suite_button");
    HorizontalPanel seventhLine = createHorizontalLineItem("", actionButton);

    suiteRunnerMainPanel = new VerticalPanel();

    suiteRunnerMainPanel.add(firstLine);
    suiteRunnerMainPanel.add(secondLine);
    suiteRunnerMainPanel.add(thirdLine);
    suiteRunnerMainPanel.add(thirdOptionalLine);
    suiteRunnerMainPanel.add(fourthOptionalLine);
    suiteRunnerMainPanel.add(fifthOptionalLine);
    suiteRunnerMainPanel.add(fourthLine);
    suiteRunnerMainPanel.add(fifthLine);
    suiteRunnerMainPanel.add(sixthLine);
    suiteRunnerMainPanel.add(seventhLine);
  }

  private HorizontalPanel createHorizontalLineItem(String label, Widget item) {
    HorizontalPanel panel = new HorizontalPanel();
    panel.add(contstructLabel(label));
    panel.add(item);
    return panel;
  }

  private Label contstructLabel(String labelName) {
    Label label = new Label(labelName);
    label.setStyleName("run_suite_label");
    return label;
  }

  private void suiteSelected() {
    int selectedIndex = suiteSelector.getSelectedIndex();
    if (selectedIndex != 0) {
      actionButton.setEnabled(true);
    }
    // Account for their being an extra item in the drop down
    int listIndex = selectedIndex - 1;
    if (listIndex  == suiteNames.indexOf("faft_setup") ||
      listIndex == suiteNames.indexOf("faft_bios") ||
      listIndex == suiteNames.indexOf("faft_ec")) {
      loadFirmwareBuilds(getSelectedBoard());
    } else {
      rwFirmwareSelector.setEnabled(false);
      roFirmwareSelector.setEnabled(false);
      rwFirmwareSelector.setSelectedIndex(0);
      roFirmwareSelector.setSelectedIndex(0);
    }

    if (listIndex  == suiteNames.indexOf("gts") ||
      listIndex == suiteNames.indexOf("cts_N")) {
      thirdOptionalLine.setVisible(true);
      fourthOptionalLine.setVisible(false);
      fifthOptionalLine.setVisible(false);
    } else if(listIndex == suiteNames.indexOf("hardware_storagequal") ||
        listIndex == suiteNames.indexOf("hardware_memoryqual")) {
      thirdOptionalLine.setVisible(false);
      fourthOptionalLine.setVisible(true);
      fifthOptionalLine.setVisible(true);
    } else {
      suiteArgsTextArea.setText("");
      thirdOptionalLine.setVisible(false);
      fourthOptionalLine.setVisible(false);
      fifthOptionalLine.setVisible(false);
    }
  }

  private void buildSelected() {
    int selectedIndex = buildSelector.getSelectedIndex();
    if (selectedIndex != 0) {
      suiteSelector.setEnabled(true);
    }
  }

  private void boardSelected() {
    suiteSelector.setEnabled(false);
    actionButton.setEnabled(false);
    String selectedBoard = getSelectedBoard();
    if (selectedBoard != null) {
      loadBuilds(selectedBoard);
    }
  }

  private String getSelectedBoard() {
    int selectedIndex = boardSelector.getSelectedIndex();
    if (selectedIndex != 0) {
      String model = boardSelector.getItemText(selectedIndex);
      return modelBoardMap.get(model);
    }
    else {
      return null;
    }
  }

  /**
   * Call an RPC to get the boards that are connected to the moblab and populate them in the
   * dropdown.
   */
  private void loadBoards() {
    boardSelector.setEnabled(false);
    boardSelector.clear();
    boardSelector.addItem("Select the board");
    MoblabRpcHelper.fetchConnectedBoards(new MoblabRpcCallbacks.FetchConnectedBoardsCallback() {
      @Override
      public void onFetchConnectedBoardsSubmitted(
          List<ConnectedBoard> connectedBoards) {
        modelBoardMap = new HashMap<String, String>();
        for (ConnectedBoard connectedBoard : connectedBoards) {
          // remember the board that goes with this model
          modelBoardMap.put(
              connectedBoard.getModel(), connectedBoard.getBoard());
          boardSelector.addItem(connectedBoard.getModel());
        }
        boardSelector.setEnabled(true);
      }
    });
  }

  /**
   * Call an RPC to get the pools that are configured on the devices connected to the moblab.
   * and fill in the .
   */
  private void loadPools() {
    poolSelector.setEnabled(false);
    poolSelector.clear();
    poolSelector.addItem("Select a pool");
    MoblabRpcHelper.fetchConnectedPools(new MoblabRpcCallbacks.FetchConnectedPoolsCallback() {
      @Override
      public void onFetchConnectedPoolsSubmitted(List<String> connectedPools) {
        for (String connectedPool : connectedPools) {
          poolSelector.addItem(connectedPool);
        }
        poolSelector.setEnabled(true);
      }
    });
  }


  /**
   * Make a RPC to get the most recent builds available for the specified board and populate them
   * in the dropdown.
   * @param selectedBoard
   */
  private void loadBuilds(String selectedBoard) {
    buildSelector.setEnabled(false);
    buildSelector.clear();
    buildSelector.addItem("Select the build");
    MoblabRpcHelper.fetchBuildsForBoard(selectedBoard,
        new MoblabRpcCallbacks.FetchBuildsForBoardCallback() {
      @Override
      public void onFetchBuildsForBoardCallbackSubmitted(List<String> boards) {
        for (String board : boards) {
          buildSelector.addItem(board);
        }
        buildSelector.setEnabled(true);
      }
    });
  }


  /**
   * Make a RPC to get the most recent firmware available for the specified board and populate them
   * in the dropdowns.
   * @param selectedBoard
   */
  private void loadFirmwareBuilds(String selectedBoard) {
    rwFirmwareSelector.setEnabled(false);
    roFirmwareSelector.setEnabled(false);
    rwFirmwareSelector.clear();
    roFirmwareSelector.clear();
    rwFirmwareSelector.addItem("Select the RW Firmware (Faft only) (Optional)");
    roFirmwareSelector.addItem("Select the RO Firmware (Faft only) (Optional)");
    MoblabRpcHelper.fetchFirmwareForBoard(selectedBoard,
        new MoblabRpcCallbacks.FetchFirmwareForBoardCallback() {
      @Override
      public void onFetchFirmwareForBoardCallbackSubmitted(List<String> firmwareBuilds) {
        for (String firmware : firmwareBuilds) {
          rwFirmwareSelector.addItem(firmware);
          roFirmwareSelector.addItem(firmware);
        }
        rwFirmwareSelector.setEnabled(true);
        roFirmwareSelector.setEnabled(true);
      }
    });
  }


  /**
   * For the selection option of board, build, suite and pool make a RPC call that will instruct
   * AFE to run the suite selected.
   * @param board, a string that specified a device connected to the moblab.
   * @param model, a string that specifies a device model connected to moblab.
   * @param build, a string that is a valid build for the specified board available in GCS.
   * @param suite, a string that specifies the name of a suite selected to run.
   * @param pool, an optional name of a pool to run the suite in.
   * @param rwFirmware, an optional firmware to use for some qual tests.
   * @param roFirmware, an optional firmware to use for some qual tests.
   * @param suiteArgs, optional params to pass to the suite.
   * @param bugId, an optional param indicates the bugnizer ticket for
   * memory/hardware avl process.
   * @param partId, an optional param identifies the component involved for
   * memory/hardare avl process.
   */
  private void runSuite(String board, String model, String build, String suite,
      String pool, String rwFirmware, String roFirmware, String suiteArgs,
      String bugId, String partId) {
    String realPoolLabel = pool;
    if (pool != null && !pool.isEmpty()) {
      realPoolLabel = pool.trim();
    }
    MoblabRpcHelper.runSuite(board, model, build, suite, realPoolLabel,
        rwFirmware, roFirmware, suiteArgs, bugId, partId,
        new RunSuiteCallback() {
      @Override
      public void onRunSuiteComplete() {
        Window.Location.assign("/afe");
      }
    });
  };

}
