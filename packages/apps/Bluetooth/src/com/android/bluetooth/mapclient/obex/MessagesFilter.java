package com.android.bluetooth.mapclient;

import java.util.Date;

/**
 * Object representation of filters to be applied on message listing
 *
 * @see #getMessagesListing(String, int, MessagesFilter, int)
 * @see #getMessagesListing(String, int, MessagesFilter, int, int, int)
 */
public final class MessagesFilter {

    public static final byte MESSAGE_TYPE_ALL = 0x00;
    public static final byte MESSAGE_TYPE_SMS_GSM = 0x01;
    public static final byte MESSAGE_TYPE_SMS_CDMA = 0x02;
    public static final byte MESSAGE_TYPE_EMAIL = 0x04;
    public static final byte MESSAGE_TYPE_MMS = 0x08;

    public static final byte READ_STATUS_ANY = 0x00;
    public static final byte READ_STATUS_UNREAD = 0x01;
    public static final byte READ_STATUS_READ = 0x02;

    public static final byte PRIORITY_ANY = 0x00;
    public static final byte PRIORITY_HIGH = 0x01;
    public static final byte PRIORITY_NON_HIGH = 0x02;

    public byte messageType = MESSAGE_TYPE_ALL;

    public String periodBegin = null;

    public String periodEnd = null;

    public byte readStatus = READ_STATUS_ANY;

    public String recipient = null;

    public String originator = null;

    public byte priority = PRIORITY_ANY;

    public MessagesFilter() {
    }

    public void setMessageType(byte filter) {
        messageType = filter;
    }

    public void setPeriod(Date filterBegin, Date filterEnd) {
        //Handle possible NPE for obexTime constructor utility
        if (filterBegin != null) {
            periodBegin = (new ObexTime(filterBegin)).toString();
        }
        if (filterEnd != null) {
            periodEnd = (new ObexTime(filterEnd)).toString();
        }
    }

    public void setReadStatus(byte readfilter) {
        readStatus = readfilter;
    }

    public void setRecipient(String filter) {
        if (filter != null && filter.isEmpty()) {
            recipient = null;
        } else {
            recipient = filter;
        }
    }

    public void setOriginator(String filter) {
        if (filter != null && filter.isEmpty()) {
            originator = null;
        } else {
            originator = filter;
        }
    }

    public void setPriority(byte filter) {
        priority = filter;
    }
}
