#include "iir_filter_design.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Internal function for the core bilinear transformation.
static void bilinear_transform_general(const double num_s[3], const double den_s[3], double fs, BiquadCoeffs* coeffs) {
    double den2 = den_s[0];
    double den1 = den_s[1];
    double den0 = den_s[2];
    
    double num2 = num_s[0];
    double num1 = num_s[1];
    double num0 = num_s[2];

    double K = 2.0 * fs;
    double K_sq = K * K;

    double a0_raw = den2 * K_sq + den1 * K + den0;
    
    if (fabs(a0_raw) < 1e-9) { // Avoid division by zero
        coeffs->b0 = 1.0; coeffs->b1 = 0.0; coeffs->b2 = 0.0;
        coeffs->a1 = 0.0; coeffs->a2 = 0.0;
        return;
    }

    coeffs->b0 = (num2 * K_sq + num1 * K + num0) / a0_raw;
    coeffs->b1 = (2.0 * num0 - 2.0 * num2 * K_sq) / a0_raw;
    coeffs->b2 = (num2 * K_sq - num1 * K + num0) / a0_raw;

    coeffs->a1 = (2.0 * den0 - 2.0 * den2 * K_sq) / a0_raw;
    coeffs->a2 = (den2 * K_sq - den1 * K + den0) / a0_raw;
}

// Implementation of the unified design function
void design_biquad_filter(FilterType_t type, double k, double f0, double Q, double fs, BiquadCoeffs* coeffs) {
    double w0 = 2.0 * M_PI * f0;
    double w0_sq = w0 * w0;

    // Analog prototype coefficients (s^2, s^1, s^0)
    double num_s[3] = {0.0, 0.0, 0.0};
    double den_s[3] = {1.0, w0 / Q, w0_sq};

    // UPDATED: Using the consistent enum values from app_config.h
    switch (type) {
        case FILTER_TYPE_LPF:
            // H(s) = k * w0^2 / (s^2 + (w0/Q)*s + w0^2)
            num_s[2] = k * w0_sq;
            break;

        case FILTER_TYPE_HPF:
            // H(s) = k * s^2 / (s^2 + (w0/Q)*s + w0^2)
            num_s[0] = k;
            break;

        case FILTER_TYPE_BPF:
            // H(s) = k * (w0/Q)*s / (s^2 + (w0/Q)*s + w0^2)
            num_s[1] = k * w0 / Q;
            break;

        case FILTER_TYPE_BSF:
            // H(s) = k * (s^2 + w0^2) / (s^2 + (w0/Q)*s + w0^2)
            num_s[0] = k;
            num_s[2] = k * w0_sq;
            break;
        
        default: // Includes FILTER_TYPE_UNKNOWN
            // Set to a safe, pass-through filter
            coeffs->b0 = 1.0; coeffs->b1 = 0.0; coeffs->b2 = 0.0;
            coeffs->a1 = 0.0; coeffs->a2 = 0.0;
            return; // Exit immediately
    }

    // Perform the transformation based on the selected prototype
    bilinear_transform_general(num_s, den_s, fs, coeffs);
}
