# Citadel components for Android

## `libnos`

`libnos_datagram` implements the interface for connecting to a Citadel device
and transferring datagrams. This is wrapped by the C++ `libnos` library which
further supports the transport API.

## `citadeld`

Citadel will be running Nugget. In order to synchronize access to the driver,
HALs should proxy all communication via the `citadeld` daemon which will be the
only service with driver access.

Synchronizing with this service, rather than in the driver, allows for easier
debugging and fixing should the need arise.

`CitadeldProxyClient` will implement `NuggetClient` to handle proxying
communication via `citadeld` without requiring change to the HALs.
