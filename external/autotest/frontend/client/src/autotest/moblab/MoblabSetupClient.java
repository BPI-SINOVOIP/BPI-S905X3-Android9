package autotest.moblab;

import autotest.common.JsonRpcProxy;
import autotest.common.ui.CustomTabPanel;
import autotest.common.ui.NotifyManager;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.user.client.ui.RootPanel;


public class MoblabSetupClient implements EntryPoint {
  private ConfigWizardView configWizardView;
  private ConfigSettingsView configSettingsView;
  private CredentialsUploadView keysUploadView;
  private DutManagementView dutManagementView;
  private SuiteRunnerView suiteRunnerView;

  public CustomTabPanel mainTabPanel = new CustomTabPanel(true);

  /**
   * Application entry point.
   */
  @Override
  public void onModuleLoad() {
    JsonRpcProxy.setDefaultBaseUrl(JsonRpcProxy.AFE_BASE_URL);
    NotifyManager.getInstance().initialize();

    configWizardView = new ConfigWizardView();
    configSettingsView = new ConfigSettingsView();
    keysUploadView = new CredentialsUploadView();
    dutManagementView = new DutManagementView();
    suiteRunnerView = new SuiteRunnerView();
    mainTabPanel.addTabView(configWizardView);
    mainTabPanel.addTabView(configSettingsView);
    mainTabPanel.addTabView(keysUploadView);
    mainTabPanel.addTabView(dutManagementView);
    mainTabPanel.addTabView(suiteRunnerView);

    final RootPanel rootPanel = RootPanel.get("tabs");
    rootPanel.add(mainTabPanel);
    mainTabPanel.initialize();
    rootPanel.setStyleName("");
  }
}
