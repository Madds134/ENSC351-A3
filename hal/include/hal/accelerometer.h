/**
 * @file accelerometer.h
 * @author Madden Wynans
 * @brief HAL for the 3-axis accelerometer (X, Y, Z on ADC channels 0, 1, 2)
 * @version 0.1
 * @date 2026-11-20
 * @copyright Copyright (c) 2026
 * 
 */
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H

typedef enum {
    XL_AXIS_X,
    XL_AXIS_Y,
    XL_AXIS_Z,
    NUM_XL_AXES
} XL_Axis;

/**
 * @brief Reads one axis of the accelerometer
 * @param axis Axis to read
 * @warning ADC_init() must be called first
 * @return 12 bit reading from ADC
 */
int Accelerometer_readAxis(XL_Axis axis);
#endif