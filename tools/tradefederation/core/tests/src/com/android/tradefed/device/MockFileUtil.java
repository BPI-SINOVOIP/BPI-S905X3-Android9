/*
 * Copyright (C) 2010 The Android Open Source Project
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
package com.android.tradefed.device;

import com.android.ddmlib.FileListingService;

import org.easymock.EasyMock;

import java.util.ArrayList;
import java.util.Collection;

/**
 * Helper class for mocking out device file system contents
 */
public class MockFileUtil {

    /**
     * Helper method to mock out a remote filesystem contents
     *
     * @param mockDevice the mock {@link ITestDevice}
     * @param rootPath the path to the root
     * @param childNames the child file names of directory to simulate
     * @throws DeviceNotAvailableException
     */
    public static void setMockDirContents(ITestDevice mockDevice, String rootPath,
            String... childNames) throws DeviceNotAvailableException {
        IFileEntry rootEntry = EasyMock.createMock(IFileEntry.class);
        EasyMock.expect(mockDevice.getFileEntry(rootPath)).andStubReturn(rootEntry);
        boolean isDir = childNames.length != 0;
        EasyMock.expect(rootEntry.isDirectory()).andStubReturn(isDir);
        EasyMock.expect(rootEntry.getFullEscapedPath()).andStubReturn(rootPath);
        EasyMock.expect(rootEntry.getName()).andStubReturn(rootPath);
        Collection<IFileEntry> mockChildren = new ArrayList<IFileEntry>(childNames.length);
        for (String childName : childNames) {
            IFileEntry childMockEntry = EasyMock.createMock(IFileEntry.class);
            EasyMock.expect(childMockEntry.getName()).andStubReturn(childName);
            String fullPath = rootPath + FileListingService.FILE_SEPARATOR + childName;
            EasyMock.expect(childMockEntry.getFullEscapedPath()).andStubReturn(fullPath);
            EasyMock.expect(childMockEntry.isDirectory()).andStubReturn(Boolean.FALSE);
            mockChildren.add(childMockEntry);
            EasyMock.replay(childMockEntry);
        }
        EasyMock.expect(rootEntry.getChildren(EasyMock.anyBoolean()))
                .andStubReturn(mockChildren);
        EasyMock.replay(rootEntry);
    }

    /**
     * Helper method to mock out a remote nested filesystem contents
     *
     * @param mockDevice the mock {@link ITestDevice}
     * @param rootPath the path to the root
     * @param pathSegments the nested file path to simulate. This method will mock out IFileEntry
     *            objects to simulate a filesystem structure of rootPath/pathSegments
     * @throws DeviceNotAvailableException
     */
    public static void setMockDirPath(ITestDevice mockDevice, String rootPath,
            String... pathSegments) throws DeviceNotAvailableException {
        IFileEntry rootEntry = EasyMock.createMock(IFileEntry.class);
        EasyMock.expect(mockDevice.getFileEntry(rootPath)).andStubReturn(rootEntry);
        EasyMock.expect(rootEntry.getFullEscapedPath()).andStubReturn(rootPath);
        EasyMock.expect(rootEntry.getName()).andStubReturn(rootPath);
        for (int i=0; i < pathSegments.length; i++) {
            IFileEntry childMockEntry = EasyMock.createMock(IFileEntry.class);
            EasyMock.expect(childMockEntry.getName()).andStubReturn(pathSegments[i]);
            rootPath = rootPath + FileListingService.FILE_SEPARATOR + pathSegments[i];
            EasyMock.expect(childMockEntry.getFullEscapedPath()).andStubReturn(rootPath);
            Collection<IFileEntry> childrenResult = new ArrayList<IFileEntry>(1);
            childrenResult.add(childMockEntry);
            EasyMock.expect(rootEntry.getChildren(EasyMock.anyBoolean())).andStubReturn(
                    childrenResult);
            EasyMock.expect(rootEntry.isDirectory()).andStubReturn(Boolean.TRUE);
            EasyMock.replay(rootEntry);
            rootEntry = childMockEntry;
        }
        // leaf node - not a directory
        EasyMock.expect(rootEntry.isDirectory()).andStubReturn(Boolean.FALSE);
        EasyMock.replay(rootEntry);
    }


}
