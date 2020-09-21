package com.droidlogic.droidlivetv.favlistsettings;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.List;

import com.droidlogic.droidlivetv.R;

public class CustomedDialogView {
    private static final String TAG = CustomedDialogView.class.getSimpleName();
    private static final boolean DEBUG = true;

    private Context mContext;
    private DialogCallback mDialogCallback;

    public static final String DIALOG_ACTION = "dialog_action";
    public static final String DIALOG_EVENT = "dialog_event";
    public static final String DIALOG_ACTION_FLAG = "dialog_action_flag";
    public static final String DIALOG_LIST_POSITION = "list_position";
    public static final String DIALOG_ACTION_SORT_LIST_CLICKED = "sort_list_clicked";
    public static final String DIALOG_ACTION_EXIT = "exit";
    public static final String DIALOG_ACTION_SEARCH_BUTTON_CLICKED = "search_button_clicked";
    public static final String DIALOG_ACTION_SEARCH_CONTENT_CHANGED = "search_content_changed";
    public static final String DIALOG_ACTION_SEARCH_VALUE = "search_channel_value";
    public static final String DIALOG_ACTION_EDIT_FAV_CLICKED = "edit_fav_clicked";
    public static final String DIALOG_ACTION_EDIT_FAV_OK_CLICKED = "edit_fav_ok_clicked";
    public static final String DIALOG_ACTION_FAV_EDIT_VALUE = "fav_edit_value";
    public static final String DIALOG_ACTION_FAV_PREVIOUS_VALUE = "fav_edit_previous_value";

    public static final int[] SORT_ITEM = {R.string.sort_a_z, R.string.sort_z_a};
    public static final int FLAG_SORT_ITEM_AZ = 0;
    public static final int FLAG_SORT_ITEM_ZA = 1;
    public static final int[] EDIT_FAV_ITEM = {R.string.edit_fav_list_add, R.string.edit_fav_list_del, R.string.edit_fav_list_edit};
    public static final int FLAG_EDIT_FAV_ADD = 0;
    public static final int FLAG_EDIT_FAV_DEL = 1;
    public static final int FLAG_EDIT_FAV_EDIT = 2;

    public CustomedDialogView(Context context, DialogCallback dialogCallback) {
        this.mContext = context;
        this.mDialogCallback = dialogCallback;
    }

