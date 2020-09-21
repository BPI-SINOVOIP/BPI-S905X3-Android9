package autotest.moblab;

import com.google.gwt.user.client.ui.Widget;

import autotest.common.ui.TabView;
import autotest.moblab.wizard.CloudStorageCard;
import autotest.moblab.wizard.ConfigWizard;
import autotest.moblab.wizard.NetworkInfoCard;
import autotest.moblab.wizard.WifiCard;
import autotest.moblab.wizard.WizardCard;


/**
 * A Moblab configuration tab with a wizard widget.
 */
public class ConfigWizardView extends TabView {
  private ConfigWizard wizard;

  public ConfigWizardView() {
    super();
    wizard = new ConfigWizard();
    WizardCard[] cards = new WizardCard[] {
      new NetworkInfoCard(),
      new CloudStorageCard(),
      new WifiCard(),
    };
    wizard.setCards(cards);
  }

  @Override
  public String getElementId() {
    return "config_Wizard";
  }

  @Override
  public Widget getWidget() {
    return wizard;
  }
}
