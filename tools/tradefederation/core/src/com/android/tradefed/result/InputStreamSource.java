/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.tradefed.result;

import java.io.Closeable;
import java.io.InputStream;

/**
 * This interface basically wraps an {@link InputStream} to make it clonable.
 *
 * <p>It should be expected that a resource will be leaked unless {@link #cancel()} is called, and
 * that once {@link #cancel()} is called on an instance, that that instance and any {@link
 * InputStream}s it has created will be invalid.
 */
public interface InputStreamSource extends Closeable {

    /**
     * Return a new clone of the {@link InputStream}, so that the caller can read the stream from
     * the beginning.  Each invocation of this method (until {@link #cancel()} is called) will
     * return an identically-behaving {@link InputStream} -- the same contents will be returned.
     *
     * @return An {@link InputStream} that the caller can use to read the data source from the
     *         beginning.  May return {@code null} if this {@code InputStreamSource} has been
     *         invalidated by a prior call to {@link #cancel()}, or if a new InputStream cannot be
     *         created for some other reason.
     */
    public InputStream createInputStream();

    /**
     * Do any required cleanup on the source of the InputStream. Calling this method essentially
     * invalidates this {@code InputStreamSource}.
     *
     * @deprecated use {@link #close()} instead.
     */
    @Deprecated
    public default void cancel() {
        close();
    }

    /**
     * Do any required cleanup on the source of the InputStream. Calling this method essentially
     * invalidates this {@code InputStreamSource}.
     */
    @Override
    public void close();

    /**
     * Return the size in bytes of the source data.
     */
    public long size();
}

