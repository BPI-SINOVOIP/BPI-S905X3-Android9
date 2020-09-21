package com.droidlogic.fragment.dialog;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;
import android.text.TextUtils;
import android.widget.Toast;

import com.droidlogic.fragment.ParameterMananer;
//import com.droidlogic.fragment.R;
import com.droidlogic.fragment.ScanMainActivity;

import java.util.LinkedList;
import java.util.Timer;
import java.util.TimerTask;

import org.dtvkit.inputsource.R;

public class CustomDialog/* extends AlertDialog*/ {

    private static final String TAG = "CustomDialog";
    private String mDialogType;
    private String mDialogTitleText;
    private String mDialogKeyText;
    private DialogCallBack mDialogCallBack;
    private Context mContext;
    private ParameterMananer mParameterMananer;
    private AlertDialog mAlertDialog = null;
    private View mDialogView = null;
    private TextView mDialogTitle = null;
    private DialogItemListView mListView = null;
    private ProgressBar mStrengthProgressBar = null;
    private ProgressBar mQualityProgressBar = null;
    private TextView mStrengthTextView = null;
    private TextView mQualityTextView = null;
    private LinkedList<DialogItemAdapter.DialogItemDetail> mItemList = new LinkedList<DialogItemAdapter.DialogItemDetail>();
    private DialogItemAdapter mItemAdapter = null;
    private String mSpinnerValue = null;

    public static final String DIALOG_SAVING = "saving";
    public static final String DIALOG_ADD_TRANSPONDER = "add_transponder";
    public static final String DIALOG_EDIT_TRANSPONDER = "edit_transponder";
    public static final String DIALOG_ADD_SATELLITE = "add_satellite";
    public static final String DIALOG_EDIT_SATELLITE = "edit_satellite";
    public static final String DIALOG_CONFIRM = "confirm";

    public static final String DIALOG_SET_SELECT_SINGLE_ITEM = "select_single_item";
    public static final String DIALOG_SET_EDIT_SWITCH_ITEM = "edit_switch_item";
    public static final String DIALOG_SET_EDIT_ITEM = "edit_item";
    public static final String DIALOG_SET_PROGRESS_ITEM = "progress_item";

