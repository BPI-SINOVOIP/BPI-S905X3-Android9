package js.kbars;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnSystemUiVisibilityChangeListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class FitSystemWindowsActivity extends Activity {
    private static final int SYSTEM_UI_CLEARABLE_FLAGS = 7;
    private static final int SYSTEM_UI_FLAG_ALLOW_TRANSIENT = 2048;
    private static final int SYSTEM_UI_FLAG_TRANSPARENT_NAVIGATION = 8192;
    private static final int SYSTEM_UI_FLAG_TRANSPARENT_STATUS = 4096;
    private final Context mContext = this;
    private FlagStateTextView mFlagStateTextView;
    private final Handler mHandler = new Handler();
    private LinearLayout mLayout;
    private final Runnable mUpdateText = new Runnable() {
        public void run() {
            FitSystemWindowsActivity.this.mFlagStateTextView.updateText();
        }
    };

    private static class Flag implements Comparable<Flag> {
        public static Flag[] ALL = find();
        public String caption;
        public int value;

        private Flag() {
        }

        private static Flag[] find() {
            List<Flag> flags = new ArrayList();
            String prefix = "SYSTEM_UI_FLAG_";
            for (Field f : View.class.getFields()) {
                if (f.getName().startsWith(prefix)) {
                    Flag flag = new Flag();
                    flag.caption = f.getName().substring(prefix.length()).replace("NAVIGATION", "NAV").replace("TRANSPARENT", "TRANSP").replace("LAYOUT", "LAY");
                    try {
                        flag.value = f.getInt(null);
                        if (flag.value != 0) {
                            flags.add(flag);
                        }
                    } catch (Throwable t) {
                        RuntimeException runtimeException = new RuntimeException(t);
                    }
                }
            }
            Collections.sort(flags);
            return (Flag[]) flags.toArray(new Flag[flags.size()]);
        }

        public int compareTo(Flag another) {
            if (this.value < another.value) {
                return -1;
            }
            return this.value > another.value ? 1 : 0;
        }
    }

    private class FlagCheckBox extends CheckBox {
        private final Flag mFlag;

        public FlagCheckBox(Context context, Flag flag) {
            super(context);
            setText(flag.caption);
            this.mFlag = flag;
        }
    }

    private class FlagSetButton extends Button {
        public FlagSetButton(Context context) {
            super(context);
            setText("Set visibility");
            setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    FlagSetButton.this.set();
                }
            });
        }

        private void set() {
            ViewGroup parent = (ViewGroup) getParent();
            int n = parent.getChildCount();
            int vis = 0;
            for (int i = 0; i < n; i++) {
                View v = parent.getChildAt(i);
                if (v instanceof FlagCheckBox) {
                    FlagCheckBox cb = (FlagCheckBox) v;
                    if (cb.isChecked()) {
                        vis |= cb.mFlag.value;
                    }
                }
            }
            FitSystemWindowsActivity.this.update(vis);
        }
    }

    private class FlagStateTextView extends TextView {
        public FlagStateTextView(Context context) {
            super(context);
            updateText();
            setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    FlagStateTextView.this.updateText();
                }
            });
        }

        private void updateText() {
            StringBuilder sb = new StringBuilder();
            append(sb, "vis", FitSystemWindowsActivity.this.mLayout.getSystemUiVisibility());
            append(sb, "win", getWindowSystemUiVisibility());
            setText(sb.toString());
        }

        private void append(StringBuilder sb, String caption, int vis) {
            if (sb.length() > 0) {
                sb.append('\n');
            }
            sb.append(caption).append(':');
            boolean first = true;
            for (Flag flag : Flag.ALL) {
                if ((flag.value & vis) != 0) {
                    if (first) {
                        first = false;
                    } else {
                        sb.append(", ");
                    }
                    sb.append(flag.caption);
                }
            }
        }
    }

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        this.mLayout = new LinearLayout(this.mContext) {
            protected boolean fitSystemWindows(Rect insets) {
                Rect insetsBefore = new Rect(insets);
                String msg = String.format("before=%s\nrt=%s\nafter=%s", new Object[]{insetsBefore.toShortString(), Boolean.valueOf(super.fitSystemWindows(insets)), insets.toShortString()});
                return super.fitSystemWindows(insets);
            }
        };
        this.mLayout.setOrientation(1);
        for (Flag flag : Flag.ALL) {
            this.mLayout.addView(new FlagCheckBox(this.mContext, flag));
        }
        this.mLayout.addView(new FlagSetButton(this.mContext));
        LinearLayout linearLayout = this.mLayout;
        View flagStateTextView = new FlagStateTextView(this.mContext);
        this.mFlagStateTextView = (FlagStateTextView) flagStateTextView;
        linearLayout.addView(flagStateTextView);
        this.mLayout.setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int vis) {
                FitSystemWindowsActivity.this.updateTextDelayed();
            }
        });
        this.mLayout.setPadding(0, 146, 0, 0);
        setContentView(this.mLayout);
    }

    private void update(int vis) {
        this.mLayout.setSystemUiVisibility(vis);
        updateTextDelayed();
    }

    private void updateTextDelayed() {
        this.mHandler.removeCallbacks(this.mUpdateText);
        this.mHandler.postDelayed(this.mUpdateText, 500);
    }
}
