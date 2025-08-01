#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <math.h>
#include "arm_math.h" // For PI define and float32_t

//==============================================================================
// --- Common Type Definitions
//==============================================================================

// Filter type enumeration, unified from both original files
typedef enum {
    FILTER_TYPE_LPF,     // Low-Pass Filter
    FILTER_TYPE_HPF,     // High-Pass Filter
    FILTER_TYPE_BPF,     // Band-Pass Filter
    FILTER_TYPE_BSF,     // Band-Stop Filter (Notch)
    FILTER_TYPE_UNKNOWN
} FilterType_t;

// Structure to store a single measurement point (frequency and gain)
typedef struct {
    float frequency_hz;
    float gain;
} MeasurementPoint_t;

// Structure to hold the fitted parameters of the filter model
typedef struct {
    float k;  // Gain
    float f0; // Center/cutoff frequency in Hz
    float q;  // Quality factor
} FilterParams_t;


//==============================================================================
// --- System Identification (Sweep & Fit) Configuration
//==============================================================================

#define SWEEP_POINTS            50
#define SWEEP_FREQ_START        1000.0f
#define SWEEP_FREQ_END          50000.0f

// --- DAC/Signal Generation Defines ---
#define SINE_TABLE_SIZE         128
#define DAC_VREF                3.3f
#define DAC_MAX_VAL             4095
#define DAC_AMP_DEFAULT         2.0f // DAC/DDS output peak-to-peak voltage

// --- ADC/Measurement Defines ---
#define ADC_BUFFER_SIZE         2048
#define ADC_VREF                3.3f
#define ADC_MAX_VAL             4095
#define NUM_BUFFERS_FOR_AVG     4      // Number of ADC buffers to average for one measurement point

// --- Dynamic Sampling Defines ---
#define TARGET_CYCLES_IN_BUFFER 10.0f  // Target this many sine cycles in the ADC buffer for good resolution
#define MIN_ADC_SAMPLING_FREQ   (SWEEP_FREQ_END * 2.5f) // Min Fs based on Nyquist for max sweep freq
#define MAX_ADC_SAMPLING_FREQ   2000000.0f // Max Fs for the ADC (e.g., 2 MSPS)

// --- Levenberg-Marquardt (LM) Algorithm Defines ---
#define LM_NUM_PARAMS           3
#define LM_MAX_ITERATIONS       100
#define LM_LAMBDA_INIT          1e-3f
#define LM_LAMBDA_FACTOR_UP     10.0f
#define LM_LAMBDA_FACTOR_DOWN   10.0f
#define LM_STOP_THRESHOLD       1e-7f
#define LM_H_FOR_JACOBIAN       1e-4f // Step for numerical differentiation

//==============================================================================
// --- Real-time IIR Filter Configuration
//==============================================================================
// UPDATED: Set to 250kHz to handle signals up to 50kHz.
#define REALTIME_SAMPLING_RATE  250000.0f 
#define REALTIME_BUFFER_SIZE    128      // Processing block size for real-time filter
#define PING_PONG_SIZE          (REALTIME_BUFFER_SIZE * 2) // Full DMA buffer size
#define DC_OFFSET               2048     // Midpoint for 12-bit ADC/DAC (4096 / 2)
#define USE_FLOAT_IIR           1        // 1 for float implementation (recommended), 0 for fixed-point

#define USE_DAC_OUTPUT

/**
 * @brief 硬件接口
 */
#define adciir hadc3

#endif // APP_CONFIG_H
