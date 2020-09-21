package js.kbars;

import android.app.Activity;
import android.content.Context;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;

public final class TransparencyToggleButton extends Button {
    private final String mDescription;
    private boolean mTransparent;
    private final int mWmFlag;

    public TransparencyToggleButton(Context context, String description, int wmFlag) {
        super(context);
        this.mDescription = description;
        this.mWmFlag = wmFlag;
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                TransparencyToggleButton.this.toggle("clicked");
            }
        });
        update();
    }

    private void toggle(String reason) {
        Log.d(KBarsActivity.TAG, "toggle reason=" + reason);
        this.mTransparent = !this.mTransparent;
        update();
    }

    private void update() {
        setText("Make " + this.mDescription + " " + (this.mTransparent ? "opaque" : "transparent"));
        Window w = ((Activity) getContext()).getWindow();
        if (this.mTransparent) {
            w.addFlags(this.mWmFlag);
        } else {
            w.clearFlags(this.mWmFlag);
        }
    }
}
