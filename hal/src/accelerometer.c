#include "hal/accelerometer.h"
#include "hal/adc.h"
#include <stdint.h>

#define XL_X_CHANNEL 0
#define XL_Y_CHANNEL 1
#define XL_Z_CHANNEL 2

// ADC channel for each axis
static const uint8_t AXIS_CHANNELS[NUM_XL_AXES] = {
    [XL_AXIS_X] = XL_X_CHANNEL,
    [XL_AXIS_Y] = XL_Y_CHANNEL,
    [XL_AXIS_Z] = XL_Z_CHANNEL
};

int Accelerometer_readAxis(XL_Axis axis)
{
    int index = (int)axis;
    if(index < 0 || index >= NUM_XL_AXES) {
        return ADC_READ_ERROR;
    }
    return ADC_read_channel(AXIS_CHANNELS[index]);
}
