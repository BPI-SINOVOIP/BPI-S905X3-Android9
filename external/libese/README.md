# libese

Document last updated: 13 Jan 2017

## Introduction

libese provides a minimal transport wrapper for communicating with
embedded secure elements. Embedded secure elements typically adhere
to smart card standards whose translation is not always smooth when
migrated to an always connected bus, like SPI.  The interfaces
exposed by libese should enable higher level "terminal" implementations
to be written on top and/or a service which provides a similar
interface.

Behind the interface, libese should help smooth over the differences
between eSEs and smart cards use in the hardware adapter
implementations. Additionally, a T=1 implementation is supplied,
as it appears to be the most common wire transport for these chips.

## Usage

Public client interface for Embedded Secure Elements.

Prior to use in a file, import all necessary variables with:

    ESE_INCLUDE_HW(SOME_HAL_IMPL);

Instantiate in a function with:

    ESE_DECLARE(my_ese, SOME_HAL_IMPL);

or

    struct EseInterface my_ese = ESE_INITIALIZER(SOME_HAL_IMPL);

or

    struct EseInterface *my_ese = malloc(sizeof(struct EseInterface));
    ...
    ese_init(my_ese, SOME_HAL_IMPL);

To initialize the hardware abstraction, call:

    ese_open(my_ese);

To release any claimed resources, call

    ese_close(my_ese)

when interface use is complete.

To perform a transmit-receive cycle, call

    ese_transceive(my_ese, ...);

with a filled transmit buffer with total data length and
an empty receive buffer and a maximum fill length.
A negative return value indicates an error and a hardware
specific code and string may be collected with calls to

    ese_error_code(my_ese);
    ese_error_message(my_ese);

The EseInterface is not safe for concurrent access.
(Patches welcome! ;).

# Components

libese is broken into multiple pieces:
  * libese
  * libese-sysdeps
  * libese-hw
  * libese-teq1

*libese* provides the headers and wrappers for writing libese clients
and for implementing hardware backends.  It depends on a backend being
provided as per *libese-hw* and on *libese-sysdeps*.

*libese-sysdeps* provides the system level libraries that are needed by
libese provided software.  If libese is being ported to a new environment,
like a bootloader or non-Linux OS, this library may need to be replaced.
(Also take a look at libese/include/ese/log.h for the macro definitions
 that may be needed.)

*libese-hw* provides existing libese hardware backends.

*libese-teq1* provides a T=1 compatible transcieve function that may be
used by a hardware backend.  It comes with some prequisites for use,
such as a specifically structured set of error messages and
EseInteface pad usage, but otherwise it does not depends on any specific
functionality not abstracted via the libese EseOperations structure.


## Supported backends

There are two test backends, fake and echo, as well as one
real backend for the NXP PN80T/PN81A.

The NXP backends support both a direct kernel driver and
a Linux SPIdev interface.
