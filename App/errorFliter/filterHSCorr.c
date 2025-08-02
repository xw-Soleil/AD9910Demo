#ifndef FILTER_HSCORR_C
#define FILTER_HSCORR_C

#include "app_config.h"
#include "expert_fitter_double.h"
#include "iir_filter_design.h"



FilterParams_t_double coeffs;
BiquadCoeffs iir_coeffs_corr;
void filter_hscorr_init(double k, double w0, double q, FilterType_t type) {
    // Initialize the filter coefficients to zero
    coeffs.k = k;
    coeffs.w0 = w0;
    coeffs.q = q;
    design_biquad_filter(
        type, coeffs.k, coeffs.w0 / (2 * PI), coeffs.q, REALTIME_SAMPLING_RATE, &iir_coeffs_corr);
    
}


#endif