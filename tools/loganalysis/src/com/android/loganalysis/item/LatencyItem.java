package com.android.loganalysis.item;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * An {@link IItem} used to LatencyInfo.
 */
public class LatencyItem extends GenericItem {

    /** Constant for JSON output */
    public static final String ACTION_ID = "ACTION_ID";
    /** Constant for JSON output */
    public static final String DELAY = "DELAY";
    /** Constant for JSON output */

    private static final Set<String> ATTRIBUTES = new HashSet<String>(Arrays.asList(
            ACTION_ID, DELAY));

    /**
     * The constructor for {@link LatencyItem}.
     */
    public LatencyItem() {
        super(ATTRIBUTES);
    }

    public int getActionId() {
        return (int) getAttribute(ACTION_ID);
    }

    public void setActionId(int actionId) {
        setAttribute(ACTION_ID, actionId);
    }

    public long getDelay() {
        return (long) getAttribute(DELAY);
    }

    public void setDelay(long delay) {
        setAttribute(DELAY, delay);
    }
}
