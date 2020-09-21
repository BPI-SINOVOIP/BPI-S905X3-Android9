package autotest.moblab.wizard;

import com.google.gwt.user.client.ui.CaptionPanel;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.gwt.user.client.ui.Widget;


/**
 * A wizard view mode widget contains a list card. Each card show a title and
 * card widget. The view mode widget also has a set of navigation buttons on
 * the top.
 */
public class ConfigViewMode implements ConfigWizard.ConfigWizardMode {
  private ConfigWizard wizard;
  private CaptionPanel[] cardContainers;

  public ConfigViewMode(ConfigWizard wizard) {
    this.wizard = wizard;
  }

  private Widget reloadModeWidget() {
    VerticalPanel pnlContent = new VerticalPanel();
    pnlContent.setStyleName("wizard-view-panel");

    WizardCard[] cards = wizard.getCards();
    cardContainers = new CaptionPanel[cards.length];
    for (int count = 0; count < cards.length; count++) {
      CaptionPanel pnlCard = new CaptionPanel();
      pnlCard.setCaptionText(cards[count].getViewTitle());
      pnlContent.add(pnlCard);
      cardContainers[count] = pnlCard;
    }
    return pnlContent;
  }

  @Override
  public void hide() {
  }

  @Override
  public Widget display() {
    Widget widget = reloadModeWidget();
    WizardCard[] cards = wizard.getCards();
    for (int count = 0; count < cards.length; count++) {
      cardContainers[count].setContentWidget(cards[count].switchToMode(ConfigWizard.Mode.View));
    }
    return widget;
  }
}
