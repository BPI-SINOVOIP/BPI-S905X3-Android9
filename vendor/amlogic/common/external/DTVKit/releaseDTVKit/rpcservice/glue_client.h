#ifndef GULE_CLIENT_H
#define GULE_CLIENT_H

#include <iostream>
typedef void (*SIGNAL_CB)(const std::string &signal, const std::string &data);
typedef void  (*DISPATCHDRAW_CB)(int32_t src_width, int32_t src_height, int32_t dst_x, int32_t dst_y, int32_t dst_width, int32_t dst_height, const uint8_t *data);
// Singleton mode, only used by getInstance
class Glue_client {

private:
    Glue_client();
    static Glue_client *p_client;

public:
    int setSignalCallback(SIGNAL_CB cb);
    int setDisPatchDrawCallback(DISPATCHDRAW_CB cb);
    int RegisterRWSysfsCallback(void *readCb, void *writeCb);
    int UnRegisterRWSysfsCallback(void);
    int RegisterRWPropCallback(void *readCb, void *writeCb);
    int UnRegisterRWPropCallback(void);
    int addInterface(void);
    int SetSurface(int path, void *surface);
    std::string request(const std::string &resource, const std::string &json);
    static Glue_client* getInstance()
    {
        return p_client;
    }
    void dispatchDraw(int32_t src_width, int32_t src_height, int32_t dst_x, int32_t dst_y, int32_t dst_width, int32_t dst_height, const uint8_t *data);
    void dispatchSignal(const std::string &signal, const std::string &data);

private:
    SIGNAL_CB signal_callback = NULL;
    DISPATCHDRAW_CB dispatchDraw_callback = NULL;
};
// no need add lock for new client.we new client first
Glue_client* Glue_client::p_client = new Glue_client();

#endif //GULE_CLIENT_H