    public AlertDialog creatSortOrderDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        final AlertDialog alertDialog = builder.create();
        WindowManager.LayoutParams params = alertDialog.getWindow().getAttributes();
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;;
        params.width = WindowManager.LayoutParams.WRAP_CONTENT;
        alertDialog.getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);

        List<ListItem> itemList = new ArrayList<ListItem>();
        ListItem item = null;
        for (int i = 0; i < SORT_ITEM.length; i++) {
            item = new ListItem(mContext.getString(SORT_ITEM[i]));
            itemList.add(item);
        }
        View dialogView = View.inflate(mContext, R.layout.dialog_list, null);
        MyAdapter myAdapter = new MyAdapter(mContext,itemList);
        ListView list = dialogView.findViewById(R.id.dialog_listview);
        list.setAdapter(myAdapter);
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                if (mDialogCallback != null) {
                    alertDialog.dismiss();
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_SORT_LIST_CLICKED);
                    bundle.putInt(DIALOG_LIST_POSITION, position);
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        alertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_EXIT);
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        alertDialog.setView(dialogView);
        return alertDialog;
    }

    public AlertDialog creatSeachChannelDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        final AlertDialog alertDialog = builder.create();
        WindowManager.LayoutParams params = alertDialog.getWindow().getAttributes();
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;;
        params.width = WindowManager.LayoutParams.WRAP_CONTENT;
        alertDialog.getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);

        View dialogView = View.inflate(mContext, R.layout.dialog_search_channel, null);
        final EditText edit = dialogView.findViewById(R.id.search_channel_edittext);
        final Button search = dialogView.findViewById(R.id.search_channel_ok);
        edit.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                Log.d(TAG, "beforeTextChanged s = " + s + ", start = " + start + ", count = " + count + ", after = " + after);
            }

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {
                Log.d(TAG, "onTextChanged s = " + s + ", start = " + start + ", before = " + before + ", count = " + count);
            }

            @Override
            public void afterTextChanged(Editable s) {
                Log.d(TAG, "afterTextChanged s = " + s);
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_SEARCH_CONTENT_CHANGED);
                    Editable editable = edit.getText();
                    bundle.putString(DIALOG_ACTION_SEARCH_VALUE, !TextUtils.isEmpty(editable) ? editable.toString() : "");
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        search.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_SEARCH_BUTTON_CLICKED);
                    Editable editable = edit.getText();
                    bundle.putString(DIALOG_ACTION_SEARCH_VALUE, !TextUtils.isEmpty(editable) ? editable.toString() : "");
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        alertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_EXIT);
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        alertDialog.setView(dialogView);
        return alertDialog;
    }

    public AlertDialog creatEditFavDialog(final FavListItem favItem) {
        AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
        final AlertDialog alertDialog = builder.create();
        WindowManager.LayoutParams params = alertDialog.getWindow().getAttributes();
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;;
        params.width = WindowManager.LayoutParams.WRAP_CONTENT;
        alertDialog.getWindow().setAttributes((android.view.WindowManager.LayoutParams) params);

        List<ListItem> itemList = new ArrayList<ListItem>();
        ListItem item = null;
        for (int i = 0; i < EDIT_FAV_ITEM.length; i++) {
            item = new ListItem(mContext.getString(EDIT_FAV_ITEM[i]));
            itemList.add(item);
        }
        View dialogView = View.inflate(mContext, R.layout.dialog_list, null);
        final TextView title = dialogView.findViewById(R.id.dialog_list_title);
        final TextView info = dialogView.findViewById(R.id.display_fav_info);
        final EditText edit = dialogView.findViewById(R.id.rename_fav_edit);
        final Button rename = dialogView.findViewById(R.id.button_rename);
        final ListView list = dialogView.findViewById(R.id.dialog_listview);
        final LinearLayout container = dialogView.findViewById(R.id.dialog_list_editname_container);
        final MyAdapter myAdapter = new MyAdapter(mContext,itemList);
        final int[] FLAG = {FLAG_EDIT_FAV_ADD};

        title.setVisibility(View.VISIBLE);
        title.setText(R.string.edit_fav_list);
        if (container.getVisibility() != View.VISIBLE) {
            container.setVisibility(View.VISIBLE);
        }
        if (info.getVisibility() != View.VISIBLE) {
            info.setVisibility(View.VISIBLE);
        }
        if (edit.getVisibility() != View.VISIBLE) {
            edit.setVisibility(View.VISIBLE);
        }
        if (rename.getVisibility() != View.VISIBLE) {
            rename.setVisibility(View.VISIBLE);
        }
        info.setText(R.string.edit_fav_list_add_description);
        edit.setText("");
        edit.setHint(R.string.edit_fav_list_add_type_in_description);
        list.setAdapter(myAdapter);

        list.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                if (DEBUG) {
                    Log.d(TAG, "onItemSelected view = " + view + ", position = " + position);
                }
                switch (position) {
                    case FLAG_EDIT_FAV_ADD:
                        FLAG[0] = FLAG_EDIT_FAV_ADD;
                        if (container.getVisibility() != View.VISIBLE) {
                            container.setVisibility(View.VISIBLE);
                        }
                        if (info.getVisibility() != View.VISIBLE) {
                            info.setVisibility(View.VISIBLE);
                        }
                        if (edit.getVisibility() != View.VISIBLE) {
                            edit.setVisibility(View.VISIBLE);
                        }
                        if (rename.getVisibility() != View.VISIBLE) {
                            rename.setVisibility(View.VISIBLE);
                        }
                        info.setText(R.string.edit_fav_list_add_description);
                        edit.setText("");
                        edit.setHint(R.string.edit_fav_list_add_type_in_description);
                        break;
                    case FLAG_EDIT_FAV_DEL:{
                        FLAG[0] = FLAG_EDIT_FAV_DEL;
                        if (container.getVisibility() != View.VISIBLE) {
                            container.setVisibility(View.VISIBLE);
                        }
                        if (info.getVisibility() != View.VISIBLE) {
                            info.setVisibility(View.VISIBLE);
                        }
                        if (edit.getVisibility() == View.VISIBLE) {
                            edit.setVisibility(View.GONE);
                        }
                        if (rename.getVisibility() != View.VISIBLE) {
                            rename.setVisibility(View.VISIBLE);
                        }
                        String temp = mContext.getResources().getString(R.string.edit_fav_list_del_description);
                        String timeTip = String.format(temp, (favItem != null ? favItem.getTitle() : ""));
                        info.setText(timeTip);
                        edit.setText("");
                        edit.setHint("");
                        break;
                    }
                    case FLAG_EDIT_FAV_EDIT:{
                        FLAG[0] = FLAG_EDIT_FAV_EDIT;
                        if (container.getVisibility() != View.VISIBLE) {
                            container.setVisibility(View.VISIBLE);
                        }
                        if (info.getVisibility() != View.VISIBLE) {
                            info.setVisibility(View.VISIBLE);
                        }
                        if (edit.getVisibility() != View.VISIBLE) {
                            edit.setVisibility(View.VISIBLE);
                        }
                        if (rename.getVisibility() != View.VISIBLE) {
                            rename.setVisibility(View.VISIBLE);
                        }

                        String temp = mContext.getResources().getString(R.string.edit_fav_list_edit_description);
                        String timeTip = String.format(temp, (favItem != null ? favItem.getTitle() : ""));
                        info.setText(timeTip);
                        edit.setText("");
                        edit.setHint(R.string.edit_fav_list_add_type_in_description);
                        break;
                    }
                    default:
                        FLAG[0] = FLAG_EDIT_FAV_EDIT;
                        break;
                }
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });
        rename.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_EDIT_FAV_OK_CLICKED);
                    bundle.putInt(DIALOG_ACTION_FLAG, FLAG[0]);
                    Editable editable = edit.getText();
                    bundle.putString(DIALOG_ACTION_FAV_EDIT_VALUE, !TextUtils.isEmpty(editable) ? editable.toString() : "");
                    bundle.putString(DIALOG_ACTION_FAV_PREVIOUS_VALUE, (favItem != null ? favItem.getTitle() : ""));
                    ChannelDataManager channelDataManager = new ChannelDataManager(mContext);
                    edit.setText("");
                    if (FLAG[0] == FLAG_EDIT_FAV_ADD) {
                        if (channelDataManager.getFavGroupPageCount() >= 64) {
                            Log.d(TAG, "rename onClick add limit is 64");
                            Toast.makeText(mContext,R.string.edit_fav_list_name_limit, Toast.LENGTH_SHORT);
                            edit.setHint(R.string.edit_fav_list_name_limit);
                            return;
                        } else if (editable == null || (editable != null && TextUtils.isEmpty(editable.toString()))) {
                            Toast.makeText(mContext,R.string.edit_fav_list_name_invalid, Toast.LENGTH_SHORT);
                            edit.setHint(R.string.edit_fav_list_name_invalid);
                            Log.d(TAG, "rename onClick add invalid fav page");
                            return;
                        } else if (channelDataManager.isFavGroupExist(editable != null ? editable.toString() : null)) {
                            Toast.makeText(mContext,R.string.edit_fav_list_name_exist, Toast.LENGTH_SHORT);
                            edit.setHint(R.string.edit_fav_list_name_exist);
                            Log.d(TAG, "rename onClick add fav page exist");
                            return;
                        }
                    } else if (FLAG[0] == FLAG_EDIT_FAV_DEL) {
                        Log.d(TAG, "rename onClick del fav page");
                    } else if (FLAG[0] == FLAG_EDIT_FAV_EDIT) {
                        if (editable == null || (editable != null && TextUtils.isEmpty(editable.toString()))) {
                            Toast.makeText(mContext,R.string.edit_fav_list_name_invalid, Toast.LENGTH_SHORT);
                            edit.setHint(R.string.edit_fav_list_name_invalid);
                            Log.d(TAG, "rename onClick edit invalid fav page");
                            return;
                        } else if (channelDataManager.isFavGroupExist(editable != null ? editable.toString() : null)) {
                            Toast.makeText(mContext,R.string.edit_fav_list_name_exist, Toast.LENGTH_SHORT);
                            edit.setHint(R.string.edit_fav_list_name_exist);
                            Log.d(TAG, "rename onClick edit fav page exist");
                            return;
                        }
                    }
                    mDialogCallback.onDialogEvent(bundle);
                    alertDialog.dismiss();
                }
            }
        });
        alertDialog.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (mDialogCallback != null) {
                    Bundle bundle = new Bundle();
                    bundle.putBoolean(DIALOG_ACTION, true);
                    bundle.putString(DIALOG_EVENT, DIALOG_ACTION_EXIT);
                    mDialogCallback.onDialogEvent(bundle);
                }
            }
        });
        alertDialog.setView(dialogView);
        return alertDialog;
    }

    public void setDialogCallback(DialogCallback dialogCallback) {
        this.mDialogCallback = dialogCallback;
    }

    public interface DialogCallback {
        void onDialogEvent(Bundle bundle);
    }

    public class MyAdapter extends BaseAdapter {
        private List<ListItem> mList;
        private Context mAdapterContext;

        public MyAdapter(Context context, List<ListItem> list) {
            this.mList = list;
            this.mAdapterContext = context;
        }

        @Override
        public int getCount() {
            return mList.size();
        }

        @Override
        public Object getItem(int position) {
            return mList.get(position);
        }

        public long getItemId(int position) {
            return position;
        }

        public View getView(int position, View convertView, ViewGroup parent) {
            ListViewHolder holder = null;
            if (convertView == null) {
                convertView = LayoutInflater.from(mAdapterContext).inflate(R.layout.dialog_list_item,parent,false);
                holder = new ListViewHolder();
                holder.mListTextView = (TextView) convertView.findViewById(R.id.list_item_text);
                convertView.setTag(holder);
            } else {
                holder = (ListViewHolder) convertView.getTag();
            }
            holder.mListTextView.setText(mList.get(position).getTitle());
            return convertView;
        }
    }

    public class ListItem {

        private String mTitle = null;

        public ListItem(String title){
            this.mTitle = title;
        }

        public String getTitle() {
            return mTitle;
        }
    }

    public class ListViewHolder {
        private TextView mListTextView;

        public ListViewHolder(){

        }

        public void setListTextView(TextView textView) {
            mListTextView = textView;
        }

        public TextView getListTextView() {
            return mListTextView;
        }
    }
}
