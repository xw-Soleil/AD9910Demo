#ifndef SYSTEM_IDENTIFICATION_H
#define SYSTEM_IDENTIFICATION_H

#include "app_config.h"
#include <stdbool.h>
#include "expert_fitter_double.h" // Also include this in the header

/**
 * @brief Initializes the system identification module.
 */
void SysId_Init(void);

/**
 * @brief Runs one iteration of the system identification state machine.
 * @note This should be called repeatedly in the main loop.
 */
void SysId_RunStateMachine(void);

/**
 * @brief Checks if the identification process is complete.
 * @return true if done, false otherwise.
 */
bool SysId_IsDone(void);



FilterParams_t_double SysId_GetFittedParams(void);

FilterType_t SysId_GetPreliminaryFilterType(void);


FilterType_t SysId_GetFinalFilterType(void);

// --- Callbacks that need to be called from main.c/stm32f4xx_it.c ---
// These functions are the link between the hardware interrupts and this module.

/**
 * @brief ADC DMA Conversion Complete Callback to be called from HAL_ADC_ConvCpltCallback.
 */
void SysId_ADCCallback(void);


/**
 * @brief ADC DMA Half Conversion Complete Callback to be called from HAL_ADC_ConvHalfCpltCallback.
 */
void SysId_ADCErrorCallback(void);


void generate_sine_table(float peak_voltage);
void start_sine_output(float frequency);
void stop_sine_output(void);


#endif // SYSTEM_IDENTIFICATION_H
