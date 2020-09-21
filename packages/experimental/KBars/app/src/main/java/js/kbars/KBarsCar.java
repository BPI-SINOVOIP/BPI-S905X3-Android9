package js.kbars;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;

public class KBarsCar extends Activity {
    private static final String TAG = Util.logTag(KBarsCar.class);
    private final Context mContext = this;
    private View mView;

    protected void onCreate(Bundle savedInstanceState) {
        requestWindowFeature(1);
        super.onCreate(savedInstanceState);
        this.mView = new View(this.mContext);
        this.mView.setBackgroundColor(-65536);
        setFullscreen();
        setContentView(this.mView);
        this.mView.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                ((UiModeManager) KBarsCar.this.mContext.getSystemService("uimode")).disableCarMode(1);
                KBarsCar.this.finish();
            }
        });
    }

    protected void onResume() {
        super.onResume();
        setFullscreen();
    }

    private void setFullscreen() {
        this.mView.setSystemUiVisibility(5382);
    }
}
