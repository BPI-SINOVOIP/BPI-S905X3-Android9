/*
 * Copyright (C) 2018 The AndroCid Open Source Project
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

package android.security.cts;

import android.test.AndroidTestCase;

import android.app.Activity;
import android.os.BaseBundle;
import android.os.Bundle;
import android.os.Parcel;
import android.annotation.SuppressLint;

import java.io.InputStream;
import java.lang.reflect.Field;
import java.util.Random;

import android.security.cts.R;
import android.platform.test.annotations.SecurityTest;

public class AmbiguousBundlesTest extends AndroidTestCase {

    @SecurityTest
    public void test_android_CVE_2017_13287() throws Exception {
        Bundle bundle;
        {
            Bundle verifyMe = new Bundle();
            verifyMe.putString("cmd", "something_safe");
            Bundle useMe = new Bundle();
            useMe.putString("cmd", "replaced_thing");
            Ambiguator a = new Ambiguator() {
                @Override
                public Bundle make(Bundle preReSerialize, Bundle postReSerialize) throws Exception {
                    Random random = new Random(1234);
                    int minHash = 0;
                    for (String s : preReSerialize.keySet()) {
                        minHash = Math.min(minHash, s.hashCode());
                    }
                    for (String s : postReSerialize.keySet()) {
                        minHash = Math.min(minHash, s.hashCode());
                    }

                    String key;
                    int keyHash;

                    do {
                        key = randomString(random);
                        keyHash = key.hashCode();
                    } while (keyHash >= minHash);

                    padBundle(postReSerialize, preReSerialize.size() + 1, minHash, random);
                    padBundle(preReSerialize, postReSerialize.size() - 1, minHash, random);

                    String key2;
                    int key2Hash;
                    do {
                        key2 = makeStringToInject(postReSerialize, random);
                        key2Hash = key2.hashCode();
                    } while (key2Hash >= minHash || key2Hash <= keyHash);


                    Parcel parcel = Parcel.obtain();

                    parcel.writeInt(preReSerialize.size() + 2);
                    parcel.writeString(key);

                    parcel.writeInt(VAL_PARCELABLE);
                    parcel.writeString("com.android.internal.widget.VerifyCredentialResponse");

                    parcel.writeInt(0);
                    parcel.writeInt(0);

                    parcel.writeString(key2);
                    parcel.writeInt(VAL_NULL);

                    writeBundleSkippingHeaders(parcel, preReSerialize);

                    parcel.setDataPosition(0);
                    Bundle bundle = new Bundle();
                    parcelledDataField.set(bundle, parcel);
                    return bundle;
                }

                @Override
                protected String makeStringToInject(Bundle stuffToInject, Random random) {
                    Parcel p = Parcel.obtain();
                    p.writeInt(0);
                    p.writeInt(0);

                    Parcel p2 = Parcel.obtain();
                    stuffToInject.writeToParcel(p2, 0);
                    int p2Len = p2.dataPosition() - BUNDLE_SKIP;

                    for (int i = 0; i < p2Len / 4 + 4; i++) {
                        int paddingVal;
                        if (i > 3) {
                            paddingVal = i;
                        } else {
                            paddingVal = random.nextInt();
                        }
                        p.writeInt(paddingVal);

                    }

                    p.appendFrom(p2, BUNDLE_SKIP, p2Len);
                    p2.recycle();

                    while (p.dataPosition() % 8 != 0) p.writeInt(0);
                    for (int i = 0; i < 2; i++) {
                        p.writeInt(0);
                    }

                    int len = p.dataPosition() / 2 - 1;
                    p.writeInt(0); p.writeInt(0);
                    p.setDataPosition(0);
                    p.writeInt(len);
                    p.writeInt(len);
                    p.setDataPosition(0);
                    String result = p.readString();
                    p.recycle();
                    return result;
                }
            };
            bundle = a.make(verifyMe, useMe);
        }

        bundle = reparcel(bundle);
        String value1 = bundle.getString("cmd");
        bundle = reparcel(bundle);
        String value2 = bundle.getString("cmd");

        if (!value1.equals(value2)) {
            fail("String " + value1 + "!=" + value2 + " after reparceling.");
        }
    }

    @SuppressLint("ParcelClassLoader")
    private Bundle reparcel(Bundle source) {
        Parcel p = Parcel.obtain();
        p.writeBundle(source);
        p.setDataPosition(0);
        Bundle copy = p.readBundle();
        p.recycle();
        return copy;
    }

    static abstract class Ambiguator {

        protected static final int VAL_PARCELABLE = 4;
        protected static final int VAL_NULL = -1;
        protected static final int BUNDLE_SKIP = 12;

        protected final Field parcelledDataField;

        public Ambiguator() throws Exception {
            parcelledDataField = BaseBundle.class.getDeclaredField("mParcelledData");
            parcelledDataField.setAccessible(true);
        }

        abstract public Bundle make(Bundle preReSerialize, Bundle postReSerialize) throws Exception;

        abstract protected String makeStringToInject(Bundle stuffToInject, Random random);

        protected static void writeBundleSkippingHeaders(Parcel parcel, Bundle bundle) {
            Parcel p2 = Parcel.obtain();
            bundle.writeToParcel(p2, 0);
            parcel.appendFrom(p2, BUNDLE_SKIP, p2.dataPosition() - BUNDLE_SKIP);
            p2.recycle();
        }

        protected static String randomString(Random random) {
            StringBuilder b = new StringBuilder();
            for (int i = 0; i < 6; i++) {
                b.append((char)(' ' + random.nextInt('~' - ' ' + 1)));
            }
            return b.toString();
        }

        protected static void padBundle(Bundle bundle, int size, int minHash, Random random) {
            while (bundle.size() < size) {
                String key;
                do {
                    key = randomString(random);
                } while (key.hashCode() < minHash || bundle.containsKey(key));
                bundle.putString(key, "PADDING");
            }
        }
    }
}
