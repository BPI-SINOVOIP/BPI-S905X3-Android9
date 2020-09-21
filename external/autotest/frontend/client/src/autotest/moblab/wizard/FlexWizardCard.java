package autotest.moblab.wizard;

import com.google.gwt.user.client.ui.FlexTable;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.PasswordTextBox;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.Widget;

import java.util.HashMap;
import java.util.Map;

/**
 * The base class for a card with flex table layout.
 */
abstract class FlexWizardCard extends WizardCard {
  /**
   * The layout table.
   */
  protected FlexTable layoutTable;

  /**
   * The card maintains a map from field id string to {@code TextBox}.
   */
  private Map<String, TextBox> fieldMap;

  protected FlexWizardCard() {
    super();
    createCardLayout();
    fieldMap = new HashMap<String, TextBox>();
  }

  protected void createCardLayout() {
    layoutTable = new FlexTable();
    layoutTable.getColumnFormatter().addStyleName(0, "wizard-card-property-name-col");
    layoutTable.getColumnFormatter().addStyleName(1, "wizard-card-property-value-col");
    setCardContentWidget(layoutTable);
  }

  /**
   * Creates a widget for String value field based on mode.
   */
  protected Widget createValueFieldWidget(String fieldId, String value) {
    return createStringValueFieldWidget(fieldId, value, false);
  }

  protected Widget createStringValueFieldWidget(String fieldId, String value,
      boolean passwordProtected) {
    Widget widget;
    if (ConfigWizard.Mode.Edit == getMode()) {
      TextBox textBox = createTextBox(value, passwordProtected);
      fieldMap.put(fieldId, textBox);
      widget = textBox;
    } else {
      if (value != null && passwordProtected) {
        value = "********";
      }
      widget = createLabel(value);
    }
    widget.setStyleName("wizard-card-property-value");
    return widget;
  }

  protected Widget createLabel(String value) {
    if (value != null) {
      return new Label(value);
    }
    return new Label();
  }

  protected TextBox createTextBox(String value, boolean passwordProtected) {
    TextBox textBox = passwordProtected ? new PasswordTextBox() : new TextBox();
    if (value != null) {
      textBox.setText(value);
    }
    return textBox;
  }

  @Override
  protected void resetUI() {
    layoutTable.removeAllRows();
    fieldMap.clear();
    super.resetUI();
  }

  protected TextBox getValueFieldEditor(String fieldId) {
    return fieldMap.get(fieldId);
  }
}