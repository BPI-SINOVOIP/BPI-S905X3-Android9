/*
 * Copyright (C) 2017 The Android Open Source Project
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

package androidx.navigation;

import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.res.TypedArray;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.AttributeSet;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * ActivityNavigator implements cross-activity navigation.
 */
@Navigator.Name("activity")
public class ActivityNavigator extends Navigator<ActivityNavigator.Destination> {
    private static final String EXTRA_NAV_SOURCE =
            "android-support-navigation:ActivityNavigator:source";
    private static final String EXTRA_NAV_CURRENT =
            "android-support-navigation:ActivityNavigator:current";

    private Context mContext;
    private Activity mHostActivity;

    public ActivityNavigator(@NonNull Context context) {
        mContext = context;
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                mHostActivity = (Activity) context;
                break;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }
    }

    @NonNull
    Context getContext() {
        return mContext;
    }

    @NonNull
    @Override
    public Destination createDestination() {
        return new Destination(this);
    }

    @Override
    public boolean popBackStack() {
        if (mHostActivity != null) {
            int destId = 0;
            final Intent intent = mHostActivity.getIntent();
            if (intent != null) {
                destId = intent.getIntExtra(EXTRA_NAV_SOURCE, 0);
            }
            mHostActivity.finish();
            dispatchOnNavigatorNavigated(destId, BACK_STACK_DESTINATION_POPPED);
            return true;
        }
        return false;
    }

    @SuppressWarnings("deprecation")
    @Override
    public void navigate(@NonNull Destination destination, @Nullable Bundle args,
            @Nullable NavOptions navOptions) {
        if (destination.getIntent() == null) {
            throw new IllegalStateException("Destination " + destination.getId()
                    + " does not have an Intent set.");
        }
        Intent intent = new Intent(destination.getIntent());
        if (args != null) {
            intent.putExtras(args);
            String dataPattern = destination.getDataPattern();
            if (!TextUtils.isEmpty(dataPattern)) {
                // Fill in the data pattern with the args to build a valid URI
                StringBuffer data = new StringBuffer();
                Pattern fillInPattern = Pattern.compile("\\{(.+?)\\}");
                Matcher matcher = fillInPattern.matcher(dataPattern);
                while (matcher.find()) {
                    String argName = matcher.group(1);
                    if (args.containsKey(argName)) {
                        matcher.appendReplacement(data, "");
                        data.append(Uri.encode(args.getString(argName)));
                    } else {
                        throw new IllegalArgumentException("Could not find " + argName + " in "
                                + args + " to fill data pattern " + dataPattern);
                    }
                }
                matcher.appendTail(data);
                intent.setData(Uri.parse(data.toString()));
            }
        }
        if (navOptions != null && navOptions.shouldClearTask()) {
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TASK);
        }
        if (navOptions != null && navOptions.shouldLaunchDocument()
                && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_DOCUMENT);
        } else if (!(mContext instanceof Activity)) {
            // If we're not launching from an Activity context we have to launch in a new task.
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }
        if (navOptions != null && navOptions.shouldLaunchSingleTop()) {
            intent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
        }
        if (mHostActivity != null) {
            final Intent hostIntent = mHostActivity.getIntent();
            if (hostIntent != null) {
                final int hostCurrentId = hostIntent.getIntExtra(EXTRA_NAV_CURRENT, 0);
                if (hostCurrentId != 0) {
                    intent.putExtra(EXTRA_NAV_SOURCE, hostCurrentId);
                }
            }
        }
        final int destId = destination.getId();
        intent.putExtra(EXTRA_NAV_CURRENT, destId);
        NavOptions.addPopAnimationsToIntent(intent, navOptions);
        mContext.startActivity(intent);
        if (navOptions != null && mHostActivity != null) {
            int enterAnim = navOptions.getEnterAnim();
            int exitAnim = navOptions.getExitAnim();
            if (enterAnim != -1 || exitAnim != -1) {
                enterAnim = enterAnim != -1 ? enterAnim : 0;
                exitAnim = exitAnim != -1 ? exitAnim : 0;
                mHostActivity.overridePendingTransition(enterAnim, exitAnim);
            }
        }

        // You can't pop the back stack from the caller of a new Activity,
        // so we don't add this navigator to the controller's back stack
        dispatchOnNavigatorNavigated(destId, BACK_STACK_UNCHANGED);
    }

    /**
     * NavDestination for activity navigation
     */
    public static class Destination extends NavDestination {
        private Intent mIntent;
        private String mDataPattern;

        /**
         * Construct a new activity destination. This destination is not valid until you set the
         * Intent via {@link #setIntent(Intent)} or one or more of the other set method.
         *
         *
         * @param navigatorProvider The {@link NavController} which this destination
         *                          will be associated with.
         */
        public Destination(@NonNull NavigatorProvider navigatorProvider) {
            this(navigatorProvider.getNavigator(ActivityNavigator.class));
        }

        /**
         * Construct a new activity destination. This destination is not valid until you set the
         * Intent via {@link #setIntent(Intent)} or one or more of the other set method.
         *
         * @param activityNavigator The {@link ActivityNavigator} which this destination
         *                          will be associated with. Generally retrieved via a
         *                          {@link NavController}'s
         *                          {@link NavigatorProvider#getNavigator(Class)} method.
         */
        public Destination(@NonNull Navigator<? extends Destination> activityNavigator) {
            super(activityNavigator);
        }

        @Override
        public void onInflate(@NonNull Context context, @NonNull AttributeSet attrs) {
            super.onInflate(context, attrs);
            TypedArray a = context.getResources().obtainAttributes(attrs,
                    R.styleable.ActivityNavigator);
            String cls = a.getString(R.styleable.ActivityNavigator_android_name);
            if (!TextUtils.isEmpty(cls)) {
                // TODO Replace with ComponentName.createRelative() when minSdkVersion is 23
                if (cls.charAt(0) == '.') {
                    cls = context.getPackageName() + cls;
                }
                setComponentName(new ComponentName(context, cls));
            }
            setAction(a.getString(R.styleable.ActivityNavigator_action));
            String data = a.getString(R.styleable.ActivityNavigator_data);
            if (data != null) {
                setData(Uri.parse(data));
            }
            setDataPattern(a.getString(R.styleable.ActivityNavigator_dataPattern));
            a.recycle();
        }

        /**
         * Set the Intent to start when navigating to this destination.
         * @param intent Intent to associated with this destination.
         * @return this {@link Destination}
         */
        @NonNull
        public Destination setIntent(Intent intent) {
            mIntent = intent;
            return this;
        }

        /**
         * Gets the Intent associated with this destination.
         * @return
         */
        @Nullable
        public Intent getIntent() {
            return mIntent;
        }

        /**
         * Set an explicit {@link ComponentName} to navigate to.
         *
         * @param name The component name of the Activity to start.
         * @return this {@link Destination}
         */
        @NonNull
        public Destination setComponentName(ComponentName name) {
            if (mIntent == null) {
                mIntent = new Intent();
            }
            mIntent.setComponent(name);
            return this;
        }

        /**
         * Get the explicit {@link ComponentName} associated with this destination, if any
         * @return
         */
        @Nullable
        public ComponentName getComponent() {
            if (mIntent == null) {
                return null;
            }
            return mIntent.getComponent();
        }

        /**
         * Sets the action sent when navigating to this destination.
         * @param action The action string to use.
         * @return this {@link Destination}
         */
        @NonNull
        public Destination setAction(String action) {
            if (mIntent == null) {
                mIntent = new Intent();
            }
            mIntent.setAction(action);
            return this;
        }

        /**
         * Get the action used to start the Activity, if any
         */
        @Nullable
        public String getAction() {
            if (mIntent == null) {
                return null;
            }
            return mIntent.getAction();
        }

        /**
         * Sets a static data URI that is sent when navigating to this destination.
         *
         * <p>To use a dynamic URI that changes based on the arguments passed in when navigating,
         * use {@link #setDataPattern(String)}, which will take precedence when arguments are
         * present.</p>
         *
         * @param data A static URI that should always be used.
         * @see #setDataPattern(String)
         * @return this {@link Destination}
         */
        @NonNull
        public Destination setData(Uri data) {
            if (mIntent == null) {
                mIntent = new Intent();
            }
            mIntent.setData(data);
            return this;
        }

        /**
         * Get the data URI used to start the Activity, if any
         */
        @Nullable
        public Uri getData() {
            if (mIntent == null) {
                return null;
            }
            return mIntent.getData();
        }

        /**
         * Sets a dynamic data URI pattern that is sent when navigating to this destination.
         *
         * <p>If a non-null arguments Bundle is present when navigating, any segments in the form
         * <code>{argName}</code> will be replaced with a URI encoded string from the arguments.</p>
         * @param dataPattern A URI pattern with segments in the form of <code>{argName}</code> that
         *                    will be replaced with URI encoded versions of the Strings in the
         *                    arguments Bundle.
         * @see #setData
         * @return this {@link Destination}
         */
        @NonNull
        public Destination setDataPattern(String dataPattern) {
            mDataPattern = dataPattern;
            return this;
        }

        /**
         * Gets the dynamic data URI pattern, if any
         */
        @Nullable
        public String getDataPattern() {
            return mDataPattern;
        }
    }
}
