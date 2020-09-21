/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tools.build.apkzlib.zfile;

import static com.google.common.base.Preconditions.checkNotNull;

import com.google.common.base.Preconditions;
import java.io.File;
import java.security.PrivateKey;
import java.security.cert.X509Certificate;
import java.util.function.Predicate;
import javax.annotation.Nonnull;
import javax.annotation.Nullable;

/**
 * Factory that creates instances of {@link ApkCreator}.
 */
public interface ApkCreatorFactory {

    /**
     * Creates an {@link ApkCreator} with a given output location, and signing information.
     *
     * @param creationData the information to create the APK
     */
    ApkCreator make(@Nonnull CreationData creationData);

    /**
     * Data structure with the required information to initiate the creation of an APK. See
     * {@link ApkCreatorFactory#make(CreationData)}.
     */
    class CreationData {

        /**
         * The path where the APK should be located. May already exist or not (if it does, then
         * the APK may be updated instead of created).
         */
        @Nonnull
        private final File apkPath;

        /**
         * Key used to sign the APK. May be {@code null}.
         */
        @Nullable
        private final PrivateKey key;

        /**
         * Certificate used to sign the APK. Is {@code null} if and only if {@link #key} is
         * {@code null}.
         */
        @Nullable
        private final X509Certificate certificate;

        /**
         * Whether signing the APK with JAR Signing Scheme (aka v1 signing) is enabled.
         */
        private final boolean v1SigningEnabled;

        /**
         * Whether signing the APK with APK Signature Scheme v2 (aka v2 signing) is enabled.
         */
        private final boolean v2SigningEnabled;

        /**
         * Built-by information for the APK, if any.
         */
        @Nullable
        private final String builtBy;

        /**
         * Created-by information for the APK, if any.
         */
        @Nullable
        private final String createdBy;

        /**
         * Minimum SDk version that will run the APK.
         */
        private final int minSdkVersion;

        /**
         * How should native libraries be packaged?
         */
        @Nonnull
        private final NativeLibrariesPackagingMode nativeLibrariesPackagingMode;

        /**
         * Predicate identifying paths that should not be compressed.
         */
        @Nonnull
        private final Predicate<String> noCompressPredicate;

        /**
         *
         * @param apkPath the path where the APK should be located. May already exist or not (if it
         * does, then the APK may be updated instead of created)
         * @param key key used to sign the APK. May be {@code null}
         * @param certificate certificate used to sign the APK. Is {@code null} if and only if
         * {@code key} is {@code null}
         * @param v1SigningEnabled {@code true} if this APK should be signed with JAR Signature
         *        Scheme (aka v1 scheme).
         * @param v2SigningEnabled {@code true} if this APK should be signed with APK Signature
         *        Scheme v2 (aka v2 scheme).
         * @param builtBy built-by information for the APK, if any; if {@code null} then the default
         * should be used
         * @param createdBy created-by information for the APK, if any; if {@code null} then the
         * default should be used
         * @param minSdkVersion minimum SDK version that will run the APK
         * @param nativeLibrariesPackagingMode packaging mode for native libraries
         * @param noCompressPredicate predicate to decide which file paths should be uncompressed;
         * returns {@code true} for files that should not be compressed
         */
        public CreationData(
                @Nonnull File apkPath,
                @Nullable PrivateKey key,
                @Nullable X509Certificate certificate,
                boolean v1SigningEnabled,
                boolean v2SigningEnabled,
                @Nullable String builtBy,
                @Nullable String createdBy,
                int minSdkVersion,
                @Nonnull NativeLibrariesPackagingMode nativeLibrariesPackagingMode,
                @Nonnull Predicate<String> noCompressPredicate) {
            Preconditions.checkArgument((key == null) == (certificate == null),
                    "(key == null) != (certificate == null)");
            Preconditions.checkArgument(minSdkVersion >= 0, "minSdkVersion < 0");

            this.apkPath = apkPath;
            this.key = key;
            this.certificate = certificate;
            this.v1SigningEnabled = v1SigningEnabled;
            this.v2SigningEnabled = v2SigningEnabled;
            this.builtBy = builtBy;
            this.createdBy = createdBy;
            this.minSdkVersion = minSdkVersion;
            this.nativeLibrariesPackagingMode = checkNotNull(nativeLibrariesPackagingMode);
            this.noCompressPredicate = checkNotNull(noCompressPredicate);
        }

        /**
         * Obtains the path where the APK should be located. If the path already exists, then the
         * APK may be updated instead of re-created.
         *
         * @return the path that may already exist or not
         */
        @Nonnull
        public File getApkPath() {
            return apkPath;
        }

        /**
         * Obtains the private key used to sign the APK.
         *
         * @return the private key or {@code null} if the APK should not be signed
         */
        @Nullable
        public PrivateKey getPrivateKey() {
            return key;
        }

        /**
         * Obtains the certificate used to sign the APK.
         *
         * @return the certificate or {@code null} if the APK should not be signed; this will return
         * {@code null} if and only if {@link #getPrivateKey()} returns {@code null}
         */
        @Nullable
        public X509Certificate getCertificate() {
            return certificate;
        }

        /**
         * Returns {@code true} if this APK should be signed with JAR Signature Scheme (aka v1
         * scheme).
         */
        public boolean isV1SigningEnabled() {
            return v1SigningEnabled;
        }

        /**
         * Returns {@code true} if this APK should be signed with APK Signature Scheme v2 (aka v2
         * scheme).
         */
        public boolean isV2SigningEnabled() {
            return v2SigningEnabled;
        }

        /**
         * Obtains the "built-by" text for the APK.
         *
         * @return the text or {@code null} if the default should be used
         */
        @Nullable
        public String getBuiltBy() {
            return builtBy;
        }

        /**
         * Obtains the "created-by" text for the APK.
         *
         * @return the text or {@code null} if the default should be used
         */
        @Nullable
        public String getCreatedBy() {
            return createdBy;
        }

        /**
         * Obtains the minimum SDK version to run the APK.
         *
         * @return the minimum SDK version
         */
        public int getMinSdkVersion() {
            return minSdkVersion;
        }

        /**
         * Returns the packaging policy that the {@link ApkCreator} should use for native libraries.
         */
        @Nonnull
        public NativeLibrariesPackagingMode getNativeLibrariesPackagingMode() {
            return nativeLibrariesPackagingMode;
        }

        /**
         * Returns the predicate to decide which file paths should be uncompressed.
         */
        @Nonnull
        public Predicate<String> getNoCompressPredicate() {
            return noCompressPredicate;
        }
    }
}
