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

package com.android.cts.util;

import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

public class TestResult implements Parcelable {
    public static final String EXTRA_TEST_RESULT =
            "com.android.cts.ephemeraltest.EXTRA_TEST_RESULT";
    private static final String ACTION_START_ACTIVITY =
            "com.android.cts.ephemeraltest.START_ACTIVITY";

    private final String mPackageName;
    private final String mComponentName;
    private final String mMethodName;
    private final String mStatus;
    private final String mException;
    private final Intent mIntent;
    private final boolean mInstantAppPackageInfoExposed;

    public String getPackageName() {
        return mPackageName;
    }

    public String getComponentName() {
        return mComponentName;
    }

    public String getMethodName() {
        return mMethodName;
    }

    public String getStatus() {
        return mStatus;
    }

    public String getException() {
        return mException;
    }

    public boolean getEphemeralPackageInfoExposed() {
        return mInstantAppPackageInfoExposed;
    }

    public Intent getIntent() {
        return mIntent;
    }

    public static Builder getBuilder() {
        return new Builder();
    }

    public void broadcast(Context context) {
        final Intent broadcastIntent = new Intent(ACTION_START_ACTIVITY);
        broadcastIntent.addCategory(Intent.CATEGORY_DEFAULT);
        broadcastIntent.addFlags(Intent.FLAG_RECEIVER_VISIBLE_TO_INSTANT_APPS);
        broadcastIntent.putExtra(EXTRA_TEST_RESULT, this);
        context.sendBroadcast(broadcastIntent);
    }

    public void startActivity(Context context, Uri uri) {
        final Intent broadcastIntent = new Intent(Intent.ACTION_VIEW);
        broadcastIntent.addCategory(Intent.CATEGORY_BROWSABLE);
        broadcastIntent.putExtra(EXTRA_TEST_RESULT, this);
        broadcastIntent.setData(uri);
        context.startActivity(broadcastIntent);
    }

    private TestResult(String packageName, String componentName, String methodName,
            String status, String exception, Intent intent,
            boolean ephemeralPackageInfoExposed) {
        mPackageName = packageName;
        mComponentName = componentName;
        mMethodName = methodName;
        mStatus = status;
        mException = exception;
        mIntent = intent;
        mInstantAppPackageInfoExposed = ephemeralPackageInfoExposed;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(mPackageName);
        dest.writeString(mComponentName);
        dest.writeString(mMethodName);
        dest.writeString(mStatus);
        dest.writeString(mException);
        dest.writeParcelable(mIntent, 0 /* flags */);
        dest.writeInt(mInstantAppPackageInfoExposed ? 1 : 0);
    }

    public static final Creator<TestResult> CREATOR = new Creator<TestResult>() {
        public TestResult createFromParcel(Parcel source) {
            return new TestResult(source);
        }
        public TestResult[] newArray(int size) {
            return new TestResult[size];
        }
    };

    private TestResult(Parcel source) {
        mPackageName = source.readString();
        mComponentName = source.readString();
        mMethodName = source.readString();
        mStatus = source.readString();
        mException = source.readString();
        mIntent = source.readParcelable(Object.class.getClassLoader());
        mInstantAppPackageInfoExposed = source.readInt() != 0;
    }

    public static class Builder {
        private String packageName;
        private String componentName;
        private String methodName;
        private String status;
        private String exception;
        private Intent intent;
        private boolean instantAppPackageInfoExposed;

        private Builder() {
        }
        public Builder setPackageName(String _packageName) {
            packageName = _packageName;
            return this;
        }
        public Builder setComponentName(String _componentName) {
            componentName = _componentName;
            return this;
        }
        public Builder setMethodName(String _methodName) {
            methodName = _methodName;
            return this;
        }
        public Builder setStatus(String _status) {
            status = _status;
            return this;
        }
        public Builder setException(String _exception) {
            exception = _exception;
            return this;
        }
        public Builder setEphemeralPackageInfoExposed(boolean _instantAppPackageInfoExposed) {
            instantAppPackageInfoExposed = _instantAppPackageInfoExposed;
            return this;
        }
        public Builder setIntent(Intent _intent) {
            intent = _intent;
            return this;
        }
        public TestResult build() {
            return new TestResult(packageName, componentName, methodName,
                    status, exception, intent, instantAppPackageInfoExposed);
        }
    }
}
