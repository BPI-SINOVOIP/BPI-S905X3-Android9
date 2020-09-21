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

package com.android.car;

import static org.junit.Assert.assertTrue;

import android.car.vms.VmsLayer;
import android.car.vms.VmsLayerDependency;
import android.car.vms.VmsLayersOffering;
import android.car.vms.VmsOperationRecorder;
import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.HashSet;

@RunWith(AndroidJUnit4.class)
@MediumTest
public class VmsOperationRecorderTest {

    /**
     * Capture messages that VmsOperationRecorder.Writer would normally pass to Log.d(...).
     */
    class TestWriter extends VmsOperationRecorder.Writer {
        public String mMsg;

        @Override
        public boolean isEnabled() {
            return true;
        }

        @Override
        public void write(String msg) {
            super.write(msg);
            mMsg = msg;
        }
    }

    private TestWriter mWriter;
    private VmsOperationRecorder mRecorder;
    private static final String TAG = "VmsOperationRecorderTest";

    private static final VmsLayer layer1 = new VmsLayer(1, 3, 2);
    private static final VmsLayer layer2 = new VmsLayer(2, 4, 3);
    private static final VmsLayer layer3 = new VmsLayer(3, 5, 4);

    private static final VmsLayerDependency layerDependency1 = new VmsLayerDependency(layer3);
    private static final VmsLayerDependency layerDependency2 = new VmsLayerDependency(layer1,
            new HashSet<VmsLayer>(Arrays.asList(layer2, layer3)));

    private static final VmsLayersOffering layersOffering0 = new VmsLayersOffering(
            new HashSet<VmsLayerDependency>(), 66);
    private static final VmsLayersOffering layersOffering1 = new VmsLayersOffering(
            new HashSet<>(Arrays.asList(layerDependency1)), 66);
    private static final VmsLayersOffering layersOffering2 = new VmsLayersOffering(
            new HashSet<>(Arrays.asList(layerDependency1, layerDependency2)), 66);

    @Before
    public void setUp() {
        mWriter = new TestWriter();
        mRecorder = new VmsOperationRecorder(mWriter);
    }

    @Test
    public void testSubscribe() throws Exception {
        mRecorder.subscribe(layer1);
        assertJsonMsgEquals("{'subscribe':{'layer':{'subtype':3,'type':1,'version':2}}}");
    }

    @Test
    public void testUnsubscribe() throws Exception {
        mRecorder.unsubscribe(layer1);
        assertJsonMsgEquals("{'unsubscribe':{'layer':{'type':1,'subtype':3,'version':2}}}");
    }

    @Test
    public void testStartMonitoring() throws Exception {
        mRecorder.startMonitoring();
        assertJsonMsgEquals("{'startMonitoring':{}}");
    }

    @Test
    public void testStopMonitoring() throws Exception {
        mRecorder.stopMonitoring();
        assertJsonMsgEquals("{'stopMonitoring':{}}");
    }

    @Test
    public void testSetLayersOffering0() throws Exception {
        mRecorder.setLayersOffering(layersOffering0);
        assertJsonMsgEquals("{'setLayersOffering':{}}");
    }

    @Test
    public void testSetLayersOffering2() throws Exception {
        mRecorder.setLayersOffering(layersOffering2);
        assertJsonMsgEquals("{'setLayersOffering':{'layerDependency':["
                + "{'layer':{'type':3,'subtype':5,'version':4}},"
                + "{'layer':{'type':1,'subtype':3,'version':2},'dependency':["
                + "{'type':2,'subtype':4,'version':3},{'type':3,'subtype':5,'version':4}]}"
                + "]}}");
    }

    @Test
    public void testGetPublisherId() throws Exception {
        mRecorder.getPublisherId(9);
        assertJsonMsgEquals("{'getPublisherId':{'publisherId':9}}");
    }

    @Test
    public void testAddSubscription() throws Exception {
        mRecorder.addSubscription(42, layer1);
        assertJsonMsgEquals(
                "{'addSubscription':{'sequenceNumber':42,'layer':{'type':1,'subtype':3,'version':2}}}"
        );
    }

    @Test
    public void testRemoveSubscription() throws Exception {
        mRecorder.removeSubscription(42, layer1);
        assertJsonMsgEquals("{'removeSubscription':"
                + "{'sequenceNumber':42,'layer':{'type':1,'subtype':3,'version':2}}}");
    }

