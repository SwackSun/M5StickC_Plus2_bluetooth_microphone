#ifndef _MIC_DRIVER_H
#define _MIC_DRIVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <Arduino.h>
#include <M5GFX.h>
#include "constants.h"

class MicDriver
{
private:

public:
    MicDriver() {}
    ~MicDriver() {}

    void mic_init();
    void mic_get_raw(uint8_t *p_buf, uint32_t sz);
    void mic_get_fft(uint8_t *p_buf, uint32_t sz);
};

#endif //_MIC_DRIVER_H
