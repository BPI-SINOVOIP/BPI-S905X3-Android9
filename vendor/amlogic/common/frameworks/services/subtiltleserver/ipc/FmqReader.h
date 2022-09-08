#pragma once

class FmqReader {
public:
    virtual ~FmqReader() {};
    virtual size_t availableSize() = 0;
    virtual size_t read(uint8_t *buf, size_t size) = 0;
    virtual bool pollEvent() = 0;
};
