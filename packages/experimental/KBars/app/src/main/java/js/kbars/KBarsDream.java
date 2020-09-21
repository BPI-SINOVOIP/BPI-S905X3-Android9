package js.kbars;

import android.content.Context;
import android.os.Handler;
import android.service.dreams.DreamService;
import android.util.Log;
import android.view.View;
import android.view.View.OnSystemUiVisibilityChangeListener;

public class KBarsDream extends DreamService {
    private static final String TAG = Util.logTag(KBarsDream.class);
    private final Context mContext = this;
    private final Handler mHandler = new Handler();
    private View mView;

    public void onCreate() {
        super.onCreate();
        setInteractive(true);
    }

    public void onAttachedToWindow() {
        super.onAttachedToWindow();
        this.mView = new View(this.mContext);
        getWindow().addFlags(1024);
        setFullscreen();
        this.mView.setOnSystemUiVisibilityChangeListener(new OnSystemUiVisibilityChangeListener() {
            public void onSystemUiVisibilityChange(int visibility) {
                Log.d(KBarsDream.TAG, "onSystemUiVisibilityChange " + Integer.toHexString(visibility));
                KBarsDream.this.setFullscreen();
            }
        });
        this.mView.setBackgroundColor(-16776961);
        setContentView(this.mView);
    }

    public void onWindowFocusChanged(boolean hasFocus) {
        Log.d(TAG, "onWindowFocusChanged " + hasFocus);
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            setFullscreen();
        }
    }

    private void setFullscreen() {
        this.mView.setSystemUiVisibility(5382);
    }
}
