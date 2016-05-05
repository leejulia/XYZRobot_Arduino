//
// Created by Anton Matosov on 5/5/16.
//

#include <cstddef>
#include <stdint-gcc.h>
#include <USBAPI.h>
#include "ArduinoProgramMemory.h"

uint8_t ArduinoProgramMemory::readByteNear(void* addr)
{
    return pgm_read_byte_near(addr);
}

uint16_t ArduinoProgramMemory::readWordNear(void* addr)
{
    return pgm_read_word_near(addr);
}

uint32_t ArduinoProgramMemory::readDwordNear(void* addr)
{
    return pgm_read_dword_near(addr);
}

float ArduinoProgramMemory::readFloatNear(void* addr)
{
    return pgm_read_float_near(addr);
}

void* ArduinoProgramMemory::readPtrNear(void* addr)
{
    return pgm_read_ptr_near(addr);
}


