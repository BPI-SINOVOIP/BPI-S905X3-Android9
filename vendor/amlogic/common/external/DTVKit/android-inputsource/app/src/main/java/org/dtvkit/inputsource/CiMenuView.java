package org.dtvkit.inputsource;

import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.text.InputFilter;
import android.text.method.PasswordTransformationMethod;
import android.util.Log;
import android.util.TypedValue;
import android.view.View;
import android.view.LayoutInflater;
import android.view.KeyEvent;

import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.LinearLayout;

import org.json.JSONArray;
import org.json.JSONObject;
import org.droidlogic.dtvkit.DtvkitGlueClient;

import java.util.ArrayList;

public class CiMenuView extends LinearLayout {

    private static final String TAG = "DtvkitCiMenu";
    private static final String EXIT_TO_QUIT = "Exit to quit";
    private static final int MENU_TIMEOUT_MESSAGE = 1;
    private static final int RETURN_BUTTON_NUM = 0;

    private static final int WAIT_CIMENU_TIMEOUT = 3000;

    private boolean signalTriggered = false;
    private boolean isMenuVisible = false;
    private final Handler uiHandler;
    private final Context mContext;

    private final DtvkitGlueClient.SignalHandler mHandler = new DtvkitGlueClient.SignalHandler() {
        @Override
        public void onSignal(String signal, JSONObject data) {
            if (signal.equals("CiOpenModule")) {
                Log.i(TAG, "Ci Menu: OnSignal " + signal);
                signalTriggered = true;
                menuHandler();
            }
            else if (signal.equals("CiCloseModule")) {
                Log.i(TAG, "Ci Menu: OnSignal " + signal);

                signalTriggered = true;
                menuCloseHandler("Ci Module closed by HOST", EXIT_TO_QUIT);
            }
            else if (signal.equals("CiRemoveModule")) {
                Log.i(TAG, "Ci Menu: OnSignal " + signal);

                signalTriggered = true;
                menuCloseHandler("Ci Module removed", EXIT_TO_QUIT);
            }
        }
    };

    private class TimerHandler extends Handler {
        public void handleMessage(Message msg) {
            if (msg.what == MENU_TIMEOUT_MESSAGE && signalTriggered == false) {
                setMenuTitleText("Ci menu response timeout. Check CAM is inserted");
                setMenuFooterText(EXIT_TO_QUIT);
            }
        }
    }

    private static class MenuItem {
        private String itemText;
        private int itemNum;

        public MenuItem(int itemNum, String itemText) {
            this.itemNum = itemNum;
            this.itemText = itemText;
        }

        public String getItemText() {
            return itemText;
        }

        public int getItemNum() {
            return itemNum;
        }
    }

    private class CiListMenu {
        public CiListMenu() {
            int numMenuItems;

            Log.i(TAG, "Entered menu List Handler");

            /* Generate list of options */
            readCiMenuTitle();

            numMenuItems = getCiNumMenuItems();
            readCiMenuOptions(numMenuItems);

            readCiMenuBottomText();
        }

        private void readCiMenuTitle() {
            String textBoxText;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiMenuScreenTitle", new JSONArray());
                textBoxText = "CAM ID: " + obj.getString("data");
                Log.i(TAG, "CiMenuTitle: " + textBoxText);
                setMenuTitleText(textBoxText);
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }
        }

        private int getCiNumMenuItems() {
            int numMenuItems = 0;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiMenuScreenNumItems", new JSONArray());
                numMenuItems = obj.getInt("data");
                Log.i(TAG, "Num menu options: " + Integer.toString(numMenuItems));
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }

