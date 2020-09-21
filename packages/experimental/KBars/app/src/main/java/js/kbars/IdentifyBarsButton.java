package js.kbars;

import android.content.Context;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

public class IdentifyBarsButton extends Button {
    public IdentifyBarsButton(Context context) {
        super(context);
        setText("Identify system bars");
        setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                IdentifyBarsButton.this.identifyBars();
            }
        });
    }

    private void identifyBars() {
    }
}
