package autotest.common.table;

import autotest.common.ui.DateTimeBox;

import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.i18n.client.DateTimeFormat;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.Widget;
import com.google.gwt.user.datepicker.client.CalendarUtil;

import java.util.Date;

public class DatetimeSegmentFilter extends SimpleFilter {
    protected DateTimeBox startDatetimeBox;
    protected DateTimeBox endDatetimeBox;
    protected Panel panel;
    protected Label fromLabel;
    protected Label toLabel;
    private String placeHolderStartDatetime;
    private String placeHolderEndDatetime;

    // only allow queries of at most 2 weeks of data to reduce the load on the
    // database.
    private static final long MAXIMUM_TIME_RANGE_MS = 14 * 1000 * 60 * 60 * 24;
    private static final DateTimeFormat dateTimeFormat =
        DateTimeFormat.getFormat("yyyy-MM-dd");
    private static final DateTimeFormat parseFormat =
        DateTimeFormat.getFormat("yyyy-MM-ddTHH:mm");

    public DatetimeSegmentFilter() {
        startDatetimeBox = new DateTimeBox();
        endDatetimeBox = new DateTimeBox();
        fromLabel = new Label("From");
        toLabel = new Label("to");

        panel = new HorizontalPanel();
        panel.add(fromLabel);
        panel.add(startDatetimeBox);
        panel.add(toLabel);
        panel.add(endDatetimeBox);

        // The order here matters. It is not possible to set startTime that is
        // not close to endTime (see updateStartDateConstraint). So, we must
        // first set endTime to really get the startTime we want.
        // We want all entries from today, so advance end date to tomorrow.
        placeHolderEndDatetime = getDateTimeStringOffsetFromToday(1);
        setEndTimeToPlaceHolderValue();
        placeHolderStartDatetime = getDateTimeStringOffsetFromToday(-6);
        setStartTimeToPlaceHolderValue();

        addValueChangeHandler(
            new ValueChangeHandler() {
                public void onValueChange(ValueChangeEvent event) {
                    notifyListeners();
                }
            },
            new ValueChangeHandler<String>() {
                public void onValueChange(ValueChangeEvent<String> event) {
                    updateStartDateConstraint(event.getValue());
                    notifyListeners();
                }
            }
        );
    }

    /*
     * Put a 2-week constraint on the width of the date interval; whenever the
     * endDate changes, update the start date if needed, and update its minimum
     * Date to be two weeks earlier than the new endDate value.
     */
    public void updateStartDateConstraint(String newEndDateValue) {
        Date newEndDate = parse(newEndDateValue);
        String currentStartDateStr = startDatetimeBox.getValue();
        Date startDateConstraint = minimumStartDate(newEndDate);
        Date newStartDate = startDateConstraint;
        // Only compare to the existing start date if it has been set.
        if (!currentStartDateStr.equals("")) {
            Date currentStartDate = parse(currentStartDateStr);
            if (currentStartDate.compareTo(startDateConstraint) < 0) {
                newStartDate = startDateConstraint;
            }
        }
        startDatetimeBox.setValue(format(newStartDate));
        startDatetimeBox.setMin(format(startDateConstraint));
    }

    public static String format(Date date) {
        return dateTimeFormat.format(date) + "T00:00";
    }

    public static Date parse(String date) {
        return parseFormat.parse(date);
    }

    public static Date minimumStartDate(Date endDate) {
        long sinceEpoch = endDate.getTime();
        return new Date(sinceEpoch - MAXIMUM_TIME_RANGE_MS);
    }

    @Override
    public Widget getWidget() {
        return panel;
    }

    public void setStartTimeToPlaceHolderValue() {
        startDatetimeBox.setValue(placeHolderStartDatetime);
    }

    public void setEndTimeToPlaceHolderValue() {
        endDatetimeBox.setValue(placeHolderEndDatetime);
        updateStartDateConstraint(placeHolderEndDatetime);
    }

    public void addValueChangeHandler(ValueChangeHandler<String> startTimeHandler,
                                      ValueChangeHandler<String> endTimeHandler) {
        startDatetimeBox.addValueChangeHandler(startTimeHandler);
        endDatetimeBox.addValueChangeHandler(endTimeHandler);
    }

    private static String getDateTimeStringOffsetFromToday(int offsetDays) {
        Date placeHolderDate = new Date();
        CalendarUtil.addDaysToDate(placeHolderDate, offsetDays);
        return format(placeHolderDate);
    }
}
