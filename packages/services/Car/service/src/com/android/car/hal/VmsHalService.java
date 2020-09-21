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
package com.android.car.hal;

import static com.android.car.CarServiceUtils.toByteArray;

import static java.lang.Integer.toHexString;

import android.annotation.SystemApi;
import android.car.VehicleAreaType;
import android.car.vms.IVmsSubscriberClient;
import android.car.vms.VmsAssociatedLayer;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.car.vms.VmsLayerDependency;
import android.car.vms.VmsLayersOffering;
import android.car.vms.VmsOperationRecorder;
import android.car.vms.VmsSubscriptionState;
import android.hardware.automotive.vehicle.V2_0.VehiclePropConfig;
import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.hardware.automotive.vehicle.V2_0.VehicleProperty;
import android.hardware.automotive.vehicle.V2_0.VmsBaseMessageIntegerValuesIndex;
import android.hardware.automotive.vehicle.V2_0.VmsMessageType;
import android.hardware.automotive.vehicle.V2_0.VmsMessageWithLayerAndPublisherIdIntegerValuesIndex;
import android.hardware.automotive.vehicle.V2_0.VmsMessageWithLayerIntegerValuesIndex;
import android.hardware.automotive.vehicle.V2_0.VmsOfferingMessageIntegerValuesIndex;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import com.android.car.CarLog;
import com.android.car.VmsLayersAvailability;
import com.android.car.VmsPublishersInfo;
import com.android.car.VmsRouting;
import com.android.internal.annotations.GuardedBy;

import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * This is a glue layer between the VehicleHal and the VmsService. It sends VMS properties back and
 * forth.
 */
@SystemApi
public class VmsHalService extends HalServiceBase {

    private static final boolean DBG = true;
    private static final int HAL_PROPERTY_ID = VehicleProperty.VEHICLE_MAP_SERVICE;
    private static final String TAG = "VmsHalService";

    private final static List<Integer> AVAILABILITY_MESSAGE_TYPES = Collections.unmodifiableList(
            Arrays.asList(
                    VmsMessageType.AVAILABILITY_RESPONSE,
                    VmsMessageType.AVAILABILITY_CHANGE));

    private boolean mIsSupported = false;
    private CopyOnWriteArrayList<VmsHalPublisherListener> mPublisherListeners =
            new CopyOnWriteArrayList<>();
    private CopyOnWriteArrayList<VmsHalSubscriberListener> mSubscriberListeners =
            new CopyOnWriteArrayList<>();

    private final IBinder mHalPublisherToken = new Binder();
    private final VehicleHal mVehicleHal;

    private final Object mLock = new Object();
    private final VmsRouting mRouting = new VmsRouting();
    @GuardedBy("mLock")
    private final Map<IBinder, VmsLayersOffering> mOfferings = new HashMap<>();
    @GuardedBy("mLock")
    private final VmsLayersAvailability mAvailableLayers = new VmsLayersAvailability();
    private final VmsPublishersInfo mPublishersInfo = new VmsPublishersInfo();

    /**
     * The VmsPublisherService implements this interface to receive data from the HAL.
     */
    public interface VmsHalPublisherListener {
        void onChange(VmsSubscriptionState subscriptionState);
    }

    /**
     * The VmsSubscriberService implements this interface to receive data from the HAL.
     */
    public interface VmsHalSubscriberListener {
        // Notifies the listener on a data Message from a publisher.
        void onDataMessage(VmsLayer layer, int publisherId, byte[] payload);

        // Notifies the listener on a change in available layers.
        void onLayersAvaiabilityChange(VmsAvailableLayers availableLayers);
    }

    /**
     * The VmsService implements this interface to receive data from the HAL.
     */
    protected VmsHalService(VehicleHal vehicleHal) {
        mVehicleHal = vehicleHal;
        if (DBG) {
            Log.d(TAG, "started VmsHalService!");
        }
    }

