#ifndef EXPERT_FITTER_DOUBLE_H
#define EXPERT_FITTER_DOUBLE_H

// NEW: Include the centralized application configuration file.
#include "app_config.h"
#include <float.h> // For DBL_MAX

//==============================================================================
// --- WARNING: Performance and Memory Notice (Unchanged) ---
//==============================================================================
// This implementation uses 'double' precision for the fitting algorithm to
// maintain maximum numerical accuracy, matching the PC simulation.
// This will result in:
// 1. SIGNIFICANTLY SLOWER execution on most STM32 MCUs.
// 2. HIGHER RAM consumption (check SWEEP_POINTS in app_config.h).
// Ensure your target MCU has sufficient resources.
//==============================================================================


//==============================================================================
// --- 1. Public API Data Structures ---
//==============================================================================
// REMOVED: FilterType_t and MeasurementPoint_t are now defined in app_config.h

// This struct remains here as it defines the fitter's core parameter set.
typedef struct {
    double k;  // Gain
    double w0; // Center/cutoff angular frequency in rad/s (2 * PI * f0)
    double q;  // Quality factor
} FilterParams_t_double;

// This struct remains here as it defines the fitter's complete output.
typedef struct {
    FilterParams_t_double params;
    double sse;
    FilterType_t preliminary_type;
    FilterType_t final_type;
} ExpertFitResult_t_double;


//==============================================================================
// --- 2. Configuration (REMOVED) ---
//==============================================================================
// REMOVED: All configuration macros (SWEEP_POINTS, LM_...) are now defined
// in app_config.h and included above.


//==============================================================================
// --- 3. Public Function Prototypes (Unchanged) ---
//==============================================================================

/**
 * @brief Runs the complete expert filter fitting process using DOUBLE precision.
 *
 * @param measured_data Pointer to the array of measurement points.
 * @param num_points    The number of points in the array (should be SWEEP_POINTS from app_config.h).
 * @param result        Pointer to the result structure to be filled.
 */
void Expert_Fitter_Run_Double(const MeasurementPoint_t* measured_data, int num_points, ExpertFitResult_t_double* result);

/**
 * @brief Helper to get a string representation of a filter type.
 * @param type The filter type enum.
 * @return A constant string describing the type.
 */
const char* Expert_Fitter_GetTypeString(FilterType_t type);


#endif // EXPERT_FITTER_DOUBLE_H