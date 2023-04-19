#include "port.h"


Port::Port(uint16_t portnumber)
{
    this->portnumber = portnumber;
}


Port::~Port()
{
}

Port8Bit::Port8Bit(uint16_t portnumber)
    : Port(portnumber)
{
}


Port8Bit::~Port8Bit()
{
}


void Port8Bit::Write(uint8_t data)
{
    Write8(portnumber, data);
}


uint8_t Port8Bit::Read()
{
    return Read8(portnumber);
}
