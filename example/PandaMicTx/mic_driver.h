#pragma once

#include <Arduino.h>
#include <arduinoFFT.h>
#include "constants.h"

extern const unsigned char error_48[4608];

namespace PandaMicTx
{
    class STICKMIC
    {
    private:
        inline void _tone(unsigned int frequency, unsigned long duration = 0UL) { tone(BUZZ_PIN, frequency, duration); }
        inline void _noTone() { noTone(BUZZ_PIN); }

        /* Mic */
        void mic_init();
        void mic_test();
        void mic_test_one_task();

        void DisplayMicro();

        void new_mic_test();
        void new_mic_test_fft();

    public:
        STICKMIC(){}
        ~STICKMIC() {}
    };

}