    public void addPublisherListener(VmsHalPublisherListener listener) {
        mPublisherListeners.add(listener);
    }

    public void addSubscriberListener(VmsHalSubscriberListener listener) {
        mSubscriberListeners.add(listener);
    }

    public void removePublisherListener(VmsHalPublisherListener listener) {
        mPublisherListeners.remove(listener);
    }

    public void removeSubscriberListener(VmsHalSubscriberListener listener) {
        mSubscriberListeners.remove(listener);
    }

    public void addSubscription(IVmsSubscriberClient listener, VmsLayer layer) {
        boolean firstSubscriptionForLayer = false;
        if (DBG) {
            Log.d(TAG, "Checking for first subscription. Layer: " + layer);
        }
        synchronized (mLock) {
            // Check if publishers need to be notified about this change in subscriptions.
            firstSubscriptionForLayer = !mRouting.hasLayerSubscriptions(layer);

            // Add the listeners subscription to the layer
            mRouting.addSubscription(listener, layer);
        }
        if (firstSubscriptionForLayer) {
            notifyHalPublishers(layer, true);
            notifyClientPublishers();
        }
    }

    public void removeSubscription(IVmsSubscriberClient listener, VmsLayer layer) {
        boolean layerHasSubscribers = true;
        synchronized (mLock) {
            if (!mRouting.hasLayerSubscriptions(layer)) {
                if (DBG) {
                    Log.d(TAG, "Trying to remove a layer with no subscription: " + layer);
                }
                return;
            }

            // Remove the listeners subscription to the layer
            mRouting.removeSubscription(listener, layer);

            // Check if publishers need to be notified about this change in subscriptions.
            layerHasSubscribers = mRouting.hasLayerSubscriptions(layer);
        }
        if (!layerHasSubscribers) {
            notifyHalPublishers(layer, false);
            notifyClientPublishers();
        }
    }

    public void addSubscription(IVmsSubscriberClient listener) {
        synchronized (mLock) {
            mRouting.addSubscription(listener);
        }
    }

    public void removeSubscription(IVmsSubscriberClient listener) {
        synchronized (mLock) {
            mRouting.removeSubscription(listener);
        }
    }

    public void addSubscription(IVmsSubscriberClient listener, VmsLayer layer, int publisherId) {
        boolean firstSubscriptionForLayer = false;
        synchronized (mLock) {
            // Check if publishers need to be notified about this change in subscriptions.
            firstSubscriptionForLayer = !(mRouting.hasLayerSubscriptions(layer) ||
                    mRouting.hasLayerFromPublisherSubscriptions(layer, publisherId));

            // Add the listeners subscription to the layer
            mRouting.addSubscription(listener, layer, publisherId);
        }
        if (firstSubscriptionForLayer) {
            notifyHalPublishers(layer, true);
            notifyClientPublishers();
        }
    }

    public void removeSubscription(IVmsSubscriberClient listener, VmsLayer layer, int publisherId) {
        boolean layerHasSubscribers = true;
        synchronized (mLock) {
            if (!mRouting.hasLayerFromPublisherSubscriptions(layer, publisherId)) {
                Log.i(TAG, "Trying to remove a layer with no subscription: " +
                        layer + ", publisher ID:" + publisherId);
                return;
            }

            // Remove the listeners subscription to the layer
            mRouting.removeSubscription(listener, layer, publisherId);

            // Check if publishers need to be notified about this change in subscriptions.
            layerHasSubscribers = mRouting.hasLayerSubscriptions(layer) ||
                    mRouting.hasLayerFromPublisherSubscriptions(layer, publisherId);
        }
        if (!layerHasSubscribers) {
            notifyHalPublishers(layer, false);
            notifyClientPublishers();
        }
    }

    public void removeDeadSubscriber(IVmsSubscriberClient listener) {
        synchronized (mLock) {
            mRouting.removeDeadSubscriber(listener);
        }
    }

