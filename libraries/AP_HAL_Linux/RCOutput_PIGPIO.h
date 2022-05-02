#pragma once

#include "AP_HAL_Linux.h"

#define PIGPIO_CHANNELS_TO_GPIO_MAPPING (int[]){27,22,0,18,23,24,25,1}
#define PIGPIO_MAX_CHANNELS 8

namespace Linux {

class RCOutput_PIGPIO : public AP_HAL::RCOutput {
    public:
        RCOutput_PIGPIO(uint8_t ch);
        
        ~RCOutput_PIGPIO();
        void init() override;
        void reset_all_channels();
        void     set_freq(uint32_t chmask, uint16_t freq_hz) override;
        uint16_t get_freq(uint8_t ch) override;
        void     enable_ch(uint8_t ch) override;
        void     disable_ch(uint8_t ch) override;
        void     write(uint8_t ch, uint16_t period_us) override;
        void     cork() override;
        void     push() override;
        uint16_t read(uint8_t ch) override;
        void     read(uint16_t* period_us, uint8_t len) override;

    private:
        void reset();

        uint16_t _frequency = 50;
        uint16_t *_pulses_buffer;
        bool _corking = false;
        uint8_t _pending_write_mask;
        uint8_t channels = 0;
        bool channels_enabled[sizeof(PIGPIO_CHANNELS_TO_GPIO_MAPPING)];

        bool init_worked = false;

};

}
