package com.droidlogic.app;

import android.content.Context;
import android.graphics.*;
import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.os.Process;
import android.os.RemoteException;
import android.util.AttributeSet;
import android.util.Log;
import android.view.*;
import android.view.WindowManager.LayoutParams;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import java.util.regex.*;

class SubtitleViewAdaptor {
    private static final String TAG = SubtitleViewAdaptor.class.getSimpleName();
    private static final boolean DEBUG_LAYOUT = false;
    private Context mContext;
    private FrameLayout mSubLayout = null;
    private TextView mTextView;
    private ImageView mImageView;
    private CCSubtitleView mCcSutbitlteView;
    boolean mIsWindowCreated;

    private AnimationDrawable mAnimationDrawable;

    //fallback display window media overlay type
    private int TYPE_APPLICATION_MEDIA_OVERLAY = 1004;

    private Display mDisplay;
    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowLayoutParams;

    private float mWscale = 1.000f;
    private float mHscale = 1.000f;
    private int mWmax = 0;
    private int mHmax = 0;
    private int mGravity = 0;
    private int mTextStyle = -1;
    private int mPosHeight = 0;
    private int mTextSize = 0;
    private int mTextColor = 0;
    private int mCordinateX = 0;
    private int mCordinateY = 0;
    private int mDisplayFlag = 0;
    private int mSubtitleType = -1;
    private boolean mDisableDisplay = false;

    private String mTitle;
    // To support setDisplayRect.
    private int mDisplayBoundWidth;
    private int mDisplayBoundHeight;
    /**
     *
     *    WARNING: ALL public method in this class MUST called in main(UI) thread!
     *
     */

    private void checkCallerOnUIThread() {
        if (Process.myPid() != Process.myTid()) {
            Log.d(TAG, "Please call this method in ui thread!");
            Log.d(TAG, "You can simply wrap a function, post runnable to let it call in UI thread:");
            throw new RuntimeException("need call in UI thread!");
        }
    }

    public SubtitleViewAdaptor(Context ctx) {
        checkCallerOnUIThread();
        mContext = ctx;
        mIsWindowCreated = false;
        mWindowManager = (WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE);
        ensureSubLayoutCreated();
    }

    public boolean isDisplayWindowAdded() {
        checkCallerOnUIThread();
        Log.d(TAG, "isDisplayWindowAdded:"+mIsWindowCreated);
        return mIsWindowCreated;
    }

    public void setDisplayFlag(boolean flag) {
        mDisableDisplay = flag;
    }

    private void ensureSubLayoutCreated() {
        if (mSubLayout != null) {
            return;
        }

        // Construct view Hierarchy layout for subtitle.
        mSubLayout = new FrameLayout(mContext);
        FrameLayout.LayoutParams tparams=new FrameLayout.LayoutParams(
            ViewGroup.LayoutParams.FILL_PARENT,
            ViewGroup.LayoutParams.FILL_PARENT);

        Log.d(TAG, "mSubLayout:"+mSubLayout);
        LinearLayout.LayoutParams lparams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT,
            ViewGroup.LayoutParams.WRAP_CONTENT);

        RelativeLayout tlayout = new RelativeLayout(mContext);
        tlayout.setLayoutParams(lparams);
        tlayout.setPadding(0, 0, 0, 50);
        tlayout.setGravity(Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        mTextView = (TextView) new TextView(mContext);
        tlayout.addView(mTextView, lparams);

        RelativeLayout ilayout = new RelativeLayout(mContext);
        ilayout.setLayoutParams(lparams);
        mImageView = new ImageView (mContext);
        ilayout.addView(mImageView, lparams);
        //ilayout.setBackgroundColor(0x7f0000FF);

        RelativeLayout cclayout = new RelativeLayout(mContext);
        ilayout.setLayoutParams(lparams);
        mCcSutbitlteView = new CCSubtitleView(mContext);
        cclayout.setPadding(0, 0, 0, 50);
        cclayout.addView(mCcSutbitlteView, lparams);
        mSubLayout.addView(tlayout, tparams);
        mSubLayout.addView(ilayout, tparams);
        mSubLayout.addView(cclayout, tparams);
        Log.d(TAG, "mSubLayout2:"+mSubLayout);

        mTextView.setVisibility(View.INVISIBLE);
        mImageView.setVisibility(View.INVISIBLE);
        mCcSutbitlteView.hide();

        mDisplay = mWindowManager.getDefaultDisplay();
        mDisplayBoundWidth = mDisplay.getWidth();
        mDisplayBoundHeight = mDisplay.getWidth();
        //mSubLayout.setBackgroundColor(0x60888888);
    }