    public Set<IVmsSubscriberClient> getSubscribersForLayerFromPublisher(VmsLayer layer,
                                                                         int publisherId) {
        synchronized (mLock) {
            return mRouting.getSubscribersForLayerFromPublisher(layer, publisherId);
        }
    }

    public Set<IVmsSubscriberClient> getAllSubscribers() {
        synchronized (mLock) {
            return mRouting.getAllSubscribers();
        }
    }

    public boolean isHalSubscribed(VmsLayer layer) {
        synchronized (mLock) {
            return mRouting.isHalSubscribed(layer);
        }
    }

    public VmsSubscriptionState getSubscriptionState() {
        synchronized (mLock) {
            return mRouting.getSubscriptionState();
        }
    }

    /**
     * Assigns an idempotent ID for publisherInfo and stores it. The idempotency in this case means
     * that the same publisherInfo will always, within a trip of the vehicle, return the same ID.
     * The publisherInfo should be static for a binary and should only change as part of a software
     * update. The publisherInfo is a serialized proto message which VMS clients can interpret.
     */
    public int getPublisherId(byte[] publisherInfo) {
        if (DBG) {
            Log.i(TAG, "Getting publisher static ID");
        }
        synchronized (mLock) {
            return mPublishersInfo.getIdForInfo(publisherInfo);
        }
    }

    public byte[] getPublisherInfo(int publisherId) {
        if (DBG) {
            Log.i(TAG, "Getting information for publisher ID: " + publisherId);
        }
        synchronized (mLock) {
            return mPublishersInfo.getPublisherInfo(publisherId);
        }
    }

    private void addHalSubscription(VmsLayer layer) {
        boolean firstSubscriptionForLayer = true;
        synchronized (mLock) {
            // Check if publishers need to be notified about this change in subscriptions.
            firstSubscriptionForLayer = !mRouting.hasLayerSubscriptions(layer);

            // Add the listeners subscription to the layer
            mRouting.addHalSubscription(layer);
        }
        if (firstSubscriptionForLayer) {
            notifyHalPublishers(layer, true);
            notifyClientPublishers();
        }
    }

    private void addHalSubscriptionToPublisher(VmsLayer layer, int publisherId) {
        boolean firstSubscriptionForLayer = true;
        synchronized (mLock) {
            // Check if publishers need to be notified about this change in subscriptions.
            firstSubscriptionForLayer = !(mRouting.hasLayerSubscriptions(layer) ||
                    mRouting.hasLayerFromPublisherSubscriptions(layer, publisherId));

            // Add the listeners subscription to the layer
            mRouting.addHalSubscriptionToPublisher(layer, publisherId);
        }
        if (firstSubscriptionForLayer) {
            notifyHalPublishers(layer, publisherId, true);
            notifyClientPublishers();
        }
    }

    private void removeHalSubscription(VmsLayer layer) {
        boolean layerHasSubscribers = true;
        synchronized (mLock) {
            if (!mRouting.hasLayerSubscriptions(layer)) {
                Log.i(TAG, "Trying to remove a layer with no subscription: " + layer);
                return;
            }

            // Remove the listeners subscription to the layer
            mRouting.removeHalSubscription(layer);

            // Check if publishers need to be notified about this change in subscriptions.
            layerHasSubscribers = mRouting.hasLayerSubscriptions(layer);
        }
        if (!layerHasSubscribers) {
            notifyHalPublishers(layer, false);
            notifyClientPublishers();
        }
    }