            return numMenuItems;
        }

        private void readCiMenuOptions(final int numMenuItems) {
            String itemText;
            JSONArray args;
            ArrayList<MenuItem> menuItems = new ArrayList<MenuItem>();

            /* Get Args */
            try {
                for (int i = 1; i <= numMenuItems; i++) {
                    args = new JSONArray();
                    args.put(i-1);

                    JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiMenuScreenItemText", args);
                    itemText = Integer.toString(i) + ")" + formatSpaces(i) + obj.getString("data");

                    Log.i(TAG, itemText);
                    menuItems.add(new MenuItem(i, itemText));
                }
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }

            /* We get the args first to remove latency when printing the menu options */
            for (int i = 0; i < menuItems.size(); i++) {
                setUpAndPrintMenuItem(menuItems.get(i).getItemNum(), menuItems.get(i).getItemText());
            }

            /* Set focus on the first menu item */
            setMenuFocus(1);
        }

        private void setUpAndPrintMenuItem(final int buttonNum, final String itemText) {
            final LayoutInflater inflater = LayoutInflater.from(mContext);
            final Button button = (Button)inflater.inflate(R.layout.mmibutton, null);
            final LinearLayout linearLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    button.setLayoutParams(new LinearLayout.LayoutParams(findViewById(R.id.textViewMenuTitle).getWidth(), 40));
                    button.setText(itemText);
                    button.setId(buttonNum);

                    button.setFocusable(true);
                    button.setFocusableInTouchMode(true);

                    linearLayout.addView(button);

                    button.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            selectMenuOption(button.getId());
                            Log.i(TAG, "Clicking button " + Integer.toString(button.getId()));
                        }
                    });

                    button.setOnKeyListener(new View.OnKeyListener() {
                        @Override
                        public boolean onKey(View v, int keyCode, KeyEvent event) {
                            /* Return to previous level */
                            if (keyCode == KeyEvent.KEYCODE_CLEAR) {
                                selectMenuOption(RETURN_BUTTON_NUM);
                            }
                            return false;
                        }
                    });
                }
            });
        }

        private void selectMenuOption(final int optionId) {
            JSONArray args = new JSONArray();
            args.put(optionId);

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.setCiMenuResponse", args);
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }
        }

        private void readCiMenuBottomText() {
            String text;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiMenuScreenBottomText", new JSONArray());
                text = obj.getString("data");
                Log.i(TAG, "CiMenuFooter: " + text);
                setMenuFooterText(text);

            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }
        }

        private String formatSpaces(int menuNumber) {
            /* Compensate for extra digits if above 10 */
            String spaces;

            if (menuNumber < 10) {
                spaces = "    ";
            } else {
                spaces = "  ";
            }

            return spaces;
        }
    }

    private class CiEnquiryMenu {

        public CiEnquiryMenu() {
            String enquiryText;
            int enquiryMaxLength;
            boolean hideEnquiryResponse;

            enquiryText = readEnquiryText();
            enquiryMaxLength = readEnquiryMaxLength();
            hideEnquiryResponse = readEnquiryHideResponse();

            createEnquiryMenu(enquiryText, enquiryMaxLength, hideEnquiryResponse);
        }

        private String readEnquiryText() {
            String enquiry;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiEnquiryScreenText", new JSONArray());
                enquiry = obj.getString("data");
                Log.i(TAG, enquiry);

            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
                enquiry = "Enquiry ERROR";
            }
            return enquiry;
        }

        private int readEnquiryMaxLength() {
            int length;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiEnquiryResponseLength", new JSONArray());
                length = obj.getInt("data");
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
                length = 0;
            }
            return length;
        }

        private boolean readEnquiryHideResponse() {
            boolean hideEnquiryResponse = false;

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiEnquiryHideResponse", new JSONArray());
                hideEnquiryResponse = obj.getBoolean("data");
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }
            return hideEnquiryResponse;
        }

        private void createEnquiryMenu(final String text, final int maxLength, final boolean hide) {
            final LayoutInflater inflater = LayoutInflater.from(mContext);

            final LinearLayout parentLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);
            final LinearLayout enquiryLayout = (LinearLayout)inflater.inflate(R.layout.enquiryform, null);

            final TextView enquiryTextView = (TextView)(enquiryLayout.findViewById(R.id.textViewEnquiryName));
            final EditText enquiryEditText = (EditText)(enquiryLayout.findViewById(R.id.editTextEnquiry));

            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    enquiryTextView.setText(text);
                    enquiryEditText.requestFocus();

                    enquiryEditText.setFilters(new InputFilter[] {
                            new InputFilter.LengthFilter(maxLength)
                    });

                    if (hide) {
                        enquiryEditText.setTransformationMethod(PasswordTransformationMethod.getInstance());
                    }

                    parentLayout.addView(enquiryLayout);

                    enquiryEditText.setOnClickListener(new View.OnClickListener() {
                        @Override
                        public void onClick(View view) {
                            sendEnquiryResponse(enquiryEditText.getText().toString());
                        }
                    });
                }
            });
        }

        private void sendEnquiryResponse(final String response) {
            JSONArray args = new JSONArray();
            args.put(response);

            try {
                JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.setCiEnquiryResponse", args);
            } catch (Exception ignore) {
                Log.e(TAG, ignore.getMessage());
            }
        }
    }

    private enum MenuType {
        MENU_NONE("MENU_NONE") {
            public void menuHandler(CiMenuView Ci) {
                Ci.menuNoneHandler();
            }
        },
        MENU_ENQUIRY("MENU_ENQUIRY") {
            public void menuHandler(CiMenuView Ci) {
                Ci.menuEnquiryHandler();
            }
        },
        MENU_LIST("MENU_LIST") {
            public void menuHandler(CiMenuView Ci) {
                Ci.menuListHandler();
            }
        };

        private String text;

        MenuType(String text) {
            this.text = text;
        }

        public String getText() {
            return this.text;
        }

        public abstract void menuHandler(CiMenuView Ci);

        public static CiMenuView.MenuType fromString(String text) {
            for (CiMenuView.MenuType m : CiMenuView.MenuType.values()) {
                if (m.text.equalsIgnoreCase(text)) {
                    return m;
                }
            }
            throw new IllegalArgumentException("No constant with text " + text + " found");
        }
    }

    public CiMenuView(Context context) {
        super(context);

        Log.i(TAG, "onCreateCiMenuView");

        uiHandler = new Handler(context.getMainLooper());
        mContext = context;

        setMenuInvisible();
        inflate(getContext(), R.layout.cimenu,  this);

        startListeningForCamSignal();
    }

    public void destroy() {
        Log.i(TAG, "destroy and unregisterSignalHandler");
        stopListeningForCamSignal();
    }

    public boolean handleKeyDown(int keyCode, KeyEvent event) {
        boolean used;

        /* We action the Launch button regardless of if the menu is open */
        if (keyCode == KeyEvent.KEYCODE_NUM_LOCK) {
            used = true;
        }
        else {
            used = handleGenericKeypress(event);
        }

        Log.i(TAG, "Ci Menu Key down, used: " + Boolean.toString(used));

        return used;
    }

    public boolean handleKeyUp(int keyCode, KeyEvent event) {
        boolean used;

        switch (keyCode) {
            case KeyEvent.KEYCODE_NUM_LOCK:
                used = handleKeypressLaunch();
                break;
            case KeyEvent.KEYCODE_BACK:
                used = handleKeypressExit();
                break;
            case KeyEvent.KEYCODE_DPAD_UP:
                used = handleKeypressUp();
                break;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                used = handleKeypressDown();
                break;
            default:
                used = handleGenericKeypress(event);
                break;
        }
        Log.i(TAG, "Ci Menu Key up, used: " + Boolean.toString(used));

        return used;
    }

    private boolean handleKeypressLaunch() {
        launchCiMenu();
        return true;
    }

    private boolean handleKeypressExit() {
        boolean used = false;

        if (isMenuVisible) {
            exitCiMenu();
            used = true;
        }
        return used;
    }

    private boolean handleKeypressUp() {
        int currentFocus;
        boolean used = false;

        if (isMenuVisible) {
            currentFocus = getCurrentFocusedItem();
            if (currentFocus > 1) {
                setMenuFocus(currentFocus - 1);
            }
            used = true;
        }
        return used;
    }

    private boolean handleKeypressDown() {
        int currentFocus;
        boolean used = false;

        if (isMenuVisible) {
            currentFocus = getCurrentFocusedItem();
            if (currentFocus < numberOfMenuItems()) {
                setMenuFocus(currentFocus + 1);
            }
            used = true;
        }
        return used;
    }

    private boolean handleGenericKeypress(KeyEvent event) {
        LinearLayout mmiItems;
        View currentFocusedItem;
        boolean used = false;

        if (isMenuVisible) {
            mmiItems = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);
            currentFocusedItem = mmiItems.getFocusedChild();

            if (currentFocusedItem != null) {
                currentFocusedItem.dispatchKeyEvent(event);
            }
            used = true;
        }
        return used;
    }

    private boolean enterCiMenu() {
        boolean result = false;
        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.enterCiMenu", new JSONArray());
            result =  obj.getBoolean("data");

        } catch (Exception ignore) {
            Log.e(TAG, ignore.getMessage());
        }

        return result;
    }

    private void launchCiMenu() {
        TimerHandler timerHandler = new TimerHandler();

        setMenuVisible();

        if (enterCiMenu() == false) {
            setMenuTitleText("Ci Module not detected");
            setMenuFooterText(EXIT_TO_QUIT);
            Log.e(TAG, "Ci Module not detected");
        }
        else {
            /* Although enterCiMenu returns TRUE, there are no guarantees that the menu has been
               entered. We must wait for a signal before telling the user that a menu has not
               been found */
            timerHandler.sendEmptyMessageDelayed(MENU_TIMEOUT_MESSAGE, WAIT_CIMENU_TIMEOUT);
        }
    }

    private void exitCiMenu() {
        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.exitCiMenu", new JSONArray());
        } catch (Exception ignore) {
            Log.e(TAG, ignore.getMessage());
        }

        clearPreviousMenu();
        setMenuInvisible();
    }

    private void startListeningForCamSignal() {
        DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
    }

    private void stopListeningForCamSignal() {
        DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
    }

    private void menuHandler() {
        MenuType menuType;
        clearPreviousMenu();
        setMenuVisible();

        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getCiMenuMode", new JSONArray());
            menuType = MenuType.fromString(obj.getString("data"));
            Log.i(TAG, "menuHandler Menu type " + obj.getString("data"));
        } catch (Exception err) {
            Log.e(TAG, err.getMessage());
            menuType = MenuType.MENU_NONE;
        }

        menuType.menuHandler(this);
    }

    private void menuListHandler() {
        new CiListMenu();
    }

    private void menuEnquiryHandler() {
        new CiEnquiryMenu();
    }

    private void menuNoneHandler() {
        setMenuTitleText("No MMI menu found for this CAM");
        setMenuFooterText(EXIT_TO_QUIT);
    }

    private void menuCloseHandler(final String titleText, final String footerText) {
        if (isMenuVisible) {
            setMenuTitleText(titleText);
            setMenuFooterText(footerText);
        }
    }

    private void setMenuFocus(final int buttonNum) {
        final LinearLayout linearLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final Button button = (Button)linearLayout.findViewById(buttonNum);

                if (linearLayout.getChildCount() > 0) {
                    button.setFocusable(true);
                    button.setFocusableInTouchMode(true);
                    button.requestFocus();
                }
            }
        });
    }

    private void runOnUiThread(Runnable r) {
        uiHandler.post(r);
    }

    private void setMenuVisible() {
        final CiMenuView runnableView = this;

        Log.i(TAG, "set MMI Menu Visible");

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                runnableView.setVisibility(View.VISIBLE);
                runnableView.requestFocus();
            }
        });
        isMenuVisible = true;
    }

    private void setMenuInvisible() {
        final CiMenuView runnableView = this;

        Log.i(TAG, "set MMI Menu Invisible");

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                runnableView.setVisibility(View.INVISIBLE);
            }
        });
        isMenuVisible = false;
    }

    private void clearPreviousMenu() {
        final LinearLayout linearLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                linearLayout.removeAllViews();
            }
        });
    }

    private int getCurrentFocusedItem() {
        final LinearLayout linearLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);
        final View currentFocusChild = linearLayout.getFocusedChild();
        int focusedId = 0;

        if (currentFocusChild != null) {
            focusedId = currentFocusChild.getId();
        }

        return focusedId;
    }

    private int numberOfMenuItems() {
        final LinearLayout linearLayout = (LinearLayout)findViewById(R.id.linearlayoutMMIItems);
        return linearLayout.getChildCount();
    }

    private void setMenuTitleText(final String text) {
        Log.i(TAG, "setMenuTitleText: text = " + text);
        final TextView textMenuTitle = (TextView)findViewById(R.id.textViewMenuTitle);
        printReceivedSignal(text, textMenuTitle);
    }

    private void setMenuFooterText(final String text) {
        Log.i(TAG, "setMenuFooterText: text = " + text);
        final TextView textMenuFooter = (TextView)findViewById(R.id.textViewMenuFooter);
        printReceivedSignal(text, textMenuFooter);
    }

    private void printReceivedSignal(final String sigText, final TextView text) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "Entered print received signal with text: " + sigText);
                text.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
                text.setText(sigText);
            }
        });
    }
}
