package js.kbars;

import android.content.Context;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public final class LightsOutModeToggleButton extends Button {
    private boolean mLightsOut;

    public LightsOutModeToggleButton(Context context) {
        super(context);
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                LightsOutModeToggleButton.this.mLightsOut = !LightsOutModeToggleButton.this.mLightsOut;
                LightsOutModeToggleButton.this.update();
            }
        });
        update();
    }

    private void update() {
        setText(new StringBuilder(String.valueOf(this.mLightsOut ? "Exit" : "Enter")).append(" lights out mode").toString());
        setSystemUiVisibility(this.mLightsOut ? 1 : 0);
    }
}
