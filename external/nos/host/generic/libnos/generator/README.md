# Service generator

The service generator auto-generates interface classes to invoke the service's
method running on a Nugget device. Three components can be generated:

   1. Interface class declarations (header)
   2. Interface class definitions (source)
   3. Interface class mock declaration (mock)

## Generated classes

All classes are generated in a namespace matching that of the protobuf package.
Each service generated its own header files, for example `service Example` is
included with:

    #include <Example.client.h>
    #include <MockExample.client.h>

### Service interface

A pure virtual class is generated which declares the methods offered by the
service. This class is named after the service with and 'I' prepended, for
example `service Example` generates `class IExample`.

The methods return the `uint32_t` app status code and takes references to the
input and ouput messages as arguments. The app's response will be decoded into
the output message if the app does not return an error.

This interface class is the type that should be used the most as it allows mocks
to be injected for testing.

### `libnos` implementation

An impementation of the service interface which wraps a `NuggetClient` reference
is also generated. This is the concrete implementation for invoking a method in
a service running on Nugget and exchanging messages with it. The name of this
class is the same as that of the service, for example `service Example` generates
`class Example`.

### Mocks

The generator can further produce mocks of the service interface to simplify
testing of code that uses the service. This class's name is the name of the
service prepended with 'Mock', for example `service Example` generates
`class MockExample`.