    @Test
    public void testAddPromiscuousSubscription() throws Exception {
        mRecorder.addPromiscuousSubscription(42);
        assertJsonMsgEquals("{'addPromiscuousSubscription':{'sequenceNumber':42}}");
    }

    @Test
    public void testRemovePromiscuousSubscription() throws Exception {
        mRecorder.removePromiscuousSubscription(42);
        assertJsonMsgEquals("{'removePromiscuousSubscription':{'sequenceNumber':42}}");
    }

    @Test
    public void testAddHalSubscription() throws Exception {
        mRecorder.addHalSubscription(42, layer1);
        assertJsonMsgEquals("{'addHalSubscription':"
                + "{'sequenceNumber':42,'layer':{'type':1,'subtype':3,'version':2}}}");
    }

    @Test
    public void testRemoveHalSubscription() throws Exception {
        mRecorder.removeHalSubscription(42, layer1);
        assertJsonMsgEquals("{'removeHalSubscription':"
                + "{'sequenceNumber':42,'layer':{'type':1,'subtype':3,'version':2}}}");
    }

    @Test
    public void testSetPublisherLayersOffering() throws Exception {
        mRecorder.setPublisherLayersOffering(layersOffering1);
        assertJsonMsgEquals("{'setPublisherLayersOffering':{'layerDependency':["
                + "{'layer':{'type':3,'subtype':5,'version':4}}]}}");
    }

    @Test public void testSetHalPublisherLayersOffering() throws Exception {
        mRecorder.setHalPublisherLayersOffering(layersOffering1);
        assertJsonMsgEquals("{'setHalPublisherLayersOffering':{'layerDependency':["
                + "{'layer':{'type':3,'subtype':5,'version':4}}]}}");
    }

    @Test
    public void testSubscribeToPublisher() throws Exception {
        mRecorder.subscribe(layer1, 99);
        assertJsonMsgEquals(
                "{'subscribe':{'publisherId':99, 'layer':{'type':1,'subtype':3,'version':2}}}");
    }

    @Test
    public void testUnsubscribeToPublisher() throws Exception {
        mRecorder.unsubscribe(layer1, 99);
        assertJsonMsgEquals(
                "{'unsubscribe':{'publisherId':99, 'layer':{'type':1,'subtype':3,'version':2}}}}");
    }

    private void assertJsonMsgEquals(String expectJson) throws Exception {
        // Escaping double quotes in a JSON string is really noisy. The test data uses single
        // quotes instead, which gets replaced here.
        JSONObject expect = new JSONObject(expectJson.replace("'", "\""));
        JSONObject got = new JSONObject(mWriter.mMsg);
        assertTrue(similar(expect, got));
    }

    /*
     * Determine if two JSONObjects are similar.
     * They must contain the same set of names which must be associated with
     * similar values.
     */
    private boolean similar(JSONObject expect, JSONObject got) {
        try {
            if (!expect.keySet().equals(got.keySet())) {
                return false;
            }

            for (String key : expect.keySet()) {
                Object valueExpect = expect.get(key);
                Object valueGot = got.get(key);

                if (valueExpect == valueGot) {
                    continue;
                }

                if (valueExpect == null) {
                    return false;
                }

                if (valueExpect instanceof JSONObject) {
                    return similar((JSONObject) valueExpect, (JSONObject) valueGot);
                } else if (valueExpect instanceof JSONArray) {
                    // Equal JSONArray have the same length and one contains the other.
                    JSONArray expectArray = (JSONArray) valueExpect;
                    JSONArray gotArray = (JSONArray) valueGot;

                    if (expectArray.length() != gotArray.length()) {
                        return false;
                    }

                    for (int i = 0; i < expectArray.length(); i++) {
                        boolean gotContainsSimilar = false;
                        for (int j = 0; j < gotArray.length(); j++) {
                            if (similar((JSONObject) expectArray.get(i),
                                    (JSONObject) gotArray.get(j))) {
                                gotContainsSimilar = true;
                                break;
                            }
                        }
                        if (!gotContainsSimilar) {
                            return false;
                        }
                    }
                } else if (!valueExpect.equals(valueGot)) {
                    return false;
                }
            }

        } catch (JSONException e) {
            Log.d(TAG, "Could not compare JSONObjects: " + e);
            return false;
        }
        return true;
    }
}