    public void removeHalSubscriptionFromPublisher(VmsLayer layer, int publisherId) {
        boolean layerHasSubscribers = true;
        synchronized (mLock) {
            if (!mRouting.hasLayerSubscriptions(layer)) {
                Log.i(TAG, "Trying to remove a layer with no subscription: " + layer);
                return;
            }

            // Remove the listeners subscription to the layer
            mRouting.removeHalSubscriptionToPublisher(layer, publisherId);

            // Check if publishers need to be notified about this change in subscriptions.
            layerHasSubscribers = mRouting.hasLayerSubscriptions(layer) ||
                    mRouting.hasLayerFromPublisherSubscriptions(layer, publisherId);
        }
        if (!layerHasSubscribers) {
            notifyHalPublishers(layer, publisherId, false);
            notifyClientPublishers();
        }
    }

    public boolean containsSubscriber(IVmsSubscriberClient subscriber) {
        synchronized (mLock) {
            return mRouting.containsSubscriber(subscriber);
        }
    }

    public void setPublisherLayersOffering(IBinder publisherToken, VmsLayersOffering offering) {
        synchronized (mLock) {
            updateOffering(publisherToken, offering);
            VmsOperationRecorder.get().setPublisherLayersOffering(offering);
        }
    }

    public VmsAvailableLayers getAvailableLayers() {
        synchronized (mLock) {
            return mAvailableLayers.getAvailableLayers();
        }
    }

    /**
     * Notify all the publishers and the HAL on subscription changes regardless of who triggered
     * the change.
     *
     * @param layer          layer which is being subscribed to or unsubscribed from.
     * @param hasSubscribers indicates if the notification is for subscription or unsubscription.
     */
    private void notifyHalPublishers(VmsLayer layer, boolean hasSubscribers) {
        // notify the HAL
        setSubscriptionRequest(layer, hasSubscribers);
    }

    private void notifyHalPublishers(VmsLayer layer, int publisherId, boolean hasSubscribers) {
        // notify the HAL
        setSubscriptionToPublisherRequest(layer, publisherId, hasSubscribers);
    }

    private void notifyClientPublishers() {
        // Notify the App publishers
        for (VmsHalPublisherListener listener : mPublisherListeners) {
            // Besides the list of layers, also a timestamp is provided to the clients.
            // They should ignore any notification with a timestamp that is older than the most
            // recent timestamp they have seen.
            listener.onChange(getSubscriptionState());
        }
    }

    /**
     * Notify all the subscribers and the HAL on layers availability change.
     *
     * @param availableLayers the layers which publishers claim they made publish.
     */
    private void notifyOfAvailabilityChange(VmsAvailableLayers availableLayers) {
        // notify the HAL
        notifyAvailabilityChangeToHal(availableLayers);

        // Notify the App subscribers
        for (VmsHalSubscriberListener listener : mSubscriberListeners) {
            listener.onLayersAvaiabilityChange(availableLayers);
        }
    }

    @Override
    public void init() {
        if (DBG) {
            Log.d(TAG, "init()");
        }
        if (mIsSupported) {
            mVehicleHal.subscribeProperty(this, HAL_PROPERTY_ID);
        }
    }

    @Override
    public void release() {
        if (DBG) {
            Log.d(TAG, "release()");
        }
        if (mIsSupported) {
            mVehicleHal.unsubscribeProperty(this, HAL_PROPERTY_ID);
        }
        mPublisherListeners.clear();
        mSubscriberListeners.clear();
    }

    @Override
    public Collection<VehiclePropConfig> takeSupportedProperties(
            Collection<VehiclePropConfig> allProperties) {
        List<VehiclePropConfig> taken = new LinkedList<>();
        for (VehiclePropConfig p : allProperties) {
            if (p.prop == HAL_PROPERTY_ID) {
                taken.add(p);
                mIsSupported = true;
                if (DBG) {
                    Log.d(TAG, "takeSupportedProperties: " + toHexString(p.prop));
                }
                break;
            }
        }
        return taken;
    }

