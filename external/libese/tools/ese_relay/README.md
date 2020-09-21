# What?

ese-relay connects libese's functionality to a local abstract socket on
an Android device.  The primary purpose is to ease development and provision
with test hardware without bringing up all the development tools needed.

ese-relay uses the same wire protocol as the
[Virtual Smart Card](http://frankmorgner.github.io/vsmartcard/) project by acting
as the "viccd" service.  This enables use of any tool that supports
[pcsc-lite](https://pcsclite.alioth.debian.org/) without any additional
development.

# Wire protocol

The format is always
    Ln d0..dn
Ln is a network byte order 16-bit unsigned integer length of the data.
d0..dn are uint8_t bytes to tunneled directly to/from the card.

If Ln == 1, it indicates an out of band control message. Supported messages
are 1:0 - 1:4:
  * 0: Power off (ignored)
  * 1: Power on (implemented with a reset)
  * 3: Reset
  * 4: ATR - returns a fake ATR

# Prerequisites

  * pcscd is installed on your system.
  * Build and install https://frankmorgner.github.io/vsmartcard/virtualsmartcard
  * Configure /etc/reader.d/vpcd as below.
  * Build ese-relay configured for the hardware in use.

## /etc/reader.conf.d/vpcd:

    FRIENDLYNAME "Virtual PCD"
    DEVICENAME   localhost:0x1000
    LIBPATH      /usr/lib/pcsc/drivers/serial/libifdvpcd.so
    CHANNELID    0x1000

This will cause pcscd to connect to localhost port 4096 on start as per the vsmartcard
documentation.

# Usage
## In one terminal, run the service and forward the abstract UNIX socket to a local TCP socket:
  $ adb shell ese-relay-<hw>
  $ adb forward tcp:4096 localabstract:ese-relay

## In another terminal, restart pcscd configured with an appropriate reader:
  $ /etc/init.d/pcscd restart # (Or whatever your init system requires.)

## In yet another terminal, use your preferred* pcsc client:

  $ java -jar gp.jar -info

* https://github.com/martinpaljak/GlobalPlatformPro is used here.
