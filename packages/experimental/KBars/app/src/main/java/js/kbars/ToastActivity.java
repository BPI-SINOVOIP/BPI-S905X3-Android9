package js.kbars;

import android.app.Activity;
import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnSystemUiVisibilityChangeListener;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

public class ToastActivity extends Activity {
    private static final int ALLOW_TRANSIENT = 2048;
    private static final String TAG = Util.logTag(ToastActivity.class);
    private View mContent;
    private final Context mContext = this;
    boolean mImmersive;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        LinearLayout buttons = new LinearLayout(this.mContext);
        buttons.setOrientation(1);
        for (final Method m : getClass().getDeclaredMethods()) {
            if (Modifier.isPublic(m.getModifiers()) && m.getParameterTypes().length == 0) {
                Button btn = new Button(this.mContext);
                btn.setText(m.getName());
                btn.setOnClickListener(new OnClickListener() {
                    public void onClick(View v) {
                        try {
                            m.invoke(ToastActivity.this.mContext, new Object[0]);
                        } catch (Throwable t) {
                            Log.w(ToastActivity.TAG, "Error running " + m.getName(), t);
                        }
                    }
                });
                buttons.addView(btn);
            }
        }
        setContentView(buttons);
        this.mContent = buttons;
        setSysui();
        this.mContent.setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int visibility) {
                if ((visibility & 2) == 0) {
                    ToastActivity.this.mImmersive = false;
                    ToastActivity.this.setSysui();
                }
            }
        });
    }

    public void toast1() {
        Toast.makeText(this.mContext, "toast!", 0).show();
    }

    public void toast2() {
        Toast t = Toast.makeText(this.mContext, "toast!", 0);
        TextView tv = new TextView(this.mContext);
        tv.setBackgroundColor(-65536);
        tv.setText("setView");
        t.setView(tv);
        t.show();
    }

    public void toast3() {
        Toast t = Toast.makeText(this.mContext, "toast!", 0);
        TextView tv = new TextView(this.mContext) {
            protected boolean fitSystemWindows(Rect insets) {
                Rect before = new Rect(insets);
                boolean rt = super.fitSystemWindows(insets);
                Log.d(ToastActivity.TAG, String.format("before=%s rt=%s after=%s", new Object[]{before.toShortString(), Boolean.valueOf(rt), insets.toShortString()}));
                return rt;
            }
        };
        Log.d(TAG, "fitsSystemWindows=" + tv.getFitsSystemWindows());
        tv.setFitsSystemWindows(true);
        tv.setSystemUiVisibility(768);
        tv.setBackgroundColor(-65536);
        tv.setText("setView");
        t.setView(tv);
        t.show();
    }

    public void hideNav() {
        this.mContent.setSystemUiVisibility(2);
    }

    public void dangerToast() {
        Toast t = Toast.makeText(this.mContext, "toast!", 0);
        TextView tv = new TextView(this.mContext);
        tv.setSystemUiVisibility(512);
        tv.setBackgroundColor(-65536);
        tv.setText("setView");
        t.setView(tv);
        t.setGravity(80, 0, 90);
        t.show();
    }

    public void toggleImmersive() {
        this.mImmersive = !this.mImmersive;
        setSysui();
    }

    private void setSysui() {
        int flags = 2560;
        if (this.mImmersive) {
            flags = 2560 | 2;
        }
        this.mContent.setSystemUiVisibility(flags);
    }
}
