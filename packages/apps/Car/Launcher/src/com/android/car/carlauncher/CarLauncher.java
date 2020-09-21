package com.android.car.carlauncher;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.View;

/**
 *  Basic Launcher for Android Automotive
 */
public class CarLauncher extends Activity {

   @Override
    protected void onCreate(Bundle savedInstanceState) {
       super.onCreate(savedInstanceState);
       setContentView(R.layout.car_launcher);
   }

   // called by onClick in xml
    public void openAppsList(View v){
        startActivity(new Intent(this, AppGridActivity.class));
    }
}
