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

import android.car.vms.VmsAssociatedLayer;
import android.car.vms.VmsAvailableLayers;
import android.car.vms.VmsLayer;
import android.car.vms.VmsLayerDependency;
import android.car.vms.VmsLayersOffering;
import android.util.Log;
import com.android.internal.annotations.GuardedBy;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * Manages VMS availability for layers.
 * <p>
 * Each VMS publisher sets its layers offering which are a list of layers the publisher claims
 * it might publish. VmsLayersAvailability calculates from all the offering what are the
 * available layers.
 *
 * @hide
 */

public class VmsLayersAvailability {

    private static final boolean DBG = true;
    private static final String TAG = "VmsLayersAvailability";

    private final Object mLock = new Object();
    @GuardedBy("mLock")
    private final Map<VmsLayer, Set<Set<VmsLayer>>> mPotentialLayersAndDependencies =
            new HashMap<>();
    @GuardedBy("mLock")
    private Set<VmsAssociatedLayer> mAvailableAssociatedLayers = Collections.EMPTY_SET;
    @GuardedBy("mLock")
    private Set<VmsAssociatedLayer> mUnavailableAssociatedLayers = Collections.EMPTY_SET;
    @GuardedBy("mLock")
    private Map<VmsLayer, Set<Integer>> mPotentialLayersAndPublishers = new HashMap<>();
    @GuardedBy("mLock")
    private int mSeq = 0;

    /**
     * Setting the current layers offerings as reported by publishers.
     */
    public void setPublishersOffering(Collection<VmsLayersOffering> publishersLayersOfferings) {
        synchronized (mLock) {
            reset();

            for (VmsLayersOffering offering : publishersLayersOfferings) {
                for (VmsLayerDependency dependency : offering.getDependencies()) {
                    VmsLayer layer = dependency.getLayer();

                    // Associate publishers with layers.
                    Set<Integer> curPotentialLayerAndPublishers =
                            mPotentialLayersAndPublishers.get(layer);
                    if (curPotentialLayerAndPublishers == null) {
                        curPotentialLayerAndPublishers = new HashSet<>();
                        mPotentialLayersAndPublishers.put(layer, curPotentialLayerAndPublishers);
                    }
                    curPotentialLayerAndPublishers.add(offering.getPublisherId());

                    // Add dependencies for availability calculation.
                    Set<Set<VmsLayer>> curDependencies =
                            mPotentialLayersAndDependencies.get(layer);
                    if (curDependencies == null) {
                        curDependencies = new HashSet<>();
                        mPotentialLayersAndDependencies.put(layer, curDependencies);
                    }
                    curDependencies.add(dependency.getDependencies());
                }
            }
            calculateLayers();
        }
    }

    /**
     * Returns a collection of all the layers which may be published.
     */
    public VmsAvailableLayers getAvailableLayers() {
        synchronized (mLock) {
            return new VmsAvailableLayers(mAvailableAssociatedLayers, mSeq);
        }
    }

    private void reset() {
        synchronized (mLock) {
            mPotentialLayersAndDependencies.clear();
            mPotentialLayersAndPublishers.clear();
            mAvailableAssociatedLayers = Collections.EMPTY_SET;
            mUnavailableAssociatedLayers = Collections.EMPTY_SET;
            if (mSeq + 1 < mSeq) {
                throw new IllegalStateException("Sequence is about to loop");
            }
            mSeq += 1;
        }
    }

    private void calculateLayers() {
        synchronized (mLock) {
            Set<VmsLayer> availableLayersSet = new HashSet<>();
            Set<VmsLayer> cyclicAvoidanceAuxiliarySet = new HashSet<>();

            for (VmsLayer layer : mPotentialLayersAndDependencies.keySet()) {
                addLayerToAvailabilityCalculationLocked(layer,
                        availableLayersSet,
                        cyclicAvoidanceAuxiliarySet);
            }

            mAvailableAssociatedLayers = Collections.unmodifiableSet(
                    availableLayersSet
                            .stream()
                            .map(l -> new VmsAssociatedLayer(l, mPotentialLayersAndPublishers.get(l)))
                            .collect(Collectors.toSet()));

            mUnavailableAssociatedLayers = Collections.unmodifiableSet(
                    mPotentialLayersAndDependencies.keySet()
                            .stream()
                            .filter(l -> !availableLayersSet.contains(l))
                            .map(l -> new VmsAssociatedLayer(l, mPotentialLayersAndPublishers.get(l)))
                            .collect(Collectors.toSet()));
        }
    }

    @GuardedBy("mLock")
    private void addLayerToAvailabilityCalculationLocked(VmsLayer layer,
                                                         Set<VmsLayer> currentAvailableLayers,
                                                         Set<VmsLayer> cyclicAvoidanceSet) {
        if (DBG) {
            Log.d(TAG, "addLayerToAvailabilityCalculationLocked: checking layer: " + layer);
        }
        // If we already know that this layer is supported then we are done.
        if (currentAvailableLayers.contains(layer)) {
            return;
        }
        // If there is no offering for this layer we're done.
        if (!mPotentialLayersAndDependencies.containsKey(layer)) {
            return;
        }
        // Avoid cyclic dependency.
        if (cyclicAvoidanceSet.contains(layer)) {
            Log.e(TAG, "Detected a cyclic dependency: " + cyclicAvoidanceSet + " -> " + layer);
            return;
        }
        // A layer may have multiple dependency sets. The layer is available if any dependency
        // set is satisfied
        for (Set<VmsLayer> dependencies : mPotentialLayersAndDependencies.get(layer)) {
            // If layer does not have any dependencies then add to supported.
            if (dependencies == null || dependencies.isEmpty()) {
                currentAvailableLayers.add(layer);
                return;
            }
            // Add the layer to cyclic avoidance set
            cyclicAvoidanceSet.add(layer);

            boolean isSupported = true;
            for (VmsLayer dependency : dependencies) {
                addLayerToAvailabilityCalculationLocked(dependency,
                        currentAvailableLayers,
                        cyclicAvoidanceSet);

                if (!currentAvailableLayers.contains(dependency)) {
                    isSupported = false;
                    break;
                }
            }
            cyclicAvoidanceSet.remove(layer);

            if (isSupported) {
                currentAvailableLayers.add(layer);
                return;
            }
        }
        return;
    }
}