    private void initialLayoutParams(int windowType, String windowTitle, int x, int y, int w, int h) {
        // Add window for subtitle
        mTitle = windowTitle;
        mWindowLayoutParams = new WindowManager.LayoutParams();
        mWindowLayoutParams.type = windowType;
        mWindowLayoutParams.format = PixelFormat.TRANSLUCENT;
        mWindowLayoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL
                  | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                  | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;
        mWindowLayoutParams.gravity = Gravity.LEFT | Gravity.TOP;
        mWindowLayoutParams.setTitle(windowTitle);
        mWindowLayoutParams.x = x;
        mWindowLayoutParams.y = y;
        mWindowLayoutParams.width = w;
        mWindowLayoutParams.height = h;

        mDisplayBoundWidth = w;
        mDisplayBoundHeight =h;
    }

    public void addSubtitleView(String title) {
        checkCallerOnUIThread();
        if (mIsWindowCreated) return;

        ensureSubLayoutCreated();

        mDisplay = mWindowManager.getDefaultDisplay();
        initialLayoutParams(LayoutParams.TYPE_APPLICATION_PANEL, title, 0, 0, mDisplay.getWidth(), mDisplay.getHeight());

        // Add window for subtitle
        mWindowManager.addView(mSubLayout, mWindowLayoutParams);
        mIsWindowCreated = true;
        Log.d(TAG, "addSubtitleView:"+title);
    }

    public void addSystemSubtitleView(String title) {
        checkCallerOnUIThread();

        if (mIsWindowCreated) return;

        ensureSubLayoutCreated();
        mDisplay = mWindowManager.getDefaultDisplay();
        initialLayoutParams(TYPE_APPLICATION_MEDIA_OVERLAY, title, 0, 0, mDisplay.getWidth(), mDisplay.getHeight());

        // Add window for subtitle
        mWindowManager.addView(mSubLayout, mWindowLayoutParams);
        mIsWindowCreated = true;
        Log.d(TAG, "addSystemSubtitleView:"+title);
    }

    public void removeSubtitleView() {
        Log.d(TAG, "removeSubtitleView");
        checkCallerOnUIThread();

        if (!mIsWindowCreated) {
            return;
        }
        if (View.VISIBLE == mCcSutbitlteView.getVisibility()) {
            mCcSutbitlteView.setVisible(false);
        }
        mWindowManager.removeView(mSubLayout);
        mIsWindowCreated = false;
    }

    public void showSubtitleString(String text, boolean showing) {
        checkCallerOnUIThread();
        if (mDisableDisplay)
            return;
        mTextView.setGravity(mGravity);
        mTextView.setTextAppearance(mTextStyle);
        if (0 == mTextSize) {
            mTextSize = 20;
        }
        if (0 == mTextColor) {
            mTextColor = Color.WHITE;
        }
        mTextView.setTextSize(mTextSize);
        mTextView.setTextColor(mTextColor);

       RelativeLayout.LayoutParams tt = new RelativeLayout.LayoutParams(mTextView.getLayoutParams());
       tt.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
       tt.addRule(RelativeLayout.CENTER_HORIZONTAL);
       mTextView.setLayoutParams(tt);

       mTextView.setGravity(Gravity.CENTER);
       ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mTextView.getLayoutParams();
       params.setMargins(0, 0, 0, mPosHeight);
       mTextView.setLayoutParams(params);

       mImageView.setVisibility(View.INVISIBLE);;
       if (!showing) {
            mTextView.setVisibility(View.INVISIBLE);
            return;
       }

        mTextView.setVisibility(View.VISIBLE);
        mCcSutbitlteView.hide();
        text = text.replace("\\N", "\n");
        Pattern pattern1 = Pattern.compile("(?<=\\{)[^\\}]+");
        Matcher m = pattern1.matcher(text);
        int i =0;
        while (m.find()) {
            text = text.replace("{"+m.group()+"}", "");
        }
        Log.d(TAG, "showText:"+text);
        if (text != null) {
            text = text.replaceAll ("\r", "");
            byte tmpStrByte[] = text.getBytes();

            if (tmpStrByte.length > 0 && 0 == tmpStrByte[tmpStrByte.length - 1]) {
                tmpStrByte[tmpStrByte.length - 1] = ' ';
            }

            if (mTextView != null) {
                mTextView.setVisibility(View.VISIBLE);
                mTextView.setText(new String(tmpStrByte));
                Log.d(TAG, "Layout" + mSubLayout + ", Text:" + mTextView.getText() + ", " + mTextView);
            }
        }
    }

    public void setDisplayType(int type) {
        checkCallerOnUIThread();
        mDisplayFlag = type;
    }

