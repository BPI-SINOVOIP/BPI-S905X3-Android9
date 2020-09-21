package autotest.common.ui;

import com.google.gwt.dom.client.Document;
import com.google.gwt.dom.client.InputElement;
import com.google.gwt.user.client.ui.TextBoxBase;

public class DateTimeBox extends TextBoxBase {
    public DateTimeBox() {
        super(Document.get().createTextInputElement());
        getElement().setAttribute("type", "datetime-local");
    }

    /*
     * See the "Setting maximum and minimum dates and times" section of
     * https://developer.mozilla.org/en-US/docs/Web/HTML/Element/input/datetime-local
     */
    public void setMin(String minDate) {
        getElement().setAttribute("min", minDate);
    }
}