    /**
     * Consumes/produces HAL messages. The format of these messages is defined in:
     * hardware/interfaces/automotive/vehicle/2.1/types.hal
     */
    @Override
    public void handleHalEvents(List<VehiclePropValue> values) {
        if (DBG) {
            Log.d(TAG, "Handling a VMS property change");
        }
        for (VehiclePropValue v : values) {
            ArrayList<Integer> vec = v.value.int32Values;
            int messageType = vec.get(VmsBaseMessageIntegerValuesIndex.MESSAGE_TYPE);

            if (DBG) {
                Log.d(TAG, "Handling VMS message type: " + messageType);
            }
            switch (messageType) {
                case VmsMessageType.DATA:
                    handleDataEvent(vec, toByteArray(v.value.bytes));
                    break;
                case VmsMessageType.SUBSCRIBE:
                    handleSubscribeEvent(vec);
                    break;
                case VmsMessageType.UNSUBSCRIBE:
                    handleUnsubscribeEvent(vec);
                    break;
                case VmsMessageType.SUBSCRIBE_TO_PUBLISHER:
                    handleSubscribeToPublisherEvent(vec);
                    break;
                case VmsMessageType.UNSUBSCRIBE_TO_PUBLISHER:
                    handleUnsubscribeFromPublisherEvent(vec);
                    break;
                case VmsMessageType.OFFERING:
                    handleOfferingEvent(vec);
                    break;
                case VmsMessageType.AVAILABILITY_REQUEST:
                    handleHalAvailabilityRequestEvent();
                    break;
                case VmsMessageType.SUBSCRIPTIONS_REQUEST:
                    handleSubscriptionsRequestEvent();
                    break;
                default:
                    throw new IllegalArgumentException("Unexpected message type: " + messageType);
            }
        }
    }

    private VmsLayer parseVmsLayerFromSimpleMessageIntegerValues(List<Integer> integerValues) {
        return new VmsLayer(integerValues.get(VmsMessageWithLayerIntegerValuesIndex.LAYER_TYPE),
                integerValues.get(VmsMessageWithLayerIntegerValuesIndex.LAYER_SUBTYPE),
                integerValues.get(VmsMessageWithLayerIntegerValuesIndex.LAYER_VERSION));
    }

    private VmsLayer parseVmsLayerFromDataMessageIntegerValues(List<Integer> integerValues) {
        return parseVmsLayerFromSimpleMessageIntegerValues(integerValues);
    }

    private int parsePublisherIdFromDataMessageIntegerValues(List<Integer> integerValues) {
        return integerValues.get(VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.PUBLISHER_ID);
    }


    /**
     * Data message format:
     * <ul>
     * <li>Message type.
     * <li>Layer id.
     * <li>Layer version.
     * <li>Layer subtype.
     * <li>Publisher ID.
     * <li>Payload.
     * </ul>
     */
    private void handleDataEvent(List<Integer> integerValues, byte[] payload) {
        VmsLayer vmsLayer = parseVmsLayerFromDataMessageIntegerValues(integerValues);
        int publisherId = parsePublisherIdFromDataMessageIntegerValues(integerValues);
        if (DBG) {
            Log.d(TAG, "Handling a data event for Layer: " + vmsLayer);
        }

        // Send the message.
        for (VmsHalSubscriberListener listener : mSubscriberListeners) {
            listener.onDataMessage(vmsLayer, publisherId, payload);
        }
    }

    /**
     * Subscribe message format:
     * <ul>
     * <li>Message type.
     * <li>Layer id.
     * <li>Layer version.
     * <li>Layer subtype.
     * </ul>
     */
    private void handleSubscribeEvent(List<Integer> integerValues) {
        VmsLayer vmsLayer = parseVmsLayerFromSimpleMessageIntegerValues(integerValues);
        if (DBG) {
            Log.d(TAG, "Handling a subscribe event for Layer: " + vmsLayer);
        }
        addHalSubscription(vmsLayer);
    }

