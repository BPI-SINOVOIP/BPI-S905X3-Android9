/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: header file
 */

#ifndef _C_TV_GPIO_H_
#define _C_TV_GPIO_H_

#define GPIO_NAME_TO_PIN      "/sys/class/aml_gpio/name_to_pin"
#define GPIO_EXPORT           "/sys/class/gpio/export"
#define GPIO_UNEXPORT         "/sys/class/gpio/unexport"
#define GPIO_DIRECTION(x, y)  sprintf(x, "/sys/class/gpio/gpio%d/direction", y)
#define GPIO_VALUE(x, y)      sprintf(x, "/sys/class/gpio/gpio%d/value", y)


class CTvGpio {
public:
    CTvGpio();
    ~CTvGpio();
    /**
     * @description set/get gpio status or level
     * @param port_name name of gpio
     * @param is_out true gpio out, false gpio in
     * @param edge 1 high, 0 low
     */
    int processCommand(const char *port_name, bool is_out, int edge);

private:
    int setGpioOutEdge(int edge);
    int getGpioInEdge();
    bool needExportAgain(char *path);

    int mGpioPinNum;
    char mGpioName[64];
};

#endif
