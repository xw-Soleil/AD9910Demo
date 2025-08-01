#ifndef SYSTEM_IDENTIFICATION_H
#define SYSTEM_IDENTIFICATION_H

#include "app_config.h"
#include <stdbool.h>

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

/**
 * @brief Gets the final fitted parameters after the process is done.
 * @return A struct containing the fitted k, f0, and q values.
 */
FilterParams_t SysId_GetFittedParams(void);

/**
 * @brief Gets the identified filter type.
 * @return The identified filter type enum.
 */
FilterType_t SysId_GetFilterType(void);

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


#endif // SYSTEM_IDENTIFICATION_H