    /**
     * Subscribe message format:
     * <ul>
     * <li>Message type.
     * <li>Layer id.
     * <li>Layer version.
     * <li>Layer subtype.
     * <li>Publisher ID
     * </ul>
     */
    private void handleSubscribeToPublisherEvent(List<Integer> integerValues) {
        VmsLayer vmsLayer = parseVmsLayerFromSimpleMessageIntegerValues(integerValues);
        if (DBG) {
            Log.d(TAG, "Handling a subscribe event for Layer: " + vmsLayer);
        }
        int publisherId =
                integerValues.get(VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.PUBLISHER_ID);
        addHalSubscriptionToPublisher(vmsLayer, publisherId);
    }

    /**
     * Unsubscribe message format:
     * <ul>
     * <li>Message type.
     * <li>Layer id.
     * <li>Layer version.
     * </ul>
     */
    private void handleUnsubscribeEvent(List<Integer> integerValues) {
        VmsLayer vmsLayer = parseVmsLayerFromSimpleMessageIntegerValues(integerValues);
        if (DBG) {
            Log.d(TAG, "Handling an unsubscribe event for Layer: " + vmsLayer);
        }
        removeHalSubscription(vmsLayer);
    }

    /**
     * Unsubscribe message format:
     * <ul>
     * <li>Message type.
     * <li>Layer id.
     * <li>Layer version.
     * </ul>
     */
    private void handleUnsubscribeFromPublisherEvent(List<Integer> integerValues) {
        VmsLayer vmsLayer = parseVmsLayerFromSimpleMessageIntegerValues(integerValues);
        int publisherId =
                integerValues.get(VmsMessageWithLayerAndPublisherIdIntegerValuesIndex.PUBLISHER_ID);
        if (DBG) {
            Log.d(TAG, "Handling an unsubscribe event for Layer: " + vmsLayer);
        }
        removeHalSubscriptionFromPublisher(vmsLayer, publisherId);
    }

    private static int NUM_INTEGERS_IN_VMS_LAYER = 3;

    private VmsLayer parseVmsLayerFromIndex(List<Integer> integerValues, int index) {
        int layerType = integerValues.get(index++);
        int layerSutype = integerValues.get(index++);
        int layerVersion = integerValues.get(index++);
        return new VmsLayer(layerType, layerSutype, layerVersion);
    }

    /**
     * Offering message format:
     * <ul>
     * <li>Message type.
     * <li>Publisher ID.
     * <li>Number of offerings.
     * <li>Each offering consists of:
     * <ul>
     * <li>Layer id.
     * <li>Layer version.
     * <li>Number of layer dependencies.
     * <li>Layer type/subtype/version.
     * </ul>
     * </ul>
     */
    private void handleOfferingEvent(List<Integer> integerValues) {
        int publisherId = integerValues.get(VmsOfferingMessageIntegerValuesIndex.PUBLISHER_ID);
        int numLayersDependencies =
                integerValues.get(
                        VmsOfferingMessageIntegerValuesIndex.NUMBER_OF_OFFERS);
        int idx = VmsOfferingMessageIntegerValuesIndex.OFFERING_START;

        Set<VmsLayerDependency> offeredLayers = new HashSet<>();

        // An offering is layerId, LayerVersion, LayerType, NumDeps, <LayerId, LayerVersion> X NumDeps.
        for (int i = 0; i < numLayersDependencies; i++) {
            VmsLayer offeredLayer = parseVmsLayerFromIndex(integerValues, idx);
            idx += NUM_INTEGERS_IN_VMS_LAYER;

            int numDependenciesForLayer = integerValues.get(idx++);
            if (numDependenciesForLayer == 0) {
                offeredLayers.add(new VmsLayerDependency(offeredLayer));
            } else {
                Set<VmsLayer> dependencies = new HashSet<>();

                for (int j = 0; j < numDependenciesForLayer; j++) {
                    VmsLayer dependantLayer = parseVmsLayerFromIndex(integerValues, idx);
                    idx += NUM_INTEGERS_IN_VMS_LAYER;
                    dependencies.add(dependantLayer);
                }
                offeredLayers.add(new VmsLayerDependency(offeredLayer, dependencies));
            }
        }
        // Store the HAL offering.
        VmsLayersOffering offering = new VmsLayersOffering(offeredLayers, publisherId);
        synchronized (mLock) {
            updateOffering(mHalPublisherToken, offering);
            VmsOperationRecorder.get().setHalPublisherLayersOffering(offering);
        }
    }

