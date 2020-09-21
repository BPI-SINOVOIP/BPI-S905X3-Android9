/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: C++ file
 */

#define LOG_NDEBUG 0
#define LOG_CEE_TAG "IHdmiCecService"

#include "IHdmiCecService.h"

namespace android {


class BpHdmiCecService : public BpInterface<IHdmiCecService> {
public:
    BpHdmiCecService(const sp<IBinder> &impl)
        : BpInterface<IHdmiCecService>(impl)
    {}

    virtual int connect(const sp<IHdmiCecCallback> &client)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeStrongBinder(IInterface::asBinder(client));
        remote()->transact(CONNECT, data, &reply);
        return reply.readInt32();
    }

    virtual int openCecDevice()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(OPEN_CEC_DEVICE, data, &reply);
        return reply.readInt32();
    }

    virtual int closeCecDevice()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(CLOSE_CEC_DEVICE, data, &reply);
        return reply.readInt32();
    }

    virtual void getPortInfos(hdmi_port_info_t* list[], int* total)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(GET_PORT_INFO, data, &reply);
        *total = reply.readInt32();
        *list = (hdmi_port_info_t*)malloc(sizeof(struct hdmi_port_info) * (*total));
        for (int i = 0; i < *total; i++) {
            (*list)[i].type = (hdmi_port_type_t)reply.readInt32();
            (*list)[i].port_id = reply.readInt32();
            (*list)[i].cec_supported = reply.readInt32();
            (*list)[i].arc_supported = reply.readInt32();
            (*list)[i].physical_address = reply.readInt32();
        }
    }

    virtual int addLogicalAddress(cec_logical_address_t address)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeInt32(address);
        remote()->transact(ADD_LOGICAL_ADDRESS, data, &reply);
        return reply.readInt32();
    }

    virtual void clearLogicaladdress()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(CLEAR_LOGICAL_ADDRESS, data, &reply);
    }

    virtual void setOption(int flag, int value)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeInt32(flag);
        data.writeInt32(value);
        remote()->transact(SET_OPTION, data, &reply);
    }

    void setAudioReturnChannel(int port, bool flag)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeInt32(port);
        data.writeInt32(flag);
        remote()->transact(SET_AUDIO_RETURN_CHANNEL, data, &reply);
    }

    virtual bool isConnected(int port)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeInt32(port);
        remote()->transact(IS_CONNECT, data, &reply);
        return reply.readInt32();
    }

    virtual int getVersion(int* version)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(GET_VERSION, data, &reply);
        *version = reply.readInt32();
        return reply.readInt32();
    }

    virtual int getVendorId(uint32_t* vendorId)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(GET_VENDOR_ID, data, &reply);
        *vendorId = reply.readUint32();
        return reply.readInt32();
    }

    virtual int getPhysicalAddress(uint16_t* addr)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        remote()->transact(GET_PHYSICAL_ADDRESS, data, &reply);
        *addr = (uint16_t)reply.readUint32();
        return reply.readInt32();
    }

    virtual int sendMessage(const cec_message_t* message, bool isExtend)
    {
        Parcel data, reply;
        data.writeInterfaceToken(IHdmiCecService::getInterfaceDescriptor());
        data.writeInt32(isExtend);
        data.write((void*)message, sizeof(cec_message_t));
        remote()->transact(SEND_MESSAGE, data, &reply);
        return reply.readInt32();
    }

};

IMPLEMENT_META_INTERFACE(HdmiCecService, "droidlogic.IHdmiCecService");

//----------------------------------------------------------------------------------------

status_t BnHdmiCecService::onTransact(
    uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    switch (code) {
        case CONNECT: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            sp<IHdmiCecCallback> client = interface_cast<IHdmiCecCallback>(data.readStrongBinder());
            int ret = connect(client);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case OPEN_CEC_DEVICE: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int ret = openCecDevice();
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case CLOSE_CEC_DEVICE: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int ret = closeCecDevice();
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case GET_VERSION: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int version = 0;
            int ret = getVersion(&version);
            reply->writeInt32(version);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case GET_VENDOR_ID: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            uint32_t vendorId = 0;
            int ret = getVendorId(&vendorId);
            reply->writeUint32(vendorId);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case GET_PHYSICAL_ADDRESS: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            uint16_t address;
            int ret = getPhysicalAddress(&address);
            reply->writeUint32(address);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case SEND_MESSAGE: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            bool isExtend = data.readInt32();
            cec_message_t message;
            data.read((void *)&message, sizeof(cec_message_t));
            int ret = sendMessage(&message, isExtend);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case IS_CONNECT: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int port = data.readInt32();
            bool ret = isConnected(port);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case SET_AUDIO_RETURN_CHANNEL: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int port = data.readInt32();
            int flag = data.readInt32();
            setAudioReturnChannel(port, flag);
            return NO_ERROR;
        }
        case SET_OPTION: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            int flag = data.readInt32();
            int value = data.readInt32();
            setOption(flag, value);
            return NO_ERROR;
        }
        case CLEAR_LOGICAL_ADDRESS: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            clearLogicaladdress();
            return NO_ERROR;
        }
        case ADD_LOGICAL_ADDRESS: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            cec_logical_address_t addr = (cec_logical_address_t)data.readInt32();
            int ret = addLogicalAddress(addr);
            reply->writeInt32(ret);
            return NO_ERROR;
        }
        case GET_PORT_INFO: {
            CHECK_INTERFACE(IHdmiCecService, data, reply);
            hdmi_port_info_t* list;
            int total;
            getPortInfos(&list, &total);
            reply->writeInt32(total);
            for (int i = 0; i < total; i++) {
                reply->writeInt32(list[i].type);
                reply->writeInt32(list[i].port_id);
                reply->writeInt32(list[i].cec_supported);
                reply->writeInt32(list[i].arc_supported);
                reply->writeInt32(list[i].physical_address);
            }
            delete list;
            return NO_ERROR;
        }
        default: {
            return BBinder::onTransact(code, data, reply, flags);
        }
    }
    return NO_ERROR;
}


};//namespace android
