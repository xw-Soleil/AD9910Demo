#ifndef IIR_FILTER_DESIGN_H
#define IIR_FILTER_DESIGN_H

// The FilterType_t enum is now defined in app_config.h to avoid redefinition.
#include "app_config.h"

/**
 * @brief Structure to hold the coefficients for a single biquad section.
 * * Represents the difference equation:
 * y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
 */
typedef struct {
    double b0, b1, b2; // Numerator (Feedforward) coefficients
    double a1, a2;     // Denominator (Feedback) coefficients (a0 is implicitly 1)
} BiquadCoeffs;

/**
 * @brief Designs a second-order IIR digital filter (biquad) based on analog prototype parameters.
 * * This is the main, unified function for filter design.
 * * @param type   The type of filter to design (e.g., FILTER_TYPE_LPF).
 * @param k      The gain of the filter.
 * @param f0     The center or cutoff frequency in Hz.
 * @param Q      The quality factor.
 * @param fs     The sampling frequency in Hz.
 * @param coeffs A pointer to a BiquadCoeffs struct where the calculated coefficients will be stored.
 */
void design_biquad_filter(FilterType_t type, double k, double f0, double Q, double fs, BiquadCoeffs* coeffs);

#endif // IIR_FILTER_DESIGN_H
