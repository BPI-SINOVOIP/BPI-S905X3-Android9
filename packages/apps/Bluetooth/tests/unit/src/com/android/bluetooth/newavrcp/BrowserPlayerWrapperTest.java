/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.bluetooth.avrcp;

import static org.mockito.Mockito.*;

import android.media.MediaDescription;
import android.media.browse.MediaBrowser.MediaItem;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class BrowserPlayerWrapperTest {

    @Captor ArgumentCaptor<MediaBrowser.ConnectionCallback> mBrowserConnCb;
    @Captor ArgumentCaptor<MediaBrowser.SubscriptionCallback> mSubscriptionCb;
    @Captor ArgumentCaptor<List<ListItem>> mWrapperBrowseCb;
    @Mock MediaBrowser mMockBrowser;
    @Mock BrowsedPlayerWrapper.ConnectionCallback mConnCb;
    @Mock BrowsedPlayerWrapper.BrowseCallback mBrowseCb;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        when(mMockBrowser.getRoot()).thenReturn("root_folder");

        MediaBrowserFactory.inject(mMockBrowser);
    }

    @Test
    public void testWrap() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        wrapper.connect(mConnCb);
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        verify(mMockBrowser).connect();

        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();
        browserConnCb.onConnected();

        verify(mConnCb).run(eq(BrowsedPlayerWrapper.STATUS_SUCCESS), eq(wrapper));
        verify(mMockBrowser).disconnect();
    }

    @Test
    public void testConnect() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        wrapper.connect(mConnCb);
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();
        browserConnCb.onConnectionFailed();

        verify(mConnCb).run(eq(BrowsedPlayerWrapper.STATUS_CONN_ERROR), eq(wrapper));

        wrapper.connect(mConnCb);
        verify(mMockBrowser, times(2)).connect();

        browserConnCb.onConnected();
        verify(mConnCb).run(eq(BrowsedPlayerWrapper.STATUS_SUCCESS), eq(wrapper));
        verify(mMockBrowser, times(2)).disconnect();
    }

    @Test
    public void testEmptyRoot() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");

        doReturn("").when(mMockBrowser).getRoot();

        wrapper.connect(mConnCb);
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();

        verify(mMockBrowser, times(1)).connect();

        browserConnCb.onConnected();
        verify(mConnCb).run(eq(BrowsedPlayerWrapper.STATUS_CONN_ERROR), eq(wrapper));
        verify(mMockBrowser, times(1)).disconnect();
    }

    @Test
    public void testDisconnect() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        wrapper.connect(mConnCb);
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();
        browserConnCb.onConnected();
        verify(mMockBrowser).disconnect();
    }

    @Test
    public void testGetRootId() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        wrapper.connect(mConnCb);
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();
        browserConnCb.onConnected();

        Assert.assertEquals("root_folder", wrapper.getRootId());
        verify(mMockBrowser).disconnect();
    }

    @Test
    public void testPlayItem() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();

        wrapper.playItem("test_item");
        verify(mMockBrowser, times(1)).connect();

        MediaController mockController = mock(MediaController.class);
        MediaController.TransportControls mockTransport =
                mock(MediaController.TransportControls.class);
        when(mockController.getTransportControls()).thenReturn(mockTransport);
        MediaControllerFactory.inject(mockController);

        browserConnCb.onConnected();
        verify(mockTransport).playFromMediaId(eq("test_item"), eq(null));
        verify(mMockBrowser).disconnect();
    }

    @Test
    public void testGetFolderItems() {
        BrowsedPlayerWrapper wrapper = BrowsedPlayerWrapper.wrap(null, "test", "test");
        verify(mMockBrowser).testInit(any(), any(), mBrowserConnCb.capture(), any());
        MediaBrowser.ConnectionCallback browserConnCb = mBrowserConnCb.getValue();

        wrapper.getFolderItems("test_folder", mBrowseCb);


        browserConnCb.onConnected();
        verify(mMockBrowser).subscribe(any(), mSubscriptionCb.capture());
        MediaBrowser.SubscriptionCallback subscriptionCb = mSubscriptionCb.getValue();

        ArrayList<MediaItem> items = new ArrayList<MediaItem>();
        MediaDescription.Builder bob = new MediaDescription.Builder();
        bob.setTitle("test_song1");
        bob.setMediaId("ts1");
        items.add(new MediaItem(bob.build(), 0));
        bob.setTitle("test_song2");
        bob.setMediaId("ts2");
        items.add(new MediaItem(bob.build(), 0));

        subscriptionCb.onChildrenLoaded("test_folder", items);
        verify(mMockBrowser).unsubscribe(eq("test_folder"));
        verify(mBrowseCb).run(eq(BrowsedPlayerWrapper.STATUS_SUCCESS), eq("test_folder"),
                mWrapperBrowseCb.capture());

        List<ListItem> item_list = mWrapperBrowseCb.getValue();
        for (int i = 0; i < item_list.size(); i++) {
            Assert.assertFalse(item_list.get(i).isFolder);
            Assert.assertEquals(item_list.get(i).song, Util.toMetadata(items.get(i)));
        }

        verify(mMockBrowser).disconnect();
    }
}
