#!/bin/bash

## Compiles proto files to .py files
## Note that proto version 3.0.0 is needed for successful compilation.
protoc -I=internal/proto --python_out=internal/proto internal/proto/internal_config.proto
protoc -I=internal/proto --python_out=internal/proto internal/proto/user_config.proto
