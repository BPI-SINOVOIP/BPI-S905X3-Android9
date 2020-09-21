# NXP PN80T/PN81A support

libese support for the PN80T series of embedded secure elements is
implemented using a common set of shared hardware functions and
communication specific behavior.

The common behavior is found in "common.h".  Support for using Linux
spidev іs in "linux\_spidev.c", and support for using a simple nq-nci
associated kernel driver is in "nq\_nci.c".

# Implementing a new backend

When implementing a new backend, the required header is:

    #include "../include/ese/hw/nxp/pn80t/common.h"

Once included, the implementation must provide its own
receive and transmit libese-hw functions and a struct Pn80tPlatform,
defined in "../include/ese/hw/nxp/pn80t/platform.h".

Platform requires a dedicated initialize, release, wait functions as
well support for controlling NFC\_VEN, SVDD\_PWR\_REQ, and reset.

See the bottom of the other implementations for an example of the
required exports.

Any other functionality, such as libese-sysdeps, may also need to be
provided separately.