    /**
     * Availability message format:
     * <ul>
     * <li>Message type.
     * <li>Number of layers.
     * <li>Layer type/subtype/version.
     * </ul>
     */
    private void handleHalAvailabilityRequestEvent() {
        synchronized (mLock) {
            VmsAvailableLayers availableLayers = mAvailableLayers.getAvailableLayers();
            VehiclePropValue vehiclePropertyValue =
                    toAvailabilityUpdateVehiclePropValue(
                            availableLayers,
                            VmsMessageType.AVAILABILITY_RESPONSE);

            setPropertyValue(vehiclePropertyValue);
        }
    }

    /**
     * VmsSubscriptionRequestFormat:
     * <ul>
     * <li>Message type.
     * </ul>
     * <p>
     * VmsSubscriptionResponseFormat:
     * <ul>
     * <li>Message type.
     * <li>Sequence number.
     * <li>Number of layers.
     * <li>Layer type/subtype/version.
     * </ul>
     */
    private void handleSubscriptionsRequestEvent() {
        VmsSubscriptionState subscription = getSubscriptionState();
        VehiclePropValue vehicleProp =
                toTypedVmsVehiclePropValue(VmsMessageType.SUBSCRIPTIONS_RESPONSE);
        VehiclePropValue.RawValue v = vehicleProp.value;
        v.int32Values.add(subscription.getSequenceNumber());
        Set<VmsLayer> layers = subscription.getLayers();
        v.int32Values.add(layers.size());

        //TODO(asafro): get the real number of associated layers in the subscriptions
        //              state and send the associated layers themselves.
        v.int32Values.add(0);

        for (VmsLayer layer : layers) {
            v.int32Values.add(layer.getType());
            v.int32Values.add(layer.getSubtype());
            v.int32Values.add(layer.getVersion());
        }
        setPropertyValue(vehicleProp);
    }

    private void updateOffering(IBinder publisherToken, VmsLayersOffering offering) {
        VmsAvailableLayers availableLayers;
        synchronized (mLock) {
            mOfferings.put(publisherToken, offering);

            // Update layers availability.
            mAvailableLayers.setPublishersOffering(mOfferings.values());

            availableLayers = mAvailableLayers.getAvailableLayers();
        }
        notifyOfAvailabilityChange(availableLayers);
    }

    @Override
    public void dump(PrintWriter writer) {
        writer.println(TAG);
        writer.println("VmsProperty " + (mIsSupported ? "" : "not") + " supported.");
    }

    /**
     * Updates the VMS HAL property with the given value.
     *
     * @param layer          layer data to update the hal property.
     * @param hasSubscribers if it is a subscribe or unsubscribe message.
     * @return true if the call to the HAL to update the property was successful.
     */
    public boolean setSubscriptionRequest(VmsLayer layer, boolean hasSubscribers) {
        VehiclePropValue vehiclePropertyValue = toTypedVmsVehiclePropValueWithLayer(
                hasSubscribers ? VmsMessageType.SUBSCRIBE : VmsMessageType.UNSUBSCRIBE, layer);
        return setPropertyValue(vehiclePropertyValue);
    }

    public boolean setSubscriptionToPublisherRequest(VmsLayer layer,
                                                     int publisherId,
                                                     boolean hasSubscribers) {
        VehiclePropValue vehiclePropertyValue = toTypedVmsVehiclePropValueWithLayer(
                hasSubscribers ?
                        VmsMessageType.SUBSCRIBE_TO_PUBLISHER :
                        VmsMessageType.UNSUBSCRIBE_TO_PUBLISHER, layer);
        vehiclePropertyValue.value.int32Values.add(publisherId);
        return setPropertyValue(vehiclePropertyValue);
    }