    /*public static final String DIALOG_SET_SELECT_SINGLE_ITEM_SATALLITE = "Satallite";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_TRANOPONDER = "Transponder";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE = "LNB Type";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE = "Unicable";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER = "LNB Power";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ = "22KHz";
    public static final String DIALOG_SET_SELECT_SINGLE_TONE_BURST = "ToneBurst";
    public static final String DIALOG_SET_SELECT_SINGLE_DISEQC1_0 = "DiSEqC1.0";
    public static final String DIALOG_SET_SELECT_SINGLE_DISEQC1_1 = "DiSEqC1.1";
    public static final String DIALOG_SET_SELECT_SINGLE_MOTOR = "Motor";
    public static final String[] ID_DIALOG_TITLE_COLLECTOR = {DIALOG_SET_SELECT_SINGLE_ITEM_SATALLITE, DIALOG_SET_SELECT_SINGLE_ITEM_TRANOPONDER, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE,
            DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER, DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ, DIALOG_SET_SELECT_SINGLE_TONE_BURST, DIALOG_SET_SELECT_SINGLE_DISEQC1_0,
            DIALOG_SET_SELECT_SINGLE_DISEQC1_1, DIALOG_SET_SELECT_SINGLE_MOTOR};*/
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_SATALLITE = R.string.list_type_satellite;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_TRANOPONDER = R.string.list_type_transponder;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE = R.string.parameter_lnb_type;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE = R.string.parameter_unicable;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER = R.string.parameter_lnb_power;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ = R.string.parameter_22_khz;
    public static final int DIALOG_SET_SELECT_SINGLE_TONE_BURST = R.string.parameter_tone_burst;
    public static final int DIALOG_SET_SELECT_SINGLE_DISEQC1_0 = R.string.parameter_diseqc1_0;
    public static final int DIALOG_SET_SELECT_SINGLE_DISEQC1_1 = R.string.parameter_diseqc1_1;
    public static final int DIALOG_SET_SELECT_SINGLE_MOTOR = R.string.parameter_motor;
    public static final int[] ID_DIALOG_TITLE_COLLECTOR = {DIALOG_SET_SELECT_SINGLE_ITEM_SATALLITE, DIALOG_SET_SELECT_SINGLE_ITEM_TRANOPONDER, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE,
            DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER, DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ, DIALOG_SET_SELECT_SINGLE_TONE_BURST, DIALOG_SET_SELECT_SINGLE_DISEQC1_0,
            DIALOG_SET_SELECT_SINGLE_DISEQC1_1, DIALOG_SET_SELECT_SINGLE_MOTOR};
    //public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST = {"5150", "9750/10600", "Customize"};
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST = {R.string.parameter_lnb_type_5150, R.string.parameter_lnb_type_9750, R.string.parameter_lnb_custom};
    //public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST = {"first freq", "sencond freg"};
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST = {R.string.parameter_lnb_custom_frequency1, R.string.parameter_lnb_custom_frequency2};
    //public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_LIST = {"off", "on"};
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_LIST = {R.string.parameter_unicable_switch_off, R.string.parameter_unicable_switch_on};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_BAND = {"0", "1", "2", "3", "4", "5", "6", "7"};
    //public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_POSITION = {"disable", "enable"};
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_POSITION = {R.string.parameter_position_disable, R.string.parameter_position_enable};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST = {"13V", "18V", "off"/*, "13/18V"*/};
    //public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ_LIST = {"on", "off"/*, "auto"*/};
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ_LIST = {R.string.parameter_unicable_switch_off, R.string.parameter_unicable_switch_on};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_TONE_BURST_LIST = {"none", "a", "b"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_0_LIST = {"none", "1/4", "2/4", "3/4", "4/4"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_1_LIST = {"none", "1/16", "2/16", "3/16", "4/16", "5/16", "6/16", "7/16", "8/16", "9/16", "10/16", "11/16", "12/16", "13/16", "14/16", "15/16", "16/16"};
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_MOTOR_LIST = {"none", "DiSEqc1.2"};

    /*public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS = "Dish Limits Status";
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS_LIST = {"off", "on"};
    //public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_DISH_LIMITS = "Set Dish Limits East";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_EAST_DISH_LIMITS = "Set Dish Limits East";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_WEST_DISH_LIMITS = "Set Dish Limits West";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION = "Move Directtion";
    public static final String[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION_LIST = {"East", "West", "Center"};
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STEP = "Move Step";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE = "Move";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_POSITION = "Current Position";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SAVE_TO_POSITION = "Save To Position";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE_TO_POSITION = "Move To Position";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STRENGTH = "Strength";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_QUALITY = "Quality";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SAVE = "Save";
    public static final String DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SCAN= "Scan";*/
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS = R.string.parameter_diseqc1_2_dish_limits_status;
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS_LIST = {R.string.parameter_unicable_switch_off, R.string.parameter_unicable_switch_on};
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_EAST_DISH_LIMITS = R.string.parameter_diseqc1_2_dish_limits_east;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_WEST_DISH_LIMITS = R.string.parameter_diseqc1_2_dish_limits_west;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION = R.string.parameter_diseqc1_2_move_direction;
    public static final int[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION_LIST = {R.string.parameter_diseqc1_2_move_direction_east, R.string.parameter_diseqc1_2_move_direction_center, R.string.parameter_diseqc1_2_move_direction_west};
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STEP = R.string.parameter_diseqc1_2_move_step;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE = R.string.parameter_diseqc1_2_move;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_POSITION = R.string.parameter_diseqc1_2_current_position;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SAVE_TO_POSITION = R.string.parameter_diseqc1_2_save_to_position;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE_TO_POSITION = R.string.parameter_diseqc1_2_move_to_position;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STRENGTH = R.string.dialog_diseqc_1_2_strength;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_QUALITY = R.string.dialog_diseqc_1_2_quality;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SAVE = R.string.dialog_save;
    public static final int DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SCAN= R.string.dialog_scan;


    public static final /*String*/int[] DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST = {DIALOG_SET_SELECT_SINGLE_ITEM_SATALLITE, DIALOG_SET_SELECT_SINGLE_ITEM_TRANOPONDER, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS,
            DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_EAST_DISH_LIMITS, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SET_WEST_DISH_LIMITS, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION,
            DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STEP, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_POSITION,
            DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_SAVE_TO_POSITION, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_MOVE_TO_POSITION, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_STRENGTH,
            DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_QUALITY, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SAVE, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_SCAN
    };

    /*public static final String DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE = "Unicable";
    public static final String DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH = "Unicable Switch";
    public static final String DIALOG_SET_EDIT_SWITCH_ITEM_USER_BAND = "User Band";
    public static final String DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY = "UB Frequency";
    public static final String DIALOG_SET_EDIT_SWITCH_ITEM_POSITION = "Position";
    public static final String[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST = {DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH, DIALOG_SET_EDIT_SWITCH_ITEM_USER_BAND, DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY, DIALOG_SET_EDIT_SWITCH_ITEM_POSITION};
    public static final String[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH_LIST = {"off", "on"};
    public static final String[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_USER_BAND_LIST = {"0", "1", "2", "3", "4", "5", "6", "7"};
    public static final String[] DIALOG_SET_EDIT_ITEM_UNICABLE_UB_FREQUENCY_LIST = {"22KHz"};
    public static final String[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_POSITION_LIST = {"off", "on"};*/
    public static final int DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE = R.string.parameter_unicable;
    public static final int DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH = R.string.parameter_unicable_switch;
    public static final int DIALOG_SET_EDIT_SWITCH_ITEM_USER_BAND = R.string.parameter_unicable_user_band;
    public static final int DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY = R.string.parameter_ub_frequency;
    public static final int DIALOG_SET_EDIT_SWITCH_ITEM_POSITION = R.string.parameter_position;
    public static final int[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST = {DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH, DIALOG_SET_EDIT_SWITCH_ITEM_USER_BAND, DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY, DIALOG_SET_EDIT_SWITCH_ITEM_POSITION};
    public static final int[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_SWITCH_LIST = {R.string.parameter_unicable_switch_off, R.string.parameter_unicable_switch_on};
    public static final String[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_USER_BAND_LIST = {"0", "1", "2", "3", "4", "5", "6", "7"};
    public static final int[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_USER_BAND_FREQUENCY_LIST = {1284, 1400, 1516, 1632, 1748, 1864, 1980, 2096};
    public static final int[] DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_POSITION_LIST = {R.string.parameter_unicable_switch_off, R.string.parameter_unicable_switch_on};

    public CustomDialog(Context context, String type, DialogCallBack callback, ParameterMananer mananer) {
        //super(context);
        this.mContext = context;
        this.mDialogType = type;
        this.mDialogCallBack = callback;
        this.mParameterMananer = mananer;
    }

    /*public void initDialogWithButton(Context context, final String key, String positive, String negative) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        mAlertDialog = builder.create();
        /*mAlertDialog = builder.setPositiveButton(positive, new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Bundle bundle = new Bundle();
                bundle.putInt("which", which);
                bundle.putString("key", key);
                if (mDialogCallBack != null) {
                    mDialogCallBack.onStatusChange(null, mDialogType, bundle);
                }
            }
        })
                .setNegativeButton(negative, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        Bundle bundle = new Bundle();
                        bundle.putInt("which", which);
                        bundle.putString("key", key);
                        if (mDialogCallBack != null) {
                            mDialogCallBack.onStatusChange(null, mDialogType, bundle);
                        }
                    }
                }).create();*/
       /* mDialogView = View.inflate(mContext, R.layout.select_single_item_dialog, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        mListView = (DialogItemListView) mDialogView.findViewById(R.id.select_single_item_listview);
    }*/

    public interface DialogUiCallBack {
        void onStatusChange(View view, String dialogtype, Bundle data);
    }

    /*public boolean onKeyDown(int keyCode, KeyEvent event) {

        return super.onKeyDown(keyCode, event);
    }*/

    public String getDialogType() {
        return mDialogType;
    }

    public String getDialogKey() {
        return mDialogKeyText;
    }

    public String getDialogTitle() {
        return mDialogTitleText;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildLnbItem(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST[i]), "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildUnicableItem() {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST.length; i++) {
            switch (i) {
                case 0:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST[i]), mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_LIST[mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[i])]), false);
                    break;
                case 1:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST[i]), mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[i]) + "", false);
                    break;
                case 2://need to type in frequency
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST[i]), mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[i]) + "MHz", false);
                    break;
                case 3:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_EDIT_SWITCH_ITEM_UNICABLE_LIST[i]), mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_UNICABLE_POSITION[mParameterMananer.getIntParameters(ParameterMananer.DIALOG_SET_ITEM_UNICABLE_KEY_LIST[i])]), false);
                    break;
                default:
                    break;
            }
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildLnbPowerItem(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_POWER_LIST[i], "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> build22KhzItem(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_22KHZ_LIST[i]), "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildToneBurstItem(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_TONE_BURST_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, DIALOG_SET_SELECT_SINGLE_ITEM_TONE_BURST_LIST[i], "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildDiseqc1_0_Item(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_0_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_0_LIST[i], "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildDiseqc1_1_Item(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_1_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_1_LIST[i], "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> buildMotorItem(int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_MOTOR_LIST.length; i++) {
            boolean needSelect = select == i ? true : false;
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_SELECT, DIALOG_SET_SELECT_SINGLE_ITEM_MOTOR_LIST[i], "", needSelect);
            items.add(item);
        }
        return items;
    }

    private LinkedList<DialogItemAdapter.DialogItemDetail> getSelectSingleItemsByKey(String title, String key, int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> items = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        switch (key) {
            case ParameterMananer.KEY_LNB_TYPE:
                items.addAll(buildLnbItem(select));
                break;
            case ParameterMananer.KEY_UNICABLE:
                items.addAll(buildUnicableItem());
                break;
            case ParameterMananer.KEY_LNB_POWER:
                items.addAll(buildLnbPowerItem(select));
                break;
            case ParameterMananer.KEY_22_KHZ:
                items.addAll(build22KhzItem(select));
                break;
            case ParameterMananer.KEY_TONE_BURST:
                items.addAll(buildToneBurstItem(select));
                break;
            case ParameterMananer.KEY_DISEQC1_0:
                items.addAll(buildDiseqc1_0_Item(select));
                break;
            case ParameterMananer.KEY_DISEQC1_1:
                items.addAll(buildDiseqc1_1_Item(select));
                break;
            case ParameterMananer.KEY_MOTOR:
                items.addAll(buildMotorItem(select));
                break;
            default:
                break;
        }
        return items;
    }

    public void initListView(String title, String key, int select) {
        initListDialog(mContext);
        LinkedList<DialogItemAdapter.DialogItemDetail> itemlist = getSelectSingleItemsByKey(title, key, select);
        mItemAdapter = new DialogItemAdapter(itemlist, mContext);
        mListView.setAdapter(mItemAdapter);
        if (!ParameterMananer.KEY_UNICABLE_SWITCH.equals(key) && select >= 0 && select < itemlist.size()) {
            mListView.setSelection(select);
        } else {
            mListView.setSelection(0);
        }
        mDialogTitle.setText(title);
        mDialogTitleText = title;
        mListView.setKey(key);
        mDialogKeyText = key;
        mListView.setDialogCallBack(mDialogCallBack);
        mAlertDialog.setView(mDialogView);
    }

    public void updateListView(String title, String key, int select) {
        LinkedList<DialogItemAdapter.DialogItemDetail> itemlist = getSelectSingleItemsByKey(title, key, select);
        if (select >= 0 && select < itemlist.size()) {
            mListView.setSelection(select);
        } else {
            mListView.setSelection(0);
        }
        mItemAdapter.reFill(itemlist);
    }

    private void initListDialog(Context context) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.select_single_item_dialog, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        mListView = (DialogItemListView) mDialogView.findViewById(R.id.select_single_item_listview);
        mAlertDialog.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public void showDialog() {
        if (mAlertDialog != null && !mAlertDialog.isShowing()) {
            mAlertDialog.show();
        }
    }

    public boolean isShowing() {
        return mAlertDialog != null && mAlertDialog.isShowing();
    }

    public void dismissDialog() {
        if (mAlertDialog != null && mAlertDialog.isShowing()) {
            mAlertDialog.dismiss();
        }
    }

    /*public AlertDialog creatSelectSingleItemDialog(final String title, String key, int select) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        AlertDialog dialog = builder.create();
        View dialogView = View.inflate(mContext, R.layout.select_single_item_dialog, null);
        TextView dialogTtile = (TextView) dialogView.findViewById(R.id.dialog_title);
        dialogTtile.setText(title);
        DialogItemListView listView = (DialogItemListView) dialogView.findViewById(R.id.select_single_item_listview);
        LinkedList<DialogItemAdapter.DialogItemDetail> itemlist = getSelectSingleItemsByKey(title, key, select);
        DialogItemAdapter itemAdapter = new DialogItemAdapter(itemlist, mContext);
        listView.setAdapter(itemAdapter);
        if (select >= 0 && select < itemlist.size()) {
            listView.setSelection(select);
        } else {
            listView.setSelection(0);
        }
        listView.setTitle(title);
        listView.setKey(key);
        listView.setDialogCallBack(mDialogCallBack);
        /*listView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("title", title);
                    bundle.putInt("position", position);
                    mDialogCallBack.onStatusChange(title, bundle);
                }
            }
        });*/
        /*dialog.setView(dialogView);
        return dialog;
    }*/

    public void initLnbCustomedItemDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.set_custom_lnb, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);

        mDialogTitle.setText(DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST[2]);
        final EditText editText1 = (EditText)mDialogView.findViewById(R.id.edittext_frequency1);
        final EditText lowMineditText = (EditText)mDialogView.findViewById(R.id.edittext_low_min_frequency);
        final EditText lowMaxeditText = (EditText)mDialogView.findViewById(R.id.edittext_low_max_frequency);
        final EditText editText2 = (EditText)mDialogView.findViewById(R.id.edittext_frequency2);
        final EditText highMineditText = (EditText)mDialogView.findViewById(R.id.edittext_high_min_frequency);
        final EditText highMaxeditText = (EditText)mDialogView.findViewById(R.id.edittext_high_max_frequency);
        final LinearLayout lowBandContainer = (LinearLayout)mDialogView.findViewById(R.id.low_band_container);
        final LinearLayout highBandContainer = (LinearLayout)mDialogView.findViewById(R.id.high_band_container);
        final Spinner lowHighBandSpinner = (Spinner)mDialogView.findViewById(R.id.spinner_band_type);
        String customlnb = mParameterMananer.getStringParameters(ParameterMananer.KEY_LNB_CUSTOM);
        boolean customSingle = mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_SINGLE_DOUBLE) == ParameterMananer.DEFAULT_LNB_CUSTOM_SINGLE_DOUBLE;
        int customLowBandMin = mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MIN);
        int customLowBandMax = mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MAX);
        int customHighBandMin = mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MIN);
        int customHighBandMax = mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MAX);
        String[] customlnbvalue = null;
        int lowlnb = 0;
        int highlnb = 0;
        if (customlnb != null) {
            customlnbvalue = customlnb.split(",");
        }
        if (customlnbvalue != null && customlnbvalue.length == 1) {
            lowlnb = !TextUtils.isEmpty(customlnbvalue[0]) ? Integer.valueOf(customlnbvalue[0]) : 0;
        } else if (customlnbvalue != null && customlnbvalue.length == 2) {
            lowlnb = !TextUtils.isEmpty(customlnbvalue[0]) ? Integer.valueOf(customlnbvalue[0]) : 0;
            highlnb = !TextUtils.isEmpty(customlnbvalue[1]) ? Integer.valueOf(customlnbvalue[1]) : 0;
        } else {
            Log.d(TAG, "null lnb customized data!");
        }
        if (lowlnb >= 0) {
            editText1.setText(lowlnb + "");
        } else {
            editText1.setText("");
        }
        if (highlnb >= 0) {
            editText2.setText(highlnb + "");
        } else {
            editText2.setText("");
        }
        if (customLowBandMin >= 0) {
            lowMineditText.setText(customLowBandMin + "");
        } else {
            lowMineditText.setText("");
        }
        if (customLowBandMax >= 0) {
            lowMaxeditText.setText(customLowBandMax + "");
        } else {
            lowMaxeditText.setText("");
        }
        if (customHighBandMin >= 0) {
            highMineditText.setText(customHighBandMin + "");
        } else {
            highMineditText.setText("");
        }
        if (customHighBandMax >= 0) {
            highMaxeditText.setText(customHighBandMax + "");
        } else {
            highMaxeditText.setText("");
        }

        if (customSingle) {
            highBandContainer.setVisibility(View.GONE);
        } else {
            highBandContainer.setVisibility(View.VISIBLE);
        }
        lowHighBandSpinner.setSelection(mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_SINGLE_DOUBLE));
        lowHighBandSpinner.setOnItemSelectedListener(new OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (position == mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_SINGLE_DOUBLE)) {
                    Log.d(TAG, "lowHighBandSpinner onItemSelected same position = " + position);
                    return;
                }
                Log.d(TAG, "lowHighBandSpinner onItemSelected position = " + position);
                mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_SINGLE_DOUBLE, position);
                if (position == ParameterMananer.DEFAULT_LNB_CUSTOM_SINGLE_DOUBLE) {
                    highBandContainer.setVisibility(View.GONE);
                    //mParameterMananer.saveStringParameters(ParameterMananer.KEY_LNB_CUSTOM, ParameterMananer.KEY_LNB_TYPE_DEFAULT_SINGLE_VALUE);
                } else {
                    highBandContainer.setVisibility(View.VISIBLE);
                    //mParameterMananer.saveStringParameters(ParameterMananer.KEY_LNB_CUSTOM, ParameterMananer.KEY_LNB_TYPE_DEFAULT_VALUE);
                }
                /*mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MIN, ParameterMananer.VALUE_LNB_CUSTOM_MIN);
                mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MAX, ParameterMananer.VALUE_LNB_CUSTOM_MAX);
                mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MIN, ParameterMananer.VALUE_LNB_CUSTOM_MIN);
                mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MAX, ParameterMananer.VALUE_LNB_CUSTOM_MAX);*/
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {
                // TODO Auto-generated method stub
            }
        });

        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_LNB_CUSTOM);
                    bundle.putString("button", "ok");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    boolean isSingle = (mParameterMananer.getIntParameters(ParameterMananer.KEY_LNB_CUSTOM_SINGLE_DOUBLE) == ParameterMananer.DEFAULT_LNB_CUSTOM_SINGLE_DOUBLE);
                    String lowMinstr = (!TextUtils.isEmpty(lowMineditText.getText()) ? lowMineditText.getText().toString() : "");
                    String lowMaxstr = (!TextUtils.isEmpty(lowMaxeditText.getText()) ? lowMaxeditText.getText().toString() : "");
                    String highMaxstr = (!TextUtils.isEmpty(highMaxeditText.getText()) ? highMaxeditText.getText().toString() : "");
                    String highMinstr = (!TextUtils.isEmpty(highMineditText.getText()) ? highMineditText.getText().toString() : "");
                    Log.d(TAG, "initLnbCustomedItemDialog lowMinstr = " + lowMinstr + ", lowMaxstr = " + lowMaxstr + ",highMinstr = " + highMinstr + ", highMaxstr = " + highMaxstr);
                    /*bundle.putBoolean("isSingle", isSingle);
                    bundle.putInt("value1_min", Integer.valueOf(lowMinstr));
                    bundle.putInt("value1_max", Integer.valueOf(lowMaxstr));
                    bundle.putInt("value2_min", Integer.valueOf(highMinstr));
                    bundle.putInt("value2_max", Integer.valueOf(highMaxstr));*/
                    if ((TextUtils.isEmpty(editText1.getText()) || TextUtils.isEmpty(lowMineditText.getText()) || TextUtils.isEmpty(lowMaxeditText.getText())) ||
                            (!isSingle && (TextUtils.isEmpty(editText2.getText()) || TextUtils.isEmpty(highMineditText.getText()) || TextUtils.isEmpty(highMaxeditText.getText())))) {
                        Toast.makeText(mContext, R.string.dialog_parameter_not_complete, Toast.LENGTH_SHORT).show();
                        return;
                    }
                    if (!isSingle) {
                        bundle.putString("value2", editText2.getText() != null ? editText2.getText().toString() : "");
                        if (!TextUtils.isEmpty(lowMaxstr) && !TextUtils.isEmpty(lowMinstr) &&
                                !TextUtils.isEmpty(highMaxstr) && !TextUtils.isEmpty(highMinstr)) {
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MIN, Integer.valueOf(lowMinstr));
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MAX, Integer.valueOf(lowMaxstr));
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MIN, Integer.valueOf(highMinstr));
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_HIGH_MAX, Integer.valueOf(highMaxstr));
                        }
                    } else {
                        bundle.putString("value2", "");
                        if (!TextUtils.isEmpty(lowMaxstr) && !TextUtils.isEmpty(lowMinstr)) {
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MIN, Integer.valueOf(lowMinstr));
                            mParameterMananer.saveIntParameters(ParameterMananer.KEY_LNB_CUSTOM_LOW_MAX, Integer.valueOf(lowMaxstr));
                        }
                    }
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_LNB_CUSTOM, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_LNB_CUSTOM);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    bundle.putString("value2", editText2.getText() != null ? editText2.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_LNB_CUSTOM, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_LNB_CUSTOM);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    bundle.putString("value2", editText2.getText() != null ? editText2.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_LNB_CUSTOM, bundle);
                }
            }
        });

        String value = mParameterMananer.getStringParameters(ParameterMananer.KEY_LNB_CUSTOM);
        String[] resultValue = {"", ""};
        if (value != null) {
            String[] allvalue = value.split(",");
            if (allvalue != null && allvalue.length > 0 && allvalue.length <= 2) {
                for (int i = 0; i < allvalue.length; i++) {
                    resultValue[i] = allvalue[i];
                }
            }
        }

        editText1.setHint(resultValue[0]);
        editText2.setHint(resultValue[1]);
        mAlertDialog.setView(mDialogView);
        mAlertDialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        /*LinkedList<DialogItemAdapter.DialogItemDetail> itemlist = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        String value = mParameterMananer.getStringParameters(ParameterMananer.KEY_LNB_CUSTOM);
        String[] resultValue = new String[DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST.length];
        LinearLayout layout0 = new LinearLayout(mContext);
        LinearLayout.LayoutParams paras = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        layout0.setOrientation(LinearLayout.VERTICAL);
        layout0.setLayoutParams(paras);

        LinearLayout layout1 = new LinearLayout(mContext);
        layout1.setOrientation(LinearLayout.HORIZONTAL);
        layout1.setLayoutParams(paras);
        TextView itemname = new TextView(mContext);
        itemname.setText("itemname1");
        itemname.setLayoutParams(paras);


        layout0.addView(layout1, paras);
        if (value != null) {
            String[] allvalue = value.split(",");
            if (allvalue != null && allvalue.length > 0 && allvalue.length <= DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST.length) {
                for (int i = 0; i < allvalue.length; i++) {
                    resultValue[i] = allvalue[i];
                }
            }
        }
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST.length; i++) {
            item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT, DIALOG_SET_SELECT_SINGLE_ITEM_LNB_CUSTOM_TYPE_LIST[i], resultValue[i], false);
            itemlist.add(item);
        }
        mItemAdapter = new DialogItemAdapter(itemlist, mContext);
        mListView.setAdapter(mItemAdapter);
        mDialogTitle.setText(DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST[2]);
        mDialogTitleText = DIALOG_SET_SELECT_SINGLE_ITEM_LNB_TYPE_LIST[2];
        mListView.setKey(ParameterMananer.KEY_LNB_CUSTOM);
        mDialogKeyText = ParameterMananer.KEY_LNB_CUSTOM;
        mListView.setDialogCallBack(mDialogCallBack);
        mAlertDialog.setView(mDialogView);*/
    }

    /*public void initUnicableCustomedItemDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.set_custom_lnb, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);

        mDialogTitle.setText(DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY);
        final TextView text1 = (TextView)mDialogView.findViewById(R.id.text_frequency1);
        final TextView text2 = (TextView)mDialogView.findViewById(R.id.text_frequency2);
        final EditText editText1 = (EditText)mDialogView.findViewById(R.id.edittext_frequency1);
        final EditText editText2 = (EditText)mDialogView.findViewById(R.id.edittext_frequency2);
        text1.setText(DIALOG_SET_EDIT_SWITCH_ITEM_UB_FREQUENCY);
        text2.setVisibility(View.GONE);
        editText2.setVisibility(View.GONE);
        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_UB_FREQUENCY);
                    bundle.putString("button", "ok");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_UB_FREQUENCY, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_UB_FREQUENCY);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_UB_FREQUENCY, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_UB_FREQUENCY);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", editText1.getText() != null ? editText1.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_UB_FREQUENCY, bundle);
                }
            }
        });

        int value = mParameterMananer.getIntParameters(ParameterMananer.KEY_UB_FREQUENCY);
        editText1.setHint(value + "MHz");
        mAlertDialog.setView(mDialogView);
    }*/

    public void initDiseqc1_2_ItemDialog() {
        initDiseqc1_2_ItemDialog(true);
    }

    public void initDiseqc1_2_ItemDialog(boolean init) {
        if (init) {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            mAlertDialog = builder.create();
            mDialogView = View.inflate(mContext, R.layout.set_diseqc_1_2, null);
            mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
            mListView = (DialogItemListView) mDialogView.findViewById(R.id.switch_edit_item_list);
            mStrengthProgressBar = (ProgressBar)mDialogView.findViewById(R.id.strength_progressbar);
            mQualityProgressBar = (ProgressBar)mDialogView.findViewById(R.id.quality_progressbar);
            mStrengthTextView = (TextView)mDialogView.findViewById(R.id.strength_percent);
            mQualityTextView = (TextView)mDialogView.findViewById(R.id.quality_percent);
        }
        final TimerTask task = new TimerTask() {
            public void run() {
                ((ScanMainActivity)mContext).runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        int strength = mParameterMananer.getStrengthStatus();
                        int quality = mParameterMananer.getQualityStatus();
                        if (mStrengthProgressBar != null && mQualityProgressBar != null &&
                                mStrengthTextView != null && mQualityTextView != null) {
                            mStrengthProgressBar.setProgress(strength);
                            mQualityProgressBar.setProgress(quality);
                            mStrengthTextView.setText(strength + "%");
                            mQualityTextView.setText(quality + "%");
                            //Log.d(TAG, "run task get strength and quality");
                        }
                    }
                });
            }
        };
        final Timer timer = new Timer();

        mStrengthProgressBar.setProgress(mParameterMananer.getStrengthStatus());
        mQualityProgressBar.setProgress(mParameterMananer.getQualityStatus());
        mStrengthTextView.setText(mParameterMananer.getStrengthStatus() + "%");
        mQualityTextView.setText(mParameterMananer.getQualityStatus() + "%");
        mListView.setSelection(mParameterMananer.getIntParameters(ParameterMananer.KEY_DISEQC1_2));

        LinkedList<DialogItemAdapter.DialogItemDetail> itemlist = new LinkedList<DialogItemAdapter.DialogItemDetail>();
        DialogItemAdapter.DialogItemDetail item = null;
        for (int i = 0; i < DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST.length; i++) {
            item = null;
            switch (i) {
                case 0:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mParameterMananer.getStringParameters(mParameterMananer.KEY_SATALLITE), false);
                    break;
                case 1:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mParameterMananer.getStringParameters(mParameterMananer.KEY_TRANSPONDER), false);
                    break;
                case 2:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DISH_LIMITS_LIST[mParameterMananer.getIntParameters(mParameterMananer.KEY_DISEQC1_2_DISH_LIMITS_STATUS)]), false);
                    break;
                case 3:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(R.string.parameter_diseqc1_2_press_to_limit_east), false);
                    break;
                case 4:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(R.string.parameter_diseqc1_2_press_to_limit_west), false);
                    break;
                case 5:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST_DIRECTTION_LIST[mParameterMananer.getIntParameters(mParameterMananer.KEY_DISEQC1_2_DISH_MOVE_DIRECTION)]), false);
                    break;
                case 6:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), String.valueOf(mParameterMananer.getIntParameters(mParameterMananer.KEY_DISEQC1_2_DISH_MOVE_STEP)), false);
                    break;
                case 7:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(R.string.parameter_diseqc1_2_press_to_move), false);
                    break;
                case 8:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_EDIT_SWITCH, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), String.valueOf(mParameterMananer.getIntParameters(mParameterMananer.KEY_DISEQC1_2_DISH_CURRENT_POSITION)), false);
                    break;
                case 9:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(R.string.parameter_diseqc1_2_press_to_save), false);
                    break;
                case 10:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_DISPLAY, mContext.getString(DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i]), mContext.getString(R.string.parameter_diseqc1_2_press_to_move), false);
                    break;
                /*case 11:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_PROGRESS, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i], mParameterMananer.getStrengthStatus() + "%", false);
                    break;
                case 12:
                    item = new DialogItemAdapter.DialogItemDetail(DialogItemAdapter.DialogItemDetail.ITEM_PROGRESS, DIALOG_SET_SELECT_SINGLE_ITEM_DISEQC1_2_LIST[i], mParameterMananer.getQualityStatus() + "%", false);
                    break;*/
                default:
                    Log.d(TAG, "initDiseqc1_2_ItemDialog unkown key");
                    break;
            }
            if (item != null) {
                itemlist.add(item);
            }
        }

        if (init) {
            mItemAdapter = new DialogItemAdapter(itemlist, mContext);
            mListView.setAdapter(mItemAdapter);
            timer.schedule(task, 1000, 1000);
        } else {
            mItemAdapter.reFill(itemlist);
            task.cancel();
            timer.cancel();
            return;
        }

        mDialogTitle.setText(CustomDialog.DIALOG_SET_SELECT_SINGLE_ITEM_MOTOR_LIST[1]);
        mDialogTitleText = CustomDialog.DIALOG_SET_SELECT_SINGLE_ITEM_MOTOR_LIST[1];
        mListView.setKey(ParameterMananer.KEY_DISEQC1_2);
        mDialogKeyText = ParameterMananer.KEY_DISEQC1_2;
        mListView.setDialogCallBack(mDialogCallBack);

        /*final TextView save = (TextView) mDialogView.findViewById(R.id.status_save);
        final TextView scan = (TextView) mDialogView.findViewById(R.id.status_scan);

        save.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_LNB_CUSTOM);
                    bundle.putString("button", "save");
                    mDialogCallBack.onStatusChange(save, ParameterMananer.KEY_DISEQC1_2, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        scan.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_LNB_CUSTOM);
                    bundle.putString("button", "scan");
                    mDialogCallBack.onStatusChange(scan, ParameterMananer.KEY_DISEQC1_2, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });*/
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_DISEQC1_2);
                    mDialogCallBack.onStatusChange(null, ParameterMananer.KEY_DISEQC1_2, bundle);
                }
                timer.cancel();
            }
        });

        mAlertDialog.setView(mDialogView);
    }

    public void updateDiseqc1_2_Dialog() {
        initDiseqc1_2_ItemDialog(false);
    }

    /*public CustomDialog getDialog(String type) {
        switch (type) {
            case DIALOG_SAVING:
                return creatSavingDialog();
            case DIALOG_ADD_TRANSPONDER:
                return initAddTransponderDialog();
            case DIALOG_EDIT_TRANSPONDER:
                return creatEditTransponderDialog();
            case DIALOG_ADD_SATELLITE:
                return creatAddSatelliteDialog();
            case DIALOG_EDIT_SATELLITE:
                return creatEditSatelliteDialog();
            case DIALOG_CONFIRM:
                //return creatConfirmDialog();
            default:
                Log.w(TAG, "getCustomDialog not exist!");
                return null;
        }
    }*/

    public CustomDialog creatSavingDialog() {
        //setContentView(R.layout.saving);
        return this;
    }

    public void initAddSatelliteDialog(final String name) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.add_satellite, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        if (name != null) {
            mDialogTitle.setText(R.string.dialog_edit_satellite);
        } else {
            mDialogTitle.setText(R.string.dialog_add_satellite);
        }
        mSpinnerValue = "";

        final EditText satellite = (EditText)mDialogView.findViewById(R.id.edittext_satellite);
        final Spinner spinner = (Spinner)mDialogView.findViewById(R.id.spinner_direction);
        final EditText longitude = (EditText)mDialogView.findViewById(R.id.edittext_longitude);
        if (name != null) {
            String satellitename1 = mParameterMananer.getSatelliteName(name);
            String direction1 = mParameterMananer.getSatelliteDirection(name);
            String longitude1 = mParameterMananer.getSatelliteLongitude(name);
            satellite.setHint(satellitename1);
            longitude.setHint(longitude1);
            mSpinnerValue = direction1;
            spinner.setSelection("east".equals(direction1) ? 0 : 1);
            longitude.setHint(longitude1);
        }
        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);
        final String[] SPINNER_VALUES = {"east", "west"};
        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mSpinnerValue = SPINNER_VALUES[position];
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_ADD_SATELLITE);
                    bundle.putString("button", "ok");
                    String value1 = !TextUtils.isEmpty(satellite.getText()) ? satellite.getText().toString() : (satellite.getHint() != null ? satellite.getHint().toString() : "");
                    boolean value2 = "east".equals(mSpinnerValue != null ? mSpinnerValue : "east");
                    String value3 = !TextUtils.isEmpty(longitude.getText()) ? longitude.getText().toString() : (longitude.getHint() != null ? longitude.getHint().toString() : "");
                    //Log.d(TAG, "initAddSatelliteDialog value1 = " + value1 + ", value2 = " + value2 + ", value3 = " + value3);
                    if (TextUtils.isEmpty(value1) || TextUtils.isEmpty(value3)) {
                        Toast.makeText(mContext, R.string.dialog_parameter_not_complete, Toast.LENGTH_SHORT).show();
                        return;
                    }
                    bundle.putString("value1", value1);
                    bundle.putBoolean("value2", value2);
                    bundle.putString("value3", value3);
                    if (name != null) {
                        mParameterMananer.removeSatellite(name);
                    }
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_SATELLITE, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_ADD_SATELLITE);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", satellite.getText() != null ? satellite.getText().toString() : (satellite.getHint() != null ? satellite.getHint().toString() : ""));
                    bundle.putString("value2", mSpinnerValue != null ? mSpinnerValue : "");
                    bundle.putString("value3", longitude.getText() != null ? longitude.getText().toString() : (longitude.getHint() != null ? longitude.getHint().toString() : ""));
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_SATELLITE, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_ADD_SATELLITE);
                    bundle.putString("button", "unkown");
                    bundle.putString("value1", satellite.getText() != null ? satellite.getText().toString() : "");
                    bundle.putString("value2", mSpinnerValue != null ? mSpinnerValue : "");
                    bundle.putString("value3", longitude.getText() != null ? longitude.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_SATELLITE, bundle);
                }
            }
        });

        mAlertDialog.setView(mDialogView);
    }

    public void initRemoveSatelliteDialog(final String name) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.remove_satellite, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        mDialogTitle.setText(mContext.getString(R.string.dialog_remove_satellite) + ": " + name);

        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_SATELLITE);
                    bundle.putString("button", "ok");
                    bundle.putString("value1", name != null ? name : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_SATELLITE, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_SATELLITE);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", name != null ? name : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_SATELLITE, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_SATELLITE);
                    bundle.putString("button", "unkown");
                    bundle.putString("value1", name != null ? name : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_SATELLITE, bundle);
                }
            }
        });

        mAlertDialog.setView(mDialogView);
    }

    public void initAddTransponderDialog(final String parameter) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.add_transponder, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        if (parameter != null) {
            mDialogTitle.setText(R.string.dialog_edit_transponder);
        } else {
            mDialogTitle.setText(R.string.dialog_add_transponder);
        }
        final EditText satellite = (EditText)mDialogView.findViewById(R.id.edittext_satellite);
        final EditText frequency = (EditText)mDialogView.findViewById(R.id.edittext_frequency);
        final Spinner spinner = (Spinner)mDialogView.findViewById(R.id.spinner_polarity);
        final EditText symbol = (EditText)mDialogView.findViewById(R.id.edittext_symbol);
        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);
        final String[] SPINNER_VALUES = {"H", "V"};

        if (parameter != null) {
            String satellitename1 = "";
            String frequency1 = "";
            String polarity1 = "";
            String symbolrate1 = "";
            String[] list = parameter.split("/");
            if (list != null && list.length == 3) {
                satellitename1 = mParameterMananer.getCurrentSatellite();
                for (int i = 0; i < list.length; i++) {
                    switch (i) {
                        case 0:
                            frequency1 = list[0];
                            break;
                        case 1:
                            polarity1 = list[1];
                            break;
                        case 2:
                            symbolrate1 = list[2];
                            break;
                        default:
                            Log.d(TAG, "erro part");
                            break;
                    }
                }
            }

            satellite.setHint(satellitename1);
            frequency.setHint(frequency1);
            mSpinnerValue = polarity1;
            spinner.setSelection("H".equals(polarity1) ? 0 : 1);
            symbol.setHint(symbolrate1);
        }

        spinner.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mSpinnerValue = SPINNER_VALUES[position];
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_ADD_TRANSPONDER);
                    bundle.putString("button", "ok");
                    String value1 = !TextUtils.isEmpty(satellite.getText()) ? satellite.getText().toString() : (satellite.getHint() != null ? satellite.getHint().toString() : "");
                    String value2 = !TextUtils.isEmpty(frequency.getText()) ? frequency.getText().toString() : (frequency.getHint() != null ? frequency.getHint().toString() : "");
                    String value4 = !TextUtils.isEmpty(symbol.getText()) ? symbol.getText().toString() : (symbol.getHint() != null ? symbol.getHint().toString() : "");
                    if (TextUtils.isEmpty(value1) || TextUtils.isEmpty(value2) || TextUtils.isEmpty(value4)) {
                        Toast.makeText(mContext, R.string.dialog_parameter_not_complete, Toast.LENGTH_SHORT).show();
                        return;
                    }
                    bundle.putString("value1", value1);
                    bundle.putString("value2", value2);
                    bundle.putString("value3", mSpinnerValue != null ? mSpinnerValue : "");
                    bundle.putString("value4", value4);

                    String[] singleParameter = null;
                    if (parameter != null) {
                        singleParameter = parameter.split("/");
                        if (singleParameter != null && singleParameter.length == 3) {
                            mParameterMananer.removeTransponder(mParameterMananer.getCurrentSatellite(), Integer.valueOf(singleParameter[0]), singleParameter[1], Integer.valueOf(singleParameter[2]));
                        } else {
                            Log.d(TAG, "initAddTransponderDialog delete fail as wrong format");
                            return;
                        }
                    }

                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_TRANSPONDER, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_ADD_TRANSPONDER);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", satellite.getText() != null ? satellite.getText().toString() : (satellite.getHint() != null ? satellite.getHint().toString() : ""));
                    bundle.putString("value2", frequency.getText() != null ? frequency.getText().toString() : (frequency.getHint() != null ? frequency.getHint().toString() : ""));
                    bundle.putString("value3", mSpinnerValue != null ? mSpinnerValue : "");
                    bundle.putString("value4", symbol.getText() != null ? symbol.getText().toString() : (symbol.getHint() != null ? symbol.getHint().toString() : ""));
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_TRANSPONDER, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_ADD_TRANSPONDER);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", satellite.getText() != null ? satellite.getText().toString() : "");
                    bundle.putString("value2", frequency.getText() != null ? frequency.getText().toString() : "");
                    bundle.putString("value3", mSpinnerValue != null ? mSpinnerValue : "");
                    bundle.putString("value4", symbol.getText() != null ? symbol.getText().toString() : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_ADD_TRANSPONDER, bundle);
                }
            }
        });

        mAlertDialog.setView(mDialogView);
    }

    public void initRemoveTransponderDialog(final String parameter) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        mAlertDialog = builder.create();
        mDialogView = View.inflate(mContext, R.layout.remove_transponder, null);
        mDialogTitle = (TextView) mDialogView.findViewById(R.id.dialog_title);
        mDialogTitle.setText(mContext.getString(R.string.dialog_remove_transponder) + ": " + parameter);

        final Button ok = (Button) mDialogView.findViewById(R.id.button1);
        final Button cancel = (Button) mDialogView.findViewById(R.id.button2);

        ok.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_TRANSPONDER);
                    bundle.putString("button", "ok");
                    //bundle.putString("value1", parameter != null ? parameter : "");
                    String[] singleParameter = null;
                    if (parameter != null) {
                        singleParameter = parameter.split("/");
                        if (singleParameter != null && singleParameter.length == 3) {
                            bundle.putString("value2", singleParameter[0]);
                            bundle.putString("value3", singleParameter[1]);
                            bundle.putString("value4", singleParameter[2]);
                        } else {
                            return;
                        }
                    } else {
                        return;
                    }
                    bundle.putString("value1", mParameterMananer.getCurrentSatellite());
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_TRANSPONDER, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onClick");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_TRANSPONDER);
                    bundle.putString("button", "cancel");
                    bundle.putString("value1", parameter != null ? parameter : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_TRANSPONDER, bundle);
                    if (mAlertDialog != null) {
                        mAlertDialog.dismiss();
                    }
                }
            }
        });
        mAlertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallBack != null) {
                    Bundle bundle = new Bundle();
                    bundle.putString("action", "onDismiss");
                    bundle.putString("key", ParameterMananer.KEY_REMOVE_TRANSPONDER);
                    bundle.putString("button", "unkown");
                    bundle.putString("value1", parameter != null ? parameter : "");
                    mDialogCallBack.onStatusChange(ok, ParameterMananer.KEY_REMOVE_TRANSPONDER, bundle);
                }
            }
        });

        mAlertDialog.setView(mDialogView);
    }

    public CustomDialog creatEditTransponderDialog() {
        AlertDialog dialog = null;
        AlertDialog.Builder builder = null;

        return this;
    }

    public CustomDialog creatAddSatelliteDialog() {
        AlertDialog dialog = null;
        AlertDialog.Builder builder = null;

        return this;
    }

    public CustomDialog creatEditSatelliteDialog() {
        AlertDialog dialog = null;
        AlertDialog.Builder builder = null;

        return this;
    }

    public AlertDialog creatConfirmDialog() {
        AlertDialog dialog = null;
        AlertDialog.Builder builder = null;
        dialog = (AlertDialog) builder.setPositiveButton(mContext.getString(R.string.dialog_ok), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Bundle bundle = new Bundle();
                bundle.putInt("which", which);
                mDialogCallBack.onStatusChange(null, mDialogType, bundle);
            }
        })
        .setNegativeButton(mContext.getString(R.string.dialog_cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Bundle bundle = new Bundle();
                bundle.putInt("which", which);
                mDialogCallBack.onStatusChange(null, mDialogType, bundle);
            }
        }).create();
        //dialog.setContentView(R.layout.confirm);
        return dialog;
    }
}
