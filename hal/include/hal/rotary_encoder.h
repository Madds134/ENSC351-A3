/**
 * @file rotary_encoder.h
 * @brief HAL for the rotary encoder (PEC12R): channel A on GPIO2, B on GPIO3
 * @details Watches both encoder pins for edges and decodes the quadrature
 *          signal. One full detent (click) of the knob is reported as one step.
 */
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#define ROTARY_STEP_CW   (+1)
#define ROTARY_STEP_CCW  (-1)

/**
 * @brief Requests the encoder's GPIO lines and starts watching for edges
 * @return 0 on success, -1 on failure
 */
int Rotary_init(void);

/**
 * @brief Releases the encoder's GPIO lines
 */
void Rotary_cleanup(void);

/**
 * @brief Blocks until the knob completes one detent, or the timeout expires
 * @param timeoutMs How long to wait for an edge (negative waits forever)
 * @param step Set to ROTARY_STEP_CW or ROTARY_STEP_CCW when a step is detected
 * @return 1 if a step was detected (*step is valid), 0 on timeout, -1 on error
 * @warning Rotary_init() must be called first
 */
int Rotary_waitStep(int timeoutMs, int *step);

/**
 * @brief Reads the raw levels of channels A and B (for debugging/display)
 * @param a Set to 1 if channel A is high, 0 if low
 * @param b Set to 1 if channel B is high, 0 if low
 * @return 0 on success, -1 on error
 * @note Safe to call from another thread while Rotary_waitStep() is blocked
 */
int Rotary_readLevels(int *a, int *b);

#endif
