package com.droidlogic.fragment.dialog;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.EditText;
import android.widget.ProgressBar;
import android.widget.Spinner;
import android.widget.TextView;

//import com.droidlogic.fragment.R;

import java.util.LinkedList;
import java.util.regex.Pattern;

import org.dtvkit.inputsource.R;

public class DialogItemAdapter extends BaseAdapter {

    private LinkedList<DialogItemDetail> mData;
    private Context mContext;

    public DialogItemAdapter(LinkedList<DialogItemDetail> mData, Context mContext) {
        this.mData = mData;
        this.mContext = mContext;
    }

    public void reFill(LinkedList<DialogItemDetail> data) {
        mData.clear();
        mData.addAll(data);
        notifyDataSetChanged();
    }

    @Override
    public int getCount() {
        return mData.size();
    }

    @Override
    public Object getItem(int position) {
        return null;
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        ViewHolder holder = null;
        if (convertView == null) {
            convertView = LayoutInflater.from(mContext).inflate(R.layout.dialog_list_item,parent,false);
            holder = new ViewHolder();
            holder.selectText = (TextView) convertView.findViewById(R.id.select_text);
            holder.selectText = (TextView) convertView.findViewById(R.id.select_text);
            holder.titleText = (TextView) convertView.findViewById(R.id.title_text);
            holder.leftarrayText = (TextView) convertView.findViewById(R.id.left_array_text);
            holder.progressBar = (ProgressBar) convertView.findViewById(R.id.progressBar);
            holder.parameterText = (TextView) convertView.findViewById(R.id.parameter_text);
            holder.editText = (EditText) convertView.findViewById(R.id.editText);
            holder.rightarrayText = (TextView) convertView.findViewById(R.id.right_array_text);
            convertView.setTag(holder);
        } else{
            holder = (ViewHolder) convertView.getTag();
        }

        if (mData.get(position).getSelectStatus()) {
            holder.selectText.setVisibility(View.VISIBLE);
        } else {
            holder.selectText.setVisibility(View.INVISIBLE);
        }
        holder.titleText.setText(mData.get(position).getTitleText());
        if (mData.get(position).getLeftArrayStatus()) {
            holder.leftarrayText.setVisibility(View.VISIBLE);
        } else {
            holder.leftarrayText.setVisibility(View.INVISIBLE);
        }
        if (mData.get(position).getProgress() > -1) {
            holder.progressBar.setProgress(mData.get(position).getProgress());
            holder.progressBar.setVisibility(View.VISIBLE);
        } else {
            holder.progressBar.setVisibility(View.GONE);
        }
        if (mData.get(position).getEditable()) {
            holder.editText.setText(mData.get(position).getParameter());
            holder.editText.setVisibility(View.VISIBLE);
            holder.parameterText.setVisibility(View.GONE);
        } else {
            holder.parameterText.setText(mData.get(position).getParameter());
            holder.parameterText.setVisibility(View.VISIBLE);
            holder.editText.setVisibility(View.GONE);
        }
        if (mData.get(position).getRightArrayStatus()) {
            holder.rightarrayText.setVisibility(View.VISIBLE);
        } else {
            holder.rightarrayText.setVisibility(View.INVISIBLE);
        }
        return convertView;
    }

    public static class DialogItemDetail {
        private boolean mSelectStatus = false;
        private String mTitle = null;
        private boolean mLeftArrayStatus = false;
        private String mParameter = null;
        private boolean mRightArrayStatus = false;
        private int mProgress = -1;
        private boolean mEditable = false;

        public static final int ITEM_DISPLAY = 0;
        public static final int ITEM_SELECT = 1;
        public static final int ITEM_EDIT = 2;
        public static final int ITEM_EDIT_SWITCH = 3;
        public static final int ITEM_PROGRESS = 4;

        public DialogItemDetail() {

        }

        public DialogItemDetail(int type, String title, String parameter, boolean isSelect) {
            this.mTitle = title;
            this.mParameter = parameter;
            initByType(type, parameter, isSelect);
        }

        private void initByType(int type, String parameter, boolean isSelect) {
            switch (type) {
                case ITEM_DISPLAY:
                    mSelectStatus = false;
                    mLeftArrayStatus = false;
                    mRightArrayStatus = false;
                    mProgress = -1;
                    mEditable = false;
                    break;
                case ITEM_SELECT:
                    mSelectStatus = isSelect;
                    mLeftArrayStatus = false;
                    mRightArrayStatus = false;
                    mProgress = -1;
                    mEditable = false;
                    break;
                case ITEM_EDIT:
                    mSelectStatus = false;
                    mLeftArrayStatus = false;
                    mRightArrayStatus = false;
                    mProgress = -1;
                    mEditable = true;
                    break;
                case ITEM_EDIT_SWITCH:
                    mSelectStatus = false;
                    mLeftArrayStatus = true;
                    mRightArrayStatus = true;
                    mProgress = -1;
                    mEditable = false;
                    break;
                case ITEM_PROGRESS:
                    mSelectStatus = false;
                    mLeftArrayStatus = false;
                    mRightArrayStatus = false;
                    mProgress = 0;
                    mEditable = false;
                    if (parameter != null) {
                        String[] result = parameter.split("%");
                        if (result != null && isNumber(result[0])) {
                            mProgress = Integer.parseInt(result[0]);
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        public boolean isNumber(String s) {
            Pattern pattern = Pattern.compile("^[0-9]*");
            return pattern.matcher(s).matches();
        }

        public boolean getSelectStatus() {
            return mSelectStatus;
        }
        public void setSelectStatus(boolean selectStatus) {
            mSelectStatus = selectStatus;
        }
        public String getTitleText() {
            return mTitle;
        }

        public void setTitleText(String title) {
            mTitle = title;
        }

        public boolean getLeftArrayStatus() {
            return mLeftArrayStatus;
        }

        public void setLeftArrayStatus(boolean leftArrayStatus) {
            mLeftArrayStatus = leftArrayStatus;
        }

        public String getParameter() {
            return mParameter;
        }

        public void setParameter(String parameter) {
            mParameter = parameter;
        }

        public boolean getRightArrayStatus() {
            return mRightArrayStatus;
        }

        public void setRightArrayStatus(boolean rightArrayStatus) {
            mRightArrayStatus = rightArrayStatus;
        }

        public int getProgress() {
            return mProgress;
        }

        public void setProgress(int progress) {
            mProgress = progress;
        }

        public boolean getEditable() {
            return mEditable;
        }

        public void setEditable(boolean editable) {
            mEditable = editable;
        }
    }

    public class ViewHolder{
        TextView selectText;
        TextView titleText;
        TextView leftarrayText;
        ProgressBar progressBar;
        TextView parameterText;
        EditText editText;
        TextView rightarrayText;
    }
}