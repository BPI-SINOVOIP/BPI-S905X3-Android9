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

package libcore.libcore.util;

import junit.framework.TestCase;

import libcore.util.NativeAllocationRegistry;

public class NativeAllocationRegistryTest extends TestCase {

    static {
        System.loadLibrary("javacoretests");
    }

    private ClassLoader classLoader = NativeAllocationRegistryTest.class.getClassLoader();

    private static class TestConfig {
        public boolean useAllocator;
        public boolean shareRegistry;

        public TestConfig(boolean useAllocator, boolean shareRegistry) {
            this.useAllocator = useAllocator;
            this.shareRegistry = shareRegistry;
        }
    }

    private static class Allocation {
        public byte[] javaAllocation;
        public long nativeAllocation;
    }

    // Verify that NativeAllocations and their referents are freed before we run
    // out of space for new allocations.
    private void testNativeAllocation(TestConfig config) {
        Runtime.getRuntime().gc();
        long max = Runtime.getRuntime().maxMemory();
        long total = Runtime.getRuntime().totalMemory();
        int size = 1024*1024;
        int expectedMaxNumAllocations = (int)(max-total)/size;
        int numSavedAllocations = expectedMaxNumAllocations/2;
        Allocation[] saved = new Allocation[numSavedAllocations];

        final int nativeSize = size/2;
        int javaSize = size/2;
        NativeAllocationRegistry registry = new NativeAllocationRegistry(
                classLoader, getNativeFinalizer(), nativeSize);

        // Allocate more native allocations than will fit in memory. This should
        // not throw OutOfMemoryError because the few allocations we save
        // references to should easily fit.
        for (int i = 0; i < expectedMaxNumAllocations * 10; i++) {
            if (!config.shareRegistry) {
                registry = new NativeAllocationRegistry(
                    classLoader, getNativeFinalizer(), nativeSize);
            }

            final Allocation alloc = new Allocation();
            alloc.javaAllocation = new byte[javaSize];
            if (config.useAllocator) {
                NativeAllocationRegistry.Allocator allocator
                  = new NativeAllocationRegistry.Allocator() {
                    public long allocate() {
                        alloc.nativeAllocation = doNativeAllocation(nativeSize);
                        return alloc.nativeAllocation;
                    }
                };
                registry.registerNativeAllocation(alloc, allocator);
            } else {
                alloc.nativeAllocation = doNativeAllocation(nativeSize);
                registry.registerNativeAllocation(alloc, alloc.nativeAllocation);
            }

            saved[i%numSavedAllocations] = alloc;
        }

        // Verify most of the allocations have been freed.
        long nativeBytes = getNumNativeBytesAllocated();
        assertTrue("Excessive native bytes still allocated (" + nativeBytes + ")"
                + " given max memory of (" + max + ")", nativeBytes < 2 * max);
    }

    public void testNativeAllocationAllocatorAndSharedRegistry() {
        testNativeAllocation(new TestConfig(true, true));
    }

    public void testNativeAllocationNoAllocatorAndSharedRegistry() {
        testNativeAllocation(new TestConfig(false, true));
    }

    public void testNativeAllocationAllocatorAndNoSharedRegistry() {
        testNativeAllocation(new TestConfig(true, false));
    }

    public void testNativeAllocationNoAllocatorAndNoSharedRegistry() {
        testNativeAllocation(new TestConfig(false, false));
    }

    public void testBadSize() {
        assertThrowsIllegalArgumentException(new Runnable() {
            public void run() {
                NativeAllocationRegistry registry = new NativeAllocationRegistry(
                        classLoader, getNativeFinalizer(), -8);
            }
        });
    }

    public void testEarlyFree() {
        long size = 1234;
        NativeAllocationRegistry registry
            = new NativeAllocationRegistry(classLoader, getNativeFinalizer(), size);
        long nativePtr = doNativeAllocation(size);
        Object referent = new Object();
        Runnable cleaner = registry.registerNativeAllocation(referent, nativePtr);
        long numBytesAllocatedBeforeClean = getNumNativeBytesAllocated();

        // Running the cleaner should cause the native finalizer to run.
        cleaner.run();
        long numBytesAllocatedAfterClean = getNumNativeBytesAllocated();
        assertEquals(numBytesAllocatedBeforeClean - size, numBytesAllocatedAfterClean);

        // Running the cleaner again should have no effect.
        cleaner.run();
        assertEquals(numBytesAllocatedAfterClean, getNumNativeBytesAllocated());

        // There shouldn't be any problems when the referent object is GC'd.
        referent = null;
        Runtime.getRuntime().gc();
    }

    public void testNullArguments() {
        final NativeAllocationRegistry registry
            = new NativeAllocationRegistry(classLoader, getNativeFinalizer(), 1024);
        final long dummyNativePtr = 0x1;
        final Object referent = new Object();

        // referent should not be null
        assertThrowsIllegalArgumentException(new Runnable() {
            public void run() {
                registry.registerNativeAllocation(null, dummyNativePtr);
            }
        });

        // nativePtr should not be null
        assertThrowsIllegalArgumentException(new Runnable() {
            public void run() {
                registry.registerNativeAllocation(referent, 0);
            }
        });

        // referent should not be null
        assertThrowsIllegalArgumentException(new Runnable() {
            public void run() {
                registry.registerNativeAllocation(null,
                        new NativeAllocationRegistry.Allocator() {
                            public long allocate() {
                                // The allocate function ought not to be called.
                                fail("allocate function called");
                                return dummyNativePtr;
                            }
                        });
            }
        });

        // Allocation that returns null should have no effect.
        assertNull(registry.registerNativeAllocation(referent,
                    new NativeAllocationRegistry.Allocator() {
                        public long allocate() {
                            return 0;
                        }
                    }));
    }

    private static void assertThrowsIllegalArgumentException(Runnable runnable) {
        try {
            runnable.run();
        } catch (IllegalArgumentException ex) {
            return;
        }
        fail("Expected IllegalArgumentException, but no exception was thrown.");
    }

    private static native long getNativeFinalizer();
    private static native long doNativeAllocation(long size);
    private static native long getNumNativeBytesAllocated();
}
