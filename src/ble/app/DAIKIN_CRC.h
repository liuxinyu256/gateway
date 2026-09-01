#ifndef __DAIKIN_CRC_H
#define __DAIKIN_CRC_H
#include "stdint.h"
#include "CH57xBLE_LIB.H"

uint8_t DAIKIN_CRC_Calc(uint8_t *bufData, uint16_t buflen);

#endif
