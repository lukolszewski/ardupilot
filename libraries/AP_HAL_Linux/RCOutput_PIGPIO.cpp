#include "RCOutput_PIGPIO.h"

#include <cmath>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

#include <pigpio.h>

#include <AP_HAL/AP_HAL.h>
#include <GCS_MAVLink/GCS.h>

#include "GPIO.h"


using namespace Linux;


RCOutput_PIGPIO::RCOutput_PIGPIO(uint8_t channels)
{
    channels = sizeof(PIGPIO_CHANNELS_TO_GPIO_MAPPING);
    for(uint8_t i=0;i<channels;i++){
        channels_enabled[i] = false;
    }
    _pulses_buffer = new uint16_t[channels];
}

RCOutput_PIGPIO::~RCOutput_PIGPIO()
{
    delete [] _pulses_buffer;
}

void RCOutput_PIGPIO::init()
{
    if (gpioInitialise() < 0)
    {
        GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "PIGPIO SW PWM init failed!");
        return;
    }
    else
    {
        init_worked=true;
    }
}

void RCOutput_PIGPIO::reset_all_channels()
{
    for(uint8_t i=0;i<channels;i++){
        if(channels_enabled[i]) gpioServo(PIGPIO_CHANNELS_TO_GPIO_MAPPING[i],0);
    }
}

void RCOutput_PIGPIO::set_freq(uint32_t chmask, uint16_t freq_hz)
{
    if(freq_hz==_frequency) return;
        else {
            GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Unsupported PWM frequency requested:%u Hz", freq_hz);    
        }
}

uint16_t RCOutput_PIGPIO::get_freq(uint8_t ch)
{
    return _frequency;
}

void RCOutput_PIGPIO::enable_ch(uint8_t ch)
{
    if(ch>channels) {
        GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Not configured PWM channel requested no %u. Ignoring.",ch);
        return;
    }
    channels_enabled[ch] = true;
    gpioSetMode(PIGPIO_CHANNELS_TO_GPIO_MAPPING[ch], PI_OUTPUT);
    //uint8_t = gpioServo(PIGPIO_CHANNELS_TO_GPIO_MAPPING[ch],1500);
}

void RCOutput_PIGPIO::disable_ch(uint8_t ch)
{
    write(ch, 0);
    channels_enabled[ch] = false;
}

void RCOutput_PIGPIO::write(uint8_t ch, uint16_t period_us)
{
   if(!channels_enabled[ch]) {
        GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Write to disabled channel %u requested. Ignoring.",ch);
        return;
   }

   _pulses_buffer[ch] = period_us;
   _pending_write_mask |= (1U << ch);

    if (!_corking) {
        _corking = true;
        push();
    }
}

void RCOutput_PIGPIO::cork()
{
    _corking = true;
}

void RCOutput_PIGPIO::push()
{
    if (!_corking) {
        return;
    }
    _corking = false;

    if (_pending_write_mask == 0)
        return;

    // Calculate the number of channels for this update.
    uint8_t max_ch = (sizeof(unsigned) * 8) - __builtin_clz(_pending_write_mask);
    uint8_t min_ch = __builtin_ctz(_pending_write_mask);

    for (unsigned ch = min_ch; ch < max_ch; ch++) {
        uint16_t period_us = _pulses_buffer[ch];
        gpioServo(PIGPIO_CHANNELS_TO_GPIO_MAPPING[ch],period_us);
    }
    _pending_write_mask = 0;
}

uint16_t RCOutput_PIGPIO::read(uint8_t ch)
{
    return _pulses_buffer[ch];
}

void RCOutput_PIGPIO::read(uint16_t* period_us, uint8_t len)
{
    for (int i = 0; i < len; i++) {
        period_us[i] = read(0 + i);
    }
}
