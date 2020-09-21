package org.dtvkit.inputsource;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

public class Setup extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.setup);

        findViewById(R.id.cablemanualsearch).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent(getApplicationContext(), DtvkitDvbcSetup.class));
                finish();
            }
        });

        findViewById(R.id.satellitemanualsearch).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent(getApplicationContext(), DtvkitDvbsSetup.class));
                finish();
            }
        });

        findViewById(R.id.terrestrialautosearch).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                startActivity(new Intent(getApplicationContext(), DtvkitDvbtSetup.class));
                finish();
            }
        });
    }


}
