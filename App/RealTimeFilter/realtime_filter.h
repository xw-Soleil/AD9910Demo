#ifndef REALTIME_FILTER_H
#define REALTIME_FILTER_H

#include "app_config.h"
#include "iir_filter_design.h" // For BiquadCoeffs
#include <stdbool.h>

#include "system_identification.h"
#include "expert_fitter_double.h" // For ExpertFitResult_t_double
#include "accurate_fft.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <float.h>

#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"




/**
 * @brief Initializes the real-time IIR filter module.
 * @param coeffs Pointer to the biquad coefficients calculated from the fitted parameters.
 */
void RealtimeFilter_Init(const BiquadCoeffs* coeffs);

/**
 * @brief Starts the ADC/DAC pipeline for real-time filtering.
 */
void RealtimeFilter_Start(void);

/**
 * @brief Stops the real-time filtering pipeline.
 */
void RealtimeFilter_Stop(void);


// --- Callbacks that need to be called from main.c/stm32f4xx_it.c ---
// These functions are the link between the hardware interrupts and this module.

/**
 * @brief ADC DMA Half Conversion Complete Callback for real-time mode.
 */
void RealtimeFilter_ADCHalfCpltCallback(void);

/**
 * @brief ADC DMA Full Conversion Complete Callback for real-time mode.
 */
void RealtimeFilter_ADCFullCpltCallback(void);

/**
 * @brief ADC Error Callback for real-time mode.
 */
void RealtimeFilter_ADCErrorCallback(void);


void Correct_HSCorr_Init(void);

uint16_t apply_hs_corr(uint16_t sample);


#endif // REALTIME_FILTER_H