    public void setSubtitleType(int type) {
        mSubtitleType= type;
    }

    public void displayView() {
        checkCallerOnUIThread();
        if (SubtitleManager.SUBTITLE_TXT == mDisplayFlag ) {
            mTextView.setVisibility(View.VISIBLE);
        }
        else if ((SubtitleManager.SUBTITLE_IMAGE == mDisplayFlag) ||
                  (SubtitleManager.SUBTITLE_IMAGE_CENTER == mDisplayFlag)) {
            mImageView.setVisibility(View.VISIBLE);
        }
        else if (SubtitleManager.SUBTITLE_CC_JASON == mDisplayFlag) {
            mCcSutbitlteView.setVisibility(View.VISIBLE);
        }
    }

    public void startTtxLoading(int loadingId) {
        checkCallerOnUIThread();
        if (mAnimationDrawable == null || !mAnimationDrawable.isRunning()) {
            mImageView.setBackground(mContext.getResources().getDrawable(loadingId));
            RelativeLayout.LayoutParams tt = new RelativeLayout.LayoutParams(mImageView.getLayoutParams());
            tt.addRule(RelativeLayout.CENTER_VERTICAL);
            tt.addRule(RelativeLayout.CENTER_HORIZONTAL);
            mImageView.setLayoutParams(tt);

            mAnimationDrawable = (AnimationDrawable) mImageView.getBackground();
            mImageView.setImageBitmap(null);
            mAnimationDrawable.start();
        }
    }

    public void stopTtxLoading() {
        checkCallerOnUIThread();
        Log.d(TAG, "stopTtxLoading");
        mImageView.setBackground(null);
        if (mAnimationDrawable != null && mAnimationDrawable.isRunning()) {
            mAnimationDrawable.stop();
        }
    }


    public void hideView() {
        checkCallerOnUIThread();
        if (SubtitleManager.SUBTITLE_TXT == mDisplayFlag ) {
            mTextView.setVisibility(View.INVISIBLE);
        }
        else if ((SubtitleManager.SUBTITLE_IMAGE == mDisplayFlag) ||
                  (SubtitleManager.SUBTITLE_IMAGE_CENTER == mDisplayFlag)) {
            mImageView.setVisibility(View.INVISIBLE);
        }
        else if (SubtitleManager.SUBTITLE_CC_JASON == mDisplayFlag) {
            mCcSutbitlteView.setVisibility(View.INVISIBLE);
        }
    }

    public void clearContent() {
        checkCallerOnUIThread();
        if (SubtitleManager.SUBTITLE_TXT == mDisplayFlag ) {
            mTextView.setText("");
        }
        else if ((SubtitleManager.SUBTITLE_IMAGE == mDisplayFlag) ||
                  (SubtitleManager.SUBTITLE_IMAGE_CENTER == mDisplayFlag)) {
            mImageView.setImageBitmap(null);
        }
   }
    public void showCaptionClose(String str) {
        checkCallerOnUIThread();
        if (mDisableDisplay)
            return;
        mTextView.setVisibility(View.INVISIBLE);
        mImageView.setVisibility(View.INVISIBLE);
        mCcSutbitlteView.setVisibility(View.VISIBLE);
        mCcSutbitlteView.showJsonStr(str);
    }
    public void resetForSeek() {
        checkCallerOnUIThread();
        // simply: invisible now
        //mTextView.setVisibility(View.INVISIBLE);
        //mImageView.setVisibility(View.INVISIBLE);;
    }

    public void setImgSubRatio(float ratioW, float ratioH, int maxW, int maxH) {
        checkCallerOnUIThread();
        mWscale = ratioW;
        mHscale = ratioH;
        mWmax = maxW;
        mHmax = maxH;
    }
    public void setGravity(int gravity) {
        checkCallerOnUIThread();
        mGravity = gravity;
    }
    public void setTextStype(int style) {
        checkCallerOnUIThread();
        mTextStyle = style;
     }
    public void setPosHeight(int posheight) {
        checkCallerOnUIThread();
       mPosHeight = posheight;
    }
    public void setTextSize(int size ) {
        checkCallerOnUIThread();
        mTextSize = size;
    }
    public void setTextColor(int color) {
        checkCallerOnUIThread();
         mTextColor = color;
    }
    public void setCordinate (int x, int y) {
        mCordinateX = x;
        mCordinateY = y;
      }

