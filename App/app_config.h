#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <math.h>
#include "arm_math.h" // For PI define

//==============================================================================
// --- Common Type Definitions ---
//==============================================================================

// Filter type enumeration, used by all modules.
typedef enum {
    FILTER_TYPE_LPF,
    FILTER_TYPE_HPF,
    FILTER_TYPE_BPF,
    FILTER_TYPE_BSF,
    FILTER_TYPE_UNKNOWN
} FilterType_t;

// Structure for a single measurement point, used by all modules.
typedef struct {
    float frequency_hz;
    float gain;
} MeasurementPoint_t;

// REMOVED: The old float-based FilterParams_t is no longer the primary parameter
//          struct. The new double-precision struct is defined in the fitter's
//          header as part of its public API.


//==============================================================================
// --- System Identification (Sweep & Fit) Configuration ---
//==============================================================================

// MODIFIED: Increased to 801 points to match the new algorithm's design.
// The new, more complex algorithm performs best with a higher point density.
#define SWEEP_POINTS            801

// MODIFIED: Expanded sweep range to match the new algorithm's simulation range.
#define SWEEP_FREQ_START        100.0f
#define SWEEP_FREQ_END          500000.0f

// --- DAC/Signal Generation Defines (Unchanged) ---
#define SINE_TABLE_SIZE         128
#define DAC_VREF                3.3f
#define DAC_MAX_VAL             4095
#define DAC_AMP_DEFAULT         2.0f // DAC/DDS output peak-to-peak voltage

// --- ADC/Measurement Defines (Unchanged) ---
#define ADC_BUFFER_SIZE         2048
#define ADC_VREF                3.3f
#define ADC_MAX_VAL             4095
#define NUM_BUFFERS_FOR_AVG     4

// --- Dynamic Sampling Defines ---
#define TARGET_CYCLES_IN_BUFFER 10.0f
// MODIFIED: MIN_ADC_SAMPLING_FREQ now depends on the new SWEEP_FREQ_END
#define MIN_ADC_SAMPLING_FREQ   (SWEEP_FREQ_END * 2.5f)
#define MAX_ADC_SAMPLING_FREQ   2000000.0f

// --- Levenberg-Marquardt (LM) Algorithm Defines ---
// NEW: These are the double-precision parameters for the new expert fitter.
// The old float-based defines have been removed.
#define LM_MAX_ITERATIONS       100
#define LM_LAMBDA_INIT          1e-3      // double
#define LM_STOP_THRESHOLD       1e-10     // double
#define LM_SSE_STOP_THRESHOLD   1e-15     // double
#define INNER_LOOP_FAILSAFE_COUNT 30
#define LM_LAMBDA_FACTOR_UP     10.0      // double
#define LM_LAMBDA_FACTOR_DOWN   10.0      // double


//==============================================================================
// --- Real-time IIR Filter Configuration (Mostly Unchanged) ---
//==============================================================================
// MODIFIED: REALTIME_SAMPLING_RATE might need adjustment for the wider sweep range.
// Let's set it to handle up to 500kHz / 2.5 = 200kHz signals, e.g., 500kHz sampling.
// This depends on your final real-time application needs.
#define REALTIME_SAMPLING_RATE  500000.0f
#define REALTIME_BUFFER_SIZE    128
#define PING_PONG_SIZE          (REALTIME_BUFFER_SIZE * 2)
#define DC_OFFSET               2048
#define USE_FLOAT_IIR           1

//==============================================================================
// --- Hardware and Interface Defines (Unchanged) ---
//==============================================================================
#define USE_DAC_OUTPUT
#define adciir hadc3

#endif // APP_CONFIG_H