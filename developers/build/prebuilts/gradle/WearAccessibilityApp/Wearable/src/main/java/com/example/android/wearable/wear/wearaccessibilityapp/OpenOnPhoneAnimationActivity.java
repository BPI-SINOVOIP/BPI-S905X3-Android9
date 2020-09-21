/*
 * Copyright (C) 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.android.wearable.wear.wearaccessibilityapp;

import android.app.Activity;
import android.graphics.drawable.Animatable2.AnimationCallback;
import android.graphics.drawable.AnimatedVectorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Bundle;
import android.support.wear.ambient.AmbientMode;
import android.widget.ImageView;

public class OpenOnPhoneAnimationActivity extends Activity
        implements AmbientMode.AmbientCallbackProvider {
    private AnimationCallback mAnimationCallback;
    private AnimatedVectorDrawable mAnimatedVectorDrawablePhone;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_open_on_phone_animation);

        AmbientMode.attachAmbientSupport(this);

        mAnimationCallback =
                new AnimationCallback() {
                    @Override
                    public void onAnimationEnd(Drawable drawable) {
                        super.onAnimationEnd(drawable);
                        // Go back to main Dialogs screen after animation.
                        finish();
                    }
                };

        // Play 'swipe left' animation only once.
        ImageView phoneImage = findViewById(R.id.open_on_phone_animation_image);
        mAnimatedVectorDrawablePhone = (AnimatedVectorDrawable) phoneImage.getDrawable();
        mAnimatedVectorDrawablePhone.registerAnimationCallback(mAnimationCallback);
        mAnimatedVectorDrawablePhone.start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mAnimatedVectorDrawablePhone.unregisterAnimationCallback(mAnimationCallback);
    }

    @Override
    public AmbientMode.AmbientCallback getAmbientCallback() {
        return new MyAmbientCallback();
    }

    private class MyAmbientCallback extends AmbientMode.AmbientCallback {}
}