    //set the bitmap by scale
    public Bitmap creatBitmapByScale(Bitmap bitmap, float wScale, float hScale, int wmax, int hmax) {
        if (bitmap == null) {
            return null;
        }
        float tempwScale = 1.0f;
        float temphScale = 1.0f;
        int w = bitmap.getWidth();
        int h = bitmap.getHeight();
        Log.d(TAG, "showBitmap:w-"+w+", h-"+h);
        if (SubtitleManager.SUBTITLE_IMAGE == mDisplayFlag) {
            tempwScale = wScale;
            temphScale = hScale;
            // Apply scale for display.
        } else if(SubtitleManager.SUBTITLE_IMAGE_CENTER == mDisplayFlag) {
            tempwScale = ((float)mWindowLayoutParams.width/w) * 0.8f;
            temphScale = ((float)mWindowLayoutParams.height/h) * 0.8f;
        }
        Matrix matrix = new Matrix();
        matrix.postScale(tempwScale, temphScale);
        Log.d(TAG, "showBitmap:matrix-"+matrix+", tempwScale="+tempwScale+", temphScale="+temphScale);
        Bitmap resizedBitmap = Bitmap.createBitmap(bitmap, 0, 0, w, h, matrix, true);
        return resizedBitmap;
    }

    public void showBitmap(Bitmap bitmap, float wScale, float hScale, boolean showing) {
        checkCallerOnUIThread();
        if (mDisableDisplay)
            return;
        mTextView.setVisibility(View.INVISIBLE);

        if (!showing) {
            Log.d(TAG, "hidden!");
            mImageView.setVisibility(View.INVISIBLE);
            return;
        }

        Log.d(TAG, "showBitmap:" + bitmap);

        Bitmap interBitmap = creatBitmapByScale(bitmap, wScale, hScale, mWmax, mHmax);
        ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) mImageView.getLayoutParams();
        android.view.ViewGroup.LayoutParams  layoutParams = mImageView.getLayoutParams();
        if ((mSubtitleType == SubtitleManager.TYPE_SUBTITLE_DVB)
            || (mSubtitleType == SubtitleManager.TYPE_SUBTITLE_SCTE27)
            || (mSubtitleType == SubtitleManager.TYPE_SUBTITLE_PGS)) {
            mCordinateX = (int)(mCordinateX*wScale);
            mCordinateY = (int)(mCordinateY*hScale);
            params.setMargins(mCordinateX, mCordinateY, 0, 0);
            mImageView.setLayoutParams(params);

            Log.d(TAG, "mCordinateX="+mCordinateX+", mCordinateY="+mCordinateY);
        } else {
            RelativeLayout.LayoutParams tt = new RelativeLayout.LayoutParams(mImageView.getLayoutParams());
            if (mSubtitleType == SubtitleManager.TYPE_SUBTITLE_DVB_TELETEXT) {
                stopTtxLoading();
                tt.addRule(RelativeLayout.CENTER_VERTICAL);
            } else {
                tt.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
            }
            tt.addRule(RelativeLayout.CENTER_HORIZONTAL);
            mImageView.setLayoutParams(tt);
        }
        if ( (interBitmap != null) && (mImageView != null) ) {
            mImageView.setImageBitmap(interBitmap);
            mImageView.setVisibility(View.VISIBLE);
            Log.d(TAG, "Layout>>"+mSubLayout+", bitmap:"+interBitmap.getWidth()+", "+mImageView);
        }
        if (DEBUG_LAYOUT) dumpViewHirarchy(mSubLayout);
    }

    public void setSurfaceDisplayRect(int x, int y, int w, int h) {
        checkCallerOnUIThread();

        if (mIsWindowCreated) {
            removeSubtitleView();
        }

        ensureSubLayoutCreated();
        initialLayoutParams(LayoutParams.TYPE_APPLICATION_OVERLAY, mTitle, x, y, w, h);

        // Add window for subtitle
        mWindowManager.addView(mSubLayout, mWindowLayoutParams);
        mIsWindowCreated = true;
        Log.d(TAG, "addSystemSubtitleView:" + mTitle);
        displayView();
    }

    private void dumpViewHirarchy(View view) {
        StringBuilder sb = new StringBuilder("");
        dumpViewHierarchy(sb, "  ", view);
        Log.d(TAG, sb.toString());
    }

    private void dumpViewHierarchy(StringBuilder sb, String prefix, View view) {
        sb.append(prefix);
        if (view == null) {
          sb.append("null");
          return;
        }
        sb.append(view.toString());
        sb.append("\n");
        if (!(view instanceof ViewGroup)) {
          return;
        }
        ViewGroup grp = (ViewGroup)view;
        final int N = grp.getChildCount();
        if (N <= 0) {
          return;
        }
        prefix = prefix + "  ";
        for (int i=0; i<N; i++) {
          dumpViewHierarchy(sb, prefix, grp.getChildAt(i));
        }
    }
}
