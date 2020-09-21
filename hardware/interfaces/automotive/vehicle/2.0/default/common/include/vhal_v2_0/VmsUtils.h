/*
 * Copyright (C) 2018 The Android Open Source Project
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

#ifndef android_hardware_automotive_vehicle_V2_0_VmsUtils_H_
#define android_hardware_automotive_vehicle_V2_0_VmsUtils_H_

#include <memory>
#include <string>

#include <android/hardware/automotive/vehicle/2.0/types.h>

namespace android {
namespace hardware {
namespace automotive {
namespace vehicle {
namespace V2_0 {
namespace vms {

// VmsUtils are a set of abstractions for creating and parsing Vehicle Property
// updates to VehicleProperty::VEHICLE_MAP_SERVICE. The format for parsing a
// VehiclePropValue update with a VMS message is specified in the Vehicle HIDL.
//
// This interface is meant for use by HAL clients of VMS; corresponding
// functionality is also provided by VMS in the embedded car service.

// A VmsLayer is comprised of a type, subtype, and version.
struct VmsLayer {
    VmsLayer(int type, int subtype, int version) : type(type), subtype(subtype), version(version) {}
    int type;
    int subtype;
    int version;
};

struct VmsLayerAndPublisher {
    VmsLayer layer;
    int publisher_id;
};

// A VmsAssociatedLayer is used by subscribers to specify which publisher IDs
// are acceptable for a given layer.
struct VmsAssociatedLayer {
    VmsLayer layer;
    std::vector<int> publisher_ids;
};

// A VmsLayerOffering refers to a single layer that can be published, along with
// its dependencies. Dependencies can be empty.
struct VmsLayerOffering {
    VmsLayerOffering(VmsLayer layer, std::vector<VmsLayer> dependencies)
        : layer(layer), dependencies(dependencies) {}
    VmsLayerOffering(VmsLayer layer) : layer(layer), dependencies() {}
    VmsLayer layer;
    std::vector<VmsLayer> dependencies;
};

// A VmsSubscriptionsState is delivered in response to a
// VmsMessageType.SUBSCRIPTIONS_REQUEST or on the first SUBSCRIBE or last
// UNSUBSCRIBE for a layer. It indicates which layers or associated_layers are
// currently being subscribed to in the system.
struct VmsSubscriptionsState {
    int sequence_number;
    std::vector<VmsLayer> layers;
    std::vector<VmsAssociatedLayer> associated_layers;
};

struct VmsAvailabilityState {
    int sequence_number;
    std::vector<VmsAssociatedLayer> associated_layers;
};

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.SUBSCRIBE, specifying to the VMS service
// which layer to subscribe to.
std::unique_ptr<VehiclePropValue> createSubscribeMessage(const VmsLayer& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.SUBSCRIBE_TO_PUBLISHER, specifying to the VMS service
// which layer and publisher_id to subscribe to.
std::unique_ptr<VehiclePropValue> createSubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.UNSUBSCRIBE, specifying to the VMS service
// which layer to unsubscribe from.
std::unique_ptr<VehiclePropValue> createUnsubscribeMessage(const VmsLayer& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.UNSUBSCRIBE_TO_PUBLISHER, specifying to the VMS service
// which layer and publisher_id to unsubscribe from.
std::unique_ptr<VehiclePropValue> createUnsubscribeToPublisherMessage(
    const VmsLayerAndPublisher& layer);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.OFFERING, specifying to the VMS service which layers are being
// offered and their dependencies, if any.
std::unique_ptr<VehiclePropValue> createOfferingMessage(
    const std::vector<VmsLayerOffering>& offering);

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.AVAILABILITY_REQUEST.
std::unique_ptr<VehiclePropValue> createAvailabilityRequest();

// Creates a VehiclePropValue containing a message of type
// VmsMessageType.AVAILABILITY_REQUEST.
std::unique_ptr<VehiclePropValue> createSubscriptionsRequest();

// Creates a VehiclePropValue containing a message of type VmsMessageType.DATA.
// Returns a nullptr if the byte string in bytes is empty.
//
// For example, to build a VehiclePropMessage containing a proto, the caller
// should convert the proto to a byte string using the SerializeToString proto
// API, then use this inteface to build the VehicleProperty.
std::unique_ptr<VehiclePropValue> createDataMessage(const std::string& bytes);

// Returns true if the VehiclePropValue pointed to by value contains a valid Vms
// message, i.e. the VehicleProperty, VehicleArea, and VmsMessageType are all
// valid. Note: If the VmsMessageType enum is extended, this function will
// return false for any new message types added.
bool isValidVmsMessage(const VehiclePropValue& value);

// Returns the message type. Expects that the VehiclePropValue contains a valid
// Vms message, as verified by isValidVmsMessage.
VmsMessageType parseMessageType(const VehiclePropValue& value);

// Constructs a string byte array from a message of type VmsMessageType.DATA.
// Returns an empty string if the message type doesn't match or if the
// VehiclePropValue does not contain a byte array.
//
// A proto message can then be constructed by passing the result of this
// function to ParseFromString.
std::string parseData(const VehiclePropValue& value);

// TODO(aditin): Need to implement additional parsing functions per message
// type.

}  // namespace vms
}  // namespace V2_0
}  // namespace vehicle
}  // namespace automotive
}  // namespace hardware
}  // namespace android

#endif  // android_hardware_automotive_vehicle_V2_0_VmsUtils_H_
