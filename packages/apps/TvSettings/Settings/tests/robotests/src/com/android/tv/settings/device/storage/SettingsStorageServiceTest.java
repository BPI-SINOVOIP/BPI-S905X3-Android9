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

package com.android.tv.settings.device.storage;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Intent;
import android.os.storage.DiskInfo;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.support.annotation.NonNull;
import android.text.TextUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentMatcher;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.Spy;
import org.robolectric.RobolectricTestRunner;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

@RunWith(RobolectricTestRunner.class)
public class SettingsStorageServiceTest {

    @Mock
    private StorageManager mMockStorageManager;

    @Spy
    private SettingsStorageService.Impl mSettingsStorageServiceImplSpy;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        doNothing().when(mSettingsStorageServiceImplSpy).sendLocalBroadcast(any(Intent.class));
        doReturn(mMockStorageManager).when(mSettingsStorageServiceImplSpy)
                .getSystemService(StorageManager.class);
    }

    @Test
    public void testFormatAsPublic_success_fromPrivate() {
        final VolumeInfo volumeInfo = mock(VolumeInfo.class);
        doReturn("asdf").when(volumeInfo).getDiskId();
        doReturn(VolumeInfo.TYPE_PRIVATE).when(volumeInfo).getType();
        doReturn("jkl;").when(volumeInfo).getFsUuid();

        final List<VolumeInfo> volumeInfos = new ArrayList<>(1);
        volumeInfos.add(volumeInfo);
        doReturn(volumeInfos).when(mMockStorageManager).getVolumes();

        final Intent formatIntent = new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC);
        formatIntent.putExtra(DiskInfo.EXTRA_DISK_ID, "asdf");

        mSettingsStorageServiceImplSpy.onHandleIntent(formatIntent);

        verify(mMockStorageManager, atLeastOnce()).forgetVolume("jkl;");
        verify(mSettingsStorageServiceImplSpy, atLeastOnce()).sendLocalBroadcast(argThat(
                new IntentMatcher(new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC)){
                    @Override
                    public boolean matches(Intent argument) {
                        return super.matches(argument)
                                && TextUtils.equals(argument.getStringExtra(DiskInfo.EXTRA_DISK_ID),
                                "asdf")
                                && argument.getBooleanExtra(SettingsStorageService.EXTRA_SUCCESS,
                                false);
                    }
                }));
    }

    @Test
    public void testFormatAsPublic_success_fromNonPrivate() {
        final VolumeInfo volumeInfo = mock(VolumeInfo.class);
        doReturn("asdf").when(volumeInfo).getDiskId();
        doReturn(VolumeInfo.TYPE_PUBLIC).when(volumeInfo).getType();
        doReturn("jkl;").when(volumeInfo).getFsUuid();

        final List<VolumeInfo> volumeInfos = new ArrayList<>(1);
        volumeInfos.add(volumeInfo);
        doReturn(volumeInfos).when(mMockStorageManager).getVolumes();

        final Intent formatIntent = new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC);
        formatIntent.putExtra(DiskInfo.EXTRA_DISK_ID, "asdf");

        mSettingsStorageServiceImplSpy.onHandleIntent(formatIntent);

        verify(mMockStorageManager, never()).forgetVolume("jkl;");
        verify(mSettingsStorageServiceImplSpy, atLeastOnce()).sendLocalBroadcast(argThat(
                new IntentMatcher(new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC)){
                    @Override
                    public boolean matches(Intent argument) {
                        return super.matches(argument)
                                && TextUtils.equals(argument.getStringExtra(DiskInfo.EXTRA_DISK_ID),
                                "asdf")
                                && argument.getBooleanExtra(SettingsStorageService.EXTRA_SUCCESS,
                                false);
                    }
                }));
    }

    @Test
    public void testFormatAsPublic_failure() {
        final List<VolumeInfo> volumeInfos = new ArrayList<>(0);
        doReturn(volumeInfos).when(mMockStorageManager).getVolumes();

        final Intent formatIntent = new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC);
        formatIntent.putExtra(DiskInfo.EXTRA_DISK_ID, "asdf");

        doThrow(new RuntimeException("Expected failure")).when(mMockStorageManager)
                .partitionPublic(anyString());

        mSettingsStorageServiceImplSpy.onHandleIntent(formatIntent);

        verify(mSettingsStorageServiceImplSpy, atLeastOnce()).sendLocalBroadcast(argThat(
                new IntentMatcher(new Intent(SettingsStorageService.ACTION_FORMAT_AS_PUBLIC)){
                    @Override
                    public boolean matches(Intent argument) {
                        return super.matches(argument)
                                && TextUtils.equals(argument.getStringExtra(DiskInfo.EXTRA_DISK_ID),
                                "asdf")
                                && !argument.getBooleanExtra(SettingsStorageService.EXTRA_SUCCESS,
                                true);
                    }
                }));
    }

    @Test
    public void testFormatAsPrivate_success() {
        final VolumeInfo volumeInfo = mock(VolumeInfo.class);
        doReturn(VolumeInfo.TYPE_PRIVATE).when(volumeInfo).getType();
        doReturn("asdf").when(volumeInfo).getDiskId();
        doReturn("jkl;").when(volumeInfo).getId();

        final List<VolumeInfo> volumeInfos = new ArrayList<>(1);
        volumeInfos.add(volumeInfo);
        doReturn(volumeInfos).when(mMockStorageManager).getVolumes();

        doReturn(123L).when(mMockStorageManager).benchmark("private");
        doReturn(456L).when(mMockStorageManager).benchmark("jkl;");

        final Intent formatIntent = new Intent(SettingsStorageService.ACTION_FORMAT_AS_PRIVATE);
        formatIntent.putExtra(DiskInfo.EXTRA_DISK_ID, "asdf");

        mSettingsStorageServiceImplSpy.onHandleIntent(formatIntent);

        verify(mSettingsStorageServiceImplSpy, atLeastOnce()).sendLocalBroadcast(argThat(
                new IntentMatcher(new Intent(SettingsStorageService.ACTION_FORMAT_AS_PRIVATE)){
                    @Override
                    public boolean matches(Intent argument) {
                        return super.matches(argument)
                                && TextUtils.equals(argument.getStringExtra(DiskInfo.EXTRA_DISK_ID),
                                "asdf")
                                && Objects.equals(argument.getLongExtra(
                                        SettingsStorageService.EXTRA_INTERNAL_BENCH, 0), 123L)
                                && Objects.equals(argument.getLongExtra(
                                        SettingsStorageService.EXTRA_PRIVATE_BENCH, 0), 456L)
                                && argument.getBooleanExtra(SettingsStorageService.EXTRA_SUCCESS,
                                false);
                    }
                }));
    }

    @Test
    public void testFormatAsPrivate_failure() {
        final Intent formatIntent = new Intent(SettingsStorageService.ACTION_FORMAT_AS_PRIVATE);
        formatIntent.putExtra(DiskInfo.EXTRA_DISK_ID, "asdf");

        doThrow(new RuntimeException("Expected failure")).when(mMockStorageManager)
                .partitionPrivate(anyString());

        mSettingsStorageServiceImplSpy.onHandleIntent(formatIntent);

        verify(mSettingsStorageServiceImplSpy, atLeastOnce()).sendLocalBroadcast(argThat(
                new IntentMatcher(new Intent(SettingsStorageService.ACTION_FORMAT_AS_PRIVATE)){
                    @Override
                    public boolean matches(Intent argument) {
                        return super.matches(argument)
                                && TextUtils.equals(argument.getStringExtra(DiskInfo.EXTRA_DISK_ID),
                                "asdf")
                                && !argument.getBooleanExtra(SettingsStorageService.EXTRA_SUCCESS,
                                true);
                    }
                }));
    }

    private static class IntentMatcher implements ArgumentMatcher<Intent> {

        private final Intent mIntent;

        private IntentMatcher(@NonNull Intent intent) {
            mIntent = intent;
        }

        @Override
        public boolean matches(Intent argument) {
            return mIntent.filterEquals(argument);
        }

        @Override
        public String toString() {
            return "Expected: " + mIntent;
        }
    }
}
