package com.droidlogic.fragment.dialog;

import android.app.AlertDialog;
import android.content.Context;
import android.util.Log;
import android.widget.AdapterView;
import android.text.TextUtils;

import com.droidlogic.fragment.ParameterMananer;

public class DialogManager {

    private static final String TAG = "DialogManager";
    private String mCurrentType;
    private ParameterMananer mParameterMananer;
    private Context mContext;

    public DialogManager(Context context, ParameterMananer mananer) {
        this.mContext = context;
        this.mParameterMananer = mananer;
    }

    public void setCurrentType(String type) {
        mCurrentType = type;
    }

    public String getCurrentType() {
        return mCurrentType;
    }

    public CustomDialog buildItemDialogById(int id, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_SELECT_SINGLE_ITEM, callBack, mParameterMananer);
        if (id > CustomDialog.ID_DIALOG_TITLE_COLLECTOR.length - 1 || id > ParameterMananer.ID_DIALOG_KEY_COLLECTOR.length - 1) {
            return null;
        }
        if (TextUtils.equals(ParameterMananer.ID_DIALOG_KEY_COLLECTOR[id], ParameterMananer.KEY_UNICABLE_SWITCH)) {
            customDialog.initListView(mContext.getString(CustomDialog.ID_DIALOG_TITLE_COLLECTOR[id]), ParameterMananer.KEY_UNICABLE, mParameterMananer.getIntParameters(ParameterMananer.ID_DIALOG_KEY_COLLECTOR[id]));
        } else {
            customDialog.initListView(mContext.getString(CustomDialog.ID_DIALOG_TITLE_COLLECTOR[id]), ParameterMananer.ID_DIALOG_KEY_COLLECTOR[id], mParameterMananer.getIntParameters(ParameterMananer.ID_DIALOG_KEY_COLLECTOR[id]));
        }
        return customDialog;
    }

    public CustomDialog buildLnbCustomedItemDialog(DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_ITEM, callBack, mParameterMananer);
        customDialog.initLnbCustomedItemDialog();
        return customDialog;
    }

    /*public CustomDialog buildUnicableCustomedItemDialog(DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_ITEM, callBack, mParameterMananer);
        customDialog.initUnicableCustomedItemDialog();
        return customDialog;
    }*/

    public CustomDialog buildDiseqc1_2_ItemDialog(DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM, callBack, mParameterMananer);
        customDialog.initDiseqc1_2_ItemDialog();
        return customDialog;
    }

    public CustomDialog buildAddTransponderDialogDialog(String parameter, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM, callBack, mParameterMananer);
        customDialog.initAddTransponderDialog(parameter);
        return customDialog;
    }

    public CustomDialog buildRemoveTransponderDialogDialog(String parameter, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM, callBack, mParameterMananer);
        customDialog.initRemoveTransponderDialog(parameter);
        return customDialog;
    }

    public CustomDialog buildAddSatelliteDialogDialog(String name, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM, callBack, mParameterMananer);
        customDialog.initAddSatelliteDialog(name);
        return customDialog;
    }

    public CustomDialog buildRemoveSatelliteDialogDialog(String name, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_EDIT_SWITCH_ITEM, callBack, mParameterMananer);
        customDialog.initRemoveSatelliteDialog(name);
        return customDialog;
    }
    /*public AlertDialog buildItemDialogByKey(String key, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_SELECT_SINGLE_ITEM, callBack, mParameterMananer);
        AlertDialog selectSingleItemDialog = customDialog.creatSelectSingleItemDialog(null, key, mParameterMananer.getIntParameters(key));
        return selectSingleItemDialog;
    }*/

    /*public AlertDialog buildSelectSingleItemDialog(String title, DialogCallBack callBack) {
        CustomDialog customDialog = new CustomDialog(mContext, CustomDialog.DIALOG_SET_SELECT_SINGLE_ITEM, callBack, mParameterMananer);
        AlertDialog selectSingleItemDialog = customDialog.creatSelectSingleItemDialog(title, null, mParameterMananer.getSelectSingleItemValueIndex(title));
        return selectSingleItemDialog;
    }*/
}
