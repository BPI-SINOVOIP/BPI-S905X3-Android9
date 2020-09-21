package js.kbars;

import android.animation.Animator;
import android.animation.Animator.AnimatorListener;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.util.DisplayMetrics;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MenuItem.OnMenuItemClickListener;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout.LayoutParams;
import android.widget.LinearLayout;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.List;

public class KBarsActivity extends Activity {
    public static final int BASE_FLAGS = 1792;
    static final boolean DEBUG = true;
    public static final int FLAG_TRANSLUCENT_NAVIGATION = 134217728;
    public static final int FLAG_TRANSLUCENT_STATUS = 67108864;
    private static final int IMMERSIVE_FLAGS = 3846;
    private static final int IMMERSIVE_FLAGS_STICKY = 5894;
    private static final int SYSTEM_UI_FLAG_IMMERSIVE = 2048;
    private static final int SYSTEM_UI_FLAG_IMMERSIVE_STICKY = 4096;
    static final String TAG = Util.logTag(KBarsActivity.class);
    private List<View> mButtons = new ArrayList();
    private CameraBackgroundMenuItem mCameraBackgroundMenuItem;
    private final Context mContext = this;
    private EditText mEditText;
    private TouchTrackingLayout mFrame;
    private ImageBackgroundMenuItem mImageBackgroundMenuItem;

    private static final class DebugButton extends Button {
        private final Handler mHandler = new Handler();
        private final Runnable mNavTransOff = new Runnable() {
            public void run() {
                DebugButton.this.setNavTrans(false);
            }
        };
        private final Runnable mNavTransOn = new Runnable() {
            public void run() {
                DebugButton.this.setNavTrans(true);
                DebugButton.this.cancelTrans();
                DebugButton.this.mHandler.postDelayed(DebugButton.this.mNavTransOff, 5000);
            }
        };