    public boolean setDataMessage(VmsLayer layer, byte[] payload) {
        VehiclePropValue vehiclePropertyValue =
                toTypedVmsVehiclePropValueWithLayer(VmsMessageType.DATA, layer);
        VehiclePropValue.RawValue v = vehiclePropertyValue.value;
        v.bytes.ensureCapacity(payload.length);
        for (byte b : payload) {
            v.bytes.add(b);
        }
        return setPropertyValue(vehiclePropertyValue);
    }

    public boolean notifyAvailabilityChangeToHal(VmsAvailableLayers availableLayers) {
        VehiclePropValue vehiclePropertyValue =
                toAvailabilityUpdateVehiclePropValue(
                        availableLayers,
                        VmsMessageType.AVAILABILITY_CHANGE);

        return setPropertyValue(vehiclePropertyValue);
    }

    public boolean setPropertyValue(VehiclePropValue vehiclePropertyValue) {
        try {
            mVehicleHal.set(vehiclePropertyValue);
            return true;
        } catch (PropertyTimeoutException e) {
            Log.e(CarLog.TAG_PROPERTY, "set, property not ready 0x" + toHexString(HAL_PROPERTY_ID));
        }
        return false;
    }

    private static VehiclePropValue toTypedVmsVehiclePropValue(int messageType) {
        VehiclePropValue vehicleProp = new VehiclePropValue();
        vehicleProp.prop = HAL_PROPERTY_ID;
        vehicleProp.areaId = VehicleAreaType.VEHICLE_AREA_TYPE_GLOBAL;
        VehiclePropValue.RawValue v = vehicleProp.value;

        v.int32Values.add(messageType);
        return vehicleProp;
    }

    /**
     * Creates a {@link VehiclePropValue}
     */
    private static VehiclePropValue toTypedVmsVehiclePropValueWithLayer(
            int messageType, VmsLayer layer) {
        VehiclePropValue vehicleProp = toTypedVmsVehiclePropValue(messageType);
        VehiclePropValue.RawValue v = vehicleProp.value;
        v.int32Values.add(layer.getType());
        v.int32Values.add(layer.getSubtype());
        v.int32Values.add(layer.getVersion());
        return vehicleProp;
    }

    private static VehiclePropValue toAvailabilityUpdateVehiclePropValue(
            VmsAvailableLayers availableLayers, int messageType) {

        if (!AVAILABILITY_MESSAGE_TYPES.contains(messageType)) {
            throw new IllegalArgumentException("Unsupported availability type: " + messageType);
        }
        VehiclePropValue vehicleProp =
                toTypedVmsVehiclePropValue(messageType);
        populateAvailabilityPropValueFields(availableLayers, vehicleProp);
        return vehicleProp;

    }

    private static void populateAvailabilityPropValueFields(
            VmsAvailableLayers availableAssociatedLayers,
            VehiclePropValue vehicleProp) {
        VehiclePropValue.RawValue v = vehicleProp.value;
        v.int32Values.add(availableAssociatedLayers.getSequence());
        int numLayers = availableAssociatedLayers.getAssociatedLayers().size();
        v.int32Values.add(numLayers);
        for (VmsAssociatedLayer layer : availableAssociatedLayers.getAssociatedLayers()) {
            v.int32Values.add(layer.getVmsLayer().getType());
            v.int32Values.add(layer.getVmsLayer().getSubtype());
            v.int32Values.add(layer.getVmsLayer().getVersion());
            v.int32Values.add(layer.getPublisherIds().size());
            for (int publisherId : layer.getPublisherIds()) {
                v.int32Values.add(publisherId);
            }
        }
    }
}
