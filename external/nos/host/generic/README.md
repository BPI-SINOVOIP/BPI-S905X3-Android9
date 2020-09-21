# Generic host components for Nugget

Nugget will be used in different contexts and with different hosts. This repo
contains the components that can be shared between those hosts.

## `nugget`

The `nugget` directory contains items that are shared between the host and the
firmware. Those include:

   * shared headers
   * service protos

## `libnos`

`libnos` is a C++ library for communication with a Nugget device. It offers an
interface to manage a connection and exchange data and a generator for RPC stubs
based on service protos.

## `libnos_datagram`

`libnos_datagram` is a C library for exchanging datagrams with a Nugget device.
This directory only contains the API of the library as the different platforms
will need to implement it differently.

## `libnos_transport`

`libnos_transport` is a C library for communicating with a Nugget device via the
transport API. This is built on top of the `libnos_datagram` library for
exchanging datagrams.