        public DebugButton(Context context) {
            super(context);
            setText("Debug");
            setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    DebugButton.this.fallingToast();
                }
            });
        }

        private void cancelTrans() {
            this.mHandler.removeCallbacks(this.mNavTransOn);
            this.mHandler.removeCallbacks(this.mNavTransOff);
        }

        private void setNavTrans(boolean trans) {
            Window w = ((Activity) getContext()).getWindow();
            if (trans) {
                w.addFlags(KBarsActivity.FLAG_TRANSLUCENT_STATUS);
            } else {
                w.clearFlags(KBarsActivity.FLAG_TRANSLUCENT_STATUS);
            }
        }

        public void fallingToast() {
            this.mNavTransOff.run();
            Toast.makeText(getContext(), "Here is a toast", 1).show();
            cancelTrans();
            this.mHandler.postDelayed(this.mNavTransOn, 1000);
        }
    }

    private static final class InvisibleButton extends Button {
        public InvisibleButton(Context context) {
            super(context);
            super.setVisibility(4);
        }

        public void setVisibility(int visibility) {
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        boolean portrait;
        int i = 0;
        super.onCreate(savedInstanceState);
        getWindow().requestFeature(9);
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getRealMetrics(dm);
        Toast.makeText(this.mContext, dm.widthPixels + "x" + dm.heightPixels + " supported=" + areTranslucentBarsSupported(), 0).show();
        getActionBar().setTitle("kbars " + Util.getVersionName(this.mContext));
        getActionBar().setBackgroundDrawable(new ColorDrawable(0));
        this.mFrame = new TouchTrackingLayout(this.mContext);
        LinearLayout allButtons = new LinearLayout(this.mContext);
        Configuration config = getResources().getConfiguration();
        if (((float) config.screenWidthDp) / ((float) config.screenHeightDp) < 1.0f) {
            portrait = true;
        } else {
            portrait = false;
        }
        if (portrait) {
            i = 1;
        }
        allButtons.setOrientation(i);
        LinearLayout buttons1 = new LinearLayout(this.mContext);
        buttons1.setOrientation(1);
        LinearLayout buttons2 = new LinearLayout(this.mContext);
        buttons2.setOrientation(1);
        allButtons.addView(buttons1);
        allButtons.addView(buttons2);
        LayoutParams buttonsLP = new LayoutParams(-2, -2);
        buttonsLP.gravity = 17;
        this.mFrame.addView(allButtons, buttonsLP);
        this.mEditText = new EditText(this.mContext);
        this.mEditText.setVisibility(8);
        buttons1.addView(this.mEditText, -1, -2);
        buttons1.addView(new TransparencyToggleButton(this.mContext, "status bar", FLAG_TRANSLUCENT_STATUS));
        buttons1.addView(new TransparencyToggleButton(this.mContext, "navigation bar", FLAG_TRANSLUCENT_NAVIGATION));
        buttons1.addView(new InvisibleButton(this.mContext));
        buttons2.addView(new ImmersiveModeToggleButton(this, "immersive", IMMERSIVE_FLAGS));
        buttons2.addView(new ImmersiveModeToggleButton(this, "sticky immersive", IMMERSIVE_FLAGS_STICKY));
        buttons2.addView(new LightsOutModeToggleButton(this.mContext));
        buttons2.addView(new MediaModeToggleButton(this.mContext, this.mFrame));
        buttons2.addView(new InvisibleButton(this.mContext));
        setContentView(this.mFrame);
        addButtons(buttons1);
        addButtons(buttons2);
    }

    private void addButtons(LinearLayout buttons) {
        for (int i = 0; i < buttons.getChildCount(); i++) {
            this.mButtons.add(buttons.getChildAt(i));
        }
    }

    private boolean areTranslucentBarsSupported() {
        int id = getResources().getIdentifier("config_enableTranslucentDecor", "bool", "android");
        if (id == 0) {
            return false;
        }
        return getResources().getBoolean(id);
    }

    public boolean onCreateOptionsMenu(Menu menu) {
        RandomColorBackgroundMenuItem randomColorBackgroundMenuItem = new RandomColorBackgroundMenuItem(menu, this.mFrame);
        this.mImageBackgroundMenuItem = new ImageBackgroundMenuItem(menu, this, this.mFrame);
        this.mCameraBackgroundMenuItem = new CameraBackgroundMenuItem(menu, this, this.mFrame);
        createDebugMenuItem(menu);
        createEditTextMenuItem(menu);
        createOrientationMenuItem(menu, R.string.action_portrait, 1);
        createOrientationMenuItem(menu, R.string.action_landscape, 0);
        createOrientationMenuItem(menu, R.string.action_reverse_portrait, 9);
        createOrientationMenuItem(menu, R.string.action_reverse_landscape, 8);
        return true;
    }

    private void createOrientationMenuItem(Menu menu, int text, final int orientation) {
        MenuItem mi = menu.add(text);
        mi.setShowAsAction(0);
        mi.setOnMenuItemClickListener(new OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                KBarsActivity.this.setRequestedOrientation(orientation);
                return true;
            }
        });
    }

    private void createDebugMenuItem(Menu menu) {
        final MenuItem mi = menu.add("Show gesture debugging");
        mi.setShowAsActionFlags(0);
        mi.setOnMenuItemClickListener(new OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                boolean debug;
                if (KBarsActivity.this.mFrame.getDebug()) {
                    debug = false;
                } else {
                    debug = true;
                }
                KBarsActivity.this.mFrame.setDebug(debug);
                mi.setTitle(new StringBuilder(String.valueOf(debug ? "Hide" : "Show")).append(" gesture debugging").toString());
                return true;
            }
        });
    }

    private void createEditTextMenuItem(Menu menu) {
        final MenuItem mi = menu.add("Show EditText");
        mi.setShowAsActionFlags(0);
        mi.setOnMenuItemClickListener(new OnMenuItemClickListener() {
            public boolean onMenuItemClick(MenuItem item) {
                boolean isVisible;
                int i = 0;
                if (KBarsActivity.this.mEditText.getVisibility() == 0) {
                    isVisible = true;
                } else {
                    isVisible = false;
                }
                EditText access$1 = KBarsActivity.this.mEditText;
                if (isVisible) {
                    i = 8;
                }
                access$1.setVisibility(i);
                mi.setTitle(new StringBuilder(String.valueOf(isVisible ? "Show" : "Hide")).append(" EditText").toString());
                return true;
            }
        });
    }

    public void setSoloButton(boolean visible, View solo, boolean animate) {
        int i;
        int vis = 0;
        if (visible) {
            i = 1;
        } else {
            i = 0;
        }
        float alpha = (float) i;
        if (!visible) {
            vis = 4;
        }
        final int _vis = vis;
        for (final View view : this.mButtons) {
            if (!(view == solo || (view instanceof EditText))) {
                view.setEnabled(visible);
                if (animate) {
                    view.animate().alpha(alpha).setListener(new AnimatorListener() {
                        public void onAnimationCancel(Animator animation) {
                        }

                        public void onAnimationEnd(Animator animation) {
                            view.setVisibility(_vis);
                        }

                        public void onAnimationRepeat(Animator animation) {
                        }

                        public void onAnimationStart(Animator animation) {
                        }
                    }).start();
                } else {
                    view.setVisibility(vis);
                }
            }
        }
    }

    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == CameraBackgroundMenuItem.REQUEST_CODE) {
            this.mCameraBackgroundMenuItem.onActivityResult(resultCode, data);
        }
        if (requestCode == ImageBackgroundMenuItem.REQUEST_CODE) {
            this.mImageBackgroundMenuItem.onActivityResult(resultCode, data);
        }
    }
}
