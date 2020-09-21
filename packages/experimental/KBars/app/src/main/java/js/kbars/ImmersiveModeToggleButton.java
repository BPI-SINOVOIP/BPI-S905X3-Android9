package js.kbars;

import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.View.OnSystemUiVisibilityChangeListener;
import android.widget.Button;

public final class ImmersiveModeToggleButton extends Button {
    private final KBarsActivity mActivity;
    private final String mCaption;
    private final int mImmersiveFlags;
    private boolean mImmersiveMode;

    public ImmersiveModeToggleButton(KBarsActivity activity, String caption, int immersiveFlags) {
        super(activity);
        this.mActivity = activity;
        this.mCaption = caption;
        this.mImmersiveFlags = immersiveFlags;
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                ImmersiveModeToggleButton.this.toggleImmersiveMode("clicked");
            }
        });
        updateImmersiveModeButton();
        setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int visibility) {
                ImmersiveModeToggleButton.this.onSysuiChanged(visibility);
            }
        });
    }

    private void toggleImmersiveMode(String reason) {
        boolean z;
        boolean z2 = false;
        Log.d(KBarsActivity.TAG, "toggleImmersiveMode reason=" + reason);
        if (this.mImmersiveMode) {
            z = false;
        } else {
            z = true;
        }
        this.mImmersiveMode = z;
        updateImmersiveModeButton();
        KBarsActivity kBarsActivity = this.mActivity;
        if (!this.mImmersiveMode) {
            z2 = true;
        }
        kBarsActivity.setSoloButton(z2, this, true);
    }

    private void updateImmersiveModeButton() {
        Log.d(KBarsActivity.TAG, "updateButtons mImmersiveMode=" + this.mImmersiveMode);
        setText(this.mImmersiveMode ? "Exit " + this.mCaption + " mode" : "Enter " + this.mCaption + " mode");
        setSystemUiVisibility(this.mImmersiveMode ? this.mImmersiveFlags : KBarsActivity.BASE_FLAGS);
    }

    private void onSysuiChanged(int vis) {
        boolean hideStatus;
        boolean hideSomething = false;
        Log.d(KBarsActivity.TAG, "sysui changed: " + Integer.toHexString(vis));
        if ((vis & 4) != 0) {
            hideStatus = true;
        } else {
            hideStatus = false;
        }
        boolean hideNav;
        if ((vis & 2) != 0) {
            hideNav = true;
        } else {
            hideNav = false;
        }
        if (hideStatus || hideNav) {
            hideSomething = true;
        }
        if (this.mImmersiveMode && !hideSomething) {
            toggleImmersiveMode("sysui_changed");
        }
    }
}
