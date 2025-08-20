#include "expert_fitter_double.h"
#include <string.h> // For memset

//==============================================================================
// --- Internal Data Structures (Double Precision) ---
//==============================================================================
typedef struct {
    FilterParams_t_double params;
    double sse;
    FilterType_t type;
} PreliminaryFitResult_t_double;


//==============================================================================
// --- Private Function Prototypes (Double Precision) ---
//==============================================================================
static double model_function_double(const FilterParams_t_double* params, double w, FilterType_t type);
static void calculate_params_from_features_double(const MeasurementPoint_t* data, int num_points, FilterType_t type, FilterParams_t_double* params);
static void find_best_filter_model_iteratively_double(const MeasurementPoint_t* data, int num_points, PreliminaryFitResult_t_double* best_result);
static void run_one_competitive_fit_attempt_double(const MeasurementPoint_t* data, int num_points, PreliminaryFitResult_t_double* best_result);
static double lm_fit_double(const MeasurementPoint_t* data, int num_points, FilterType_t type, FilterParams_t_double* params);
static double lm_fit_w0_q_double(const MeasurementPoint_t* data, int num_points, FilterType_t type, FilterParams_t_double* params);
static void lm_calc_jacobian_3_params_double(const FilterParams_t_double* params, double w, FilterType_t type, double jacobian_row[3]);
static void lm_calc_jacobian_2_params_double(const FilterParams_t_double* params, double w, FilterType_t type, double jacobian_row[2]);
static int solve_linear_system_3x3_double(const double A[3][3], const double b[3], double x[3]);
static int solve_linear_system_2x2_double(const double A[2][2], const double b[2], double x[2]);
static double log_interp_double(double f1, double g1, double f2, double g2, double target_g);

// --- Feature analysis can remain float-based as it's less sensitive ---
static void generate_frequency_response_for_analysis(const FilterParams_t_double* params, FilterType_t type, MeasurementPoint_t* output_data, int num_points);
static FilterType_t identify_filter_type_from_response(const MeasurementPoint_t* data, int num_points);


//==============================================================================
// --- Public API Implementation ---
//==============================================================================

void Expert_Fitter_Run_Double(const MeasurementPoint_t* measured_data, int num_points, ExpertFitResult_t_double* result) {
    if (!measured_data || !result || num_points <= 0) return;

    // Step 1: Run competitive fitting (double precision) for a preliminary result
    PreliminaryFitResult_t_double preliminary_result;
    find_best_filter_model_iteratively_double(measured_data, num_points, &preliminary_result);

    // Step 2: Populate the output struct
    result->params = preliminary_result.params;
    result->sse = preliminary_result.sse;
    result->preliminary_type = preliminary_result.type;

    // Step 3: Generate a smooth curve for feature analysis
    static MeasurementPoint_t ideal_fit_curve[SWEEP_POINTS];
    generate_frequency_response_for_analysis(&preliminary_result.params, preliminary_result.type, ideal_fit_curve, num_points);

    // Step 4: Run final, robust type identification on the smooth curve
    result->final_type = identify_filter_type_from_response(ideal_fit_curve, num_points);
}

const char* Expert_Fitter_GetTypeString(FilterType_t type) {
    switch(type) {
        case FILTER_TYPE_LPF: return "Low-Pass";
        case FILTER_TYPE_HPF: return "High-Pass";
        case FILTER_TYPE_BPF: return "Band-Pass";
        case FILTER_TYPE_BSF: return "Band-Stop";
        default: return "Unknown";
    }
}

//==============================================================================
// --- Core Algorithm (Double Precision) ---
//==============================================================================

static double model_function_double(const FilterParams_t_double* params, double w, FilterType_t type) {
    if (params->q <= 0.0 || params->w0 <= 0.0) return 0.0;
    double w_sq = w * w;
    double w0_sq = params->w0 * params->w0;
    double term1_denom = w0_sq - w_sq;
    double term2_denom = params->w0 * w / params->q;
    double denominator = sqrt(term1_denom * term1_denom + term2_denom * term2_denom);
    if (denominator < 1e-12) return 0.0;
    double numerator = 0.0;
    switch(type) {
        case FILTER_TYPE_LPF: numerator = w0_sq; break;
        case FILTER_TYPE_HPF: numerator = w_sq; break;
        case FILTER_TYPE_BPF: numerator = term2_denom; break;
        case FILTER_TYPE_BSF: numerator = fabs(term1_denom); break;
        default: return 0.0;
    }
    return params->k * (numerator / denominator);
}

static void find_best_filter_model_iteratively_double(const MeasurementPoint_t* data, int num_points, PreliminaryFitResult_t_double* best_result_so_far) {
    const int MAX_ATTEMPTS = 1;
    const double GOOD_FIT_SSE_THRESHOLD = 0.15;
    best_result_so_far->sse = DBL_MAX;
    best_result_so_far->type = FILTER_TYPE_UNKNOWN;

    for (int i = 0; i < MAX_ATTEMPTS; i++) {
        PreliminaryFitResult_t_double current_result;
        run_one_competitive_fit_attempt_double(data, num_points, &current_result);
        if (current_result.sse < best_result_so_far->sse) {
            *best_result_so_far = current_result;
        }
        if (best_result_so_far->sse < GOOD_FIT_SSE_THRESHOLD) {
            break;
        }
    }
}

static void run_one_competitive_fit_attempt_double(const MeasurementPoint_t* data, int num_points, PreliminaryFitResult_t_double* best_result) {
    PreliminaryFitResult_t_double results[4];
    FilterType_t types_to_test[] = {FILTER_TYPE_LPF, FILTER_TYPE_HPF, FILTER_TYPE_BPF, FILTER_TYPE_BSF};

    for (int i = 0; i < 4; i++) {
        FilterType_t current_type = types_to_test[i];
        results[i].type = current_type;
        FilterParams_t_double initial_guess = {0};
        calculate_params_from_features_double(data, num_points, current_type, &initial_guess);
        if (current_type == FILTER_TYPE_LPF || current_type == FILTER_TYPE_BSF) {
            results[i].sse = lm_fit_w0_q_double(data, num_points, current_type, &initial_guess);
        } else {
            results[i].sse = lm_fit_double(data, num_points, current_type, &initial_guess);
        }
        results[i].params = initial_guess;
    }

    int best_fit_idx = 0;
    for (int i = 1; i < 4; i++) {
        if (results[i].sse < results[best_fit_idx].sse) {
            best_fit_idx = i;
        }
    }
    *best_result = results[best_fit_idx];
}


static void calculate_params_from_features_double(const MeasurementPoint_t* data, int num_points, FilterType_t type, FilterParams_t_double* params) {
    const int AVG_POINTS = 10;
    double gain_low = 0.0, gain_high = 0.0;
    for(int i = 0; i < AVG_POINTS; ++i) {
        gain_low += data[i].gain;
        gain_high += data[num_points - 1 - i].gain;
    }
    gain_low /= AVG_POINTS;
    gain_high /= AVG_POINTS;

    double gain_max = 0.0, freq_at_max = 0.0;
    int max_freq_idx = 0;
    for (int i = 0; i < num_points; i++) {
        if (data[i].gain > gain_max) { gain_max = data[i].gain; freq_at_max = data[i].frequency_hz; max_freq_idx = i; }
    }

    switch(type) {
        case FILTER_TYPE_LPF:
            params->k = 1.0; params->q = 0.707;
            params->w0 = 2.0 * M_PI * sqrt(data[0].frequency_hz * data[num_points - 1].frequency_hz);
            break;
        case FILTER_TYPE_HPF:
            params->k = gain_high; params->q = 0.707;
            params->w0 = 2.0 * M_PI * sqrt(data[0].frequency_hz * data[num_points - 1].frequency_hz);
            break;
        case FILTER_TYPE_BPF: {
            params->k = gain_max;
            params->w0 = 2.0 * M_PI * freq_at_max;
            double f_low = 0, f_high = 0;
            double half_power_gain = gain_max / sqrt(2.0);
            for (int i = max_freq_idx; i > 0; --i) if (data[i].gain >= half_power_gain && data[i-1].gain < half_power_gain) { f_low = log_interp_double(data[i-1].frequency_hz, data[i-1].gain, data[i].frequency_hz, data[i].gain, half_power_gain); break; }
            for (int i = max_freq_idx; i < num_points - 1; ++i) if (data[i].gain >= half_power_gain && data[i+1].gain < half_power_gain) { f_high = log_interp_double(data[i].frequency_hz, data[i].gain, data[i+1].frequency_hz, data[i+1].gain, half_power_gain); break; }
            if (f_high > f_low && f_low > 0) params->q = freq_at_max / (f_high - f_low); else params->q = 5.0;
            break;
        }
        case FILTER_TYPE_BSF: {
            params->k = 1.0;
            double gain_min = DBL_MAX, freq_at_min = 0.0;
            int min_freq_idx = 0;
            for(int i=0; i<num_points; ++i) if(data[i].gain < gain_min) { gain_min = data[i].gain; freq_at_min = data[i].frequency_hz; min_freq_idx = i; }
            params->w0 = 2.0 * M_PI * freq_at_min;
            double f_low = 0, f_high = 0;
            double half_power_gain = (params->k + gain_min) / 2.0;
            for (int i = min_freq_idx; i > 0; --i) if (data[i].gain <= half_power_gain && data[i-1].gain > half_power_gain) { f_low = log_interp_double(data[i-1].frequency_hz, data[i-1].gain, data[i].frequency_hz, data[i].gain, half_power_gain); break; }
            for (int i = min_freq_idx; i < num_points - 1; ++i) if (data[i].gain <= half_power_gain && data[i+1].gain > half_power_gain) { f_high = log_interp_double(data[i].frequency_hz, data[i].gain, data[i+1].frequency_hz, data[i+1].gain, half_power_gain); break; }
            if (f_high > f_low && f_low > 0) params->q = freq_at_min / (f_high - f_low); else params->q = 5.0;
            break;
        }
        default: break;
    }
    if (params->w0 <= 0) params->w0 = 2 * M_PI * 10000.0;
    if (params->q <= 0.01) params->q = 0.01;
    if (params->q > 50.0) params->q = 50.0;
    if (params->k <= 0) params->k = 1.0;
}


//==============================================================================
// --- Levenberg-Marquardt (LM) Algorithm Section (Double Precision) ---
//==============================================================================

static double lm_fit_double(const MeasurementPoint_t* measured_data, int num_points, FilterType_t type, FilterParams_t_double* fit_params) {
    static double J[SWEEP_POINTS][3]; static double r[SWEEP_POINTS];
    static double JtJ[3][3], JtJ_augmented[3][3]; static double Jtr[3], delta_p[3];
    FilterParams_t_double current_params = *fit_params;
    double lambda = LM_LAMBDA_INIT; double current_sse = 0.0;
    for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; double err = (double)measured_data[i].gain - model_function_double(&current_params, w, type); current_sse += err * err; }
    for (int iter = 0; iter < LM_MAX_ITERATIONS; iter++) {
        double prev_sse = current_sse;
        for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; lm_calc_jacobian_3_params_double(&current_params, w, type, J[i]); r[i] = (double)measured_data[i].gain - model_function_double(&current_params, w, type); }
        memset(JtJ, 0, sizeof(JtJ)); memset(Jtr, 0, sizeof(Jtr));
        for (int i = 0; i < 3; i++) { for (int j = 0; j < 3; j++) for (int k = 0; k < num_points; k++) JtJ[i][j] += J[k][i] * J[k][j]; for (int k = 0; k < num_points; k++) Jtr[i] += J[k][i] * r[k]; }
        
        int solved = 0; int inner_loop_count = 0;
        while (!solved && inner_loop_count++ < INNER_LOOP_FAILSAFE_COUNT) {
            for (int i = 0; i < 3; i++) { for (int j = 0; j < 3; j++) JtJ_augmented[i][j] = JtJ[i][j]; JtJ_augmented[i][i] += lambda * (JtJ[i][i] + 1e-9); }
            if (solve_linear_system_3x3_double(JtJ_augmented, Jtr, delta_p) == 0) {
                FilterParams_t_double new_params = { current_params.k + delta_p[0], current_params.w0 + delta_p[1], current_params.q + delta_p[2] };
                if (new_params.k < 0.1) new_params.k = 0.1; if (new_params.k > 2.5) new_params.k = 2.5;
                if (new_params.w0 < 0.0) new_params.w0 = 1e-6;
                if (new_params.q < 0.001) new_params.q = 0.001; if (new_params.q > 100.0) new_params.q = 100.0;
                
                double new_sse = 0; for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; double err = (double)measured_data[i].gain - model_function_double(&new_params, w, type); new_sse += err * err; }
                if (new_sse < current_sse) { current_params = new_params; current_sse = new_sse; lambda /= LM_LAMBDA_FACTOR_DOWN; solved = 1; } else { lambda *= LM_LAMBDA_FACTOR_UP; }
            } else { lambda *= LM_LAMBDA_FACTOR_UP; }
            if (lambda > 1e30) goto lm_exit_3p;
        }
        double delta_norm_sq = delta_p[0]*delta_p[0] + delta_p[1]*delta_p[1] + delta_p[2]*delta_p[2];
        if (sqrt(delta_norm_sq) < LM_STOP_THRESHOLD) goto lm_exit_3p;
        if (iter > 0 && fabs(current_sse - prev_sse) / (prev_sse + 1e-20) < LM_SSE_STOP_THRESHOLD) goto lm_exit_3p;
    }
lm_exit_3p: *fit_params = current_params; return current_sse;
}

static double lm_fit_w0_q_double(const MeasurementPoint_t* measured_data, int num_points, FilterType_t type, FilterParams_t_double* fit_params) {
    static double J[SWEEP_POINTS][2]; static double r[SWEEP_POINTS];
    static double JtJ[2][2], JtJ_augmented[2][2]; static double Jtr[2], delta_p[2];
    FilterParams_t_double current_params = *fit_params;
    double lambda = LM_LAMBDA_INIT; double current_sse = 0.0;
    for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; double err = (double)measured_data[i].gain - model_function_double(&current_params, w, type); current_sse += err * err; }
    for (int iter = 0; iter < LM_MAX_ITERATIONS; iter++) {
        double prev_sse = current_sse;
        for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; lm_calc_jacobian_2_params_double(&current_params, w, type, J[i]); r[i] = (double)measured_data[i].gain - model_function_double(&current_params, w, type); }
        memset(JtJ, 0, sizeof(JtJ)); memset(Jtr, 0, sizeof(Jtr));
        for (int i = 0; i < 2; i++) { for (int j = 0; j < 2; j++) for (int k = 0; k < num_points; k++) JtJ[i][j] += J[k][i] * J[k][j]; for (int k = 0; k < num_points; k++) Jtr[i] += J[k][i] * r[k]; }
        
        int solved = 0; int inner_loop_count = 0;
        while (!solved && inner_loop_count++ < INNER_LOOP_FAILSAFE_COUNT) {
            for (int i = 0; i < 2; i++) { for (int j = 0; j < 2; j++) JtJ_augmented[i][j] = JtJ[i][j]; JtJ_augmented[i][i] += lambda * (JtJ[i][i] + 1e-9); }
            if (solve_linear_system_2x2_double(JtJ_augmented, Jtr, delta_p) == 0) {
                FilterParams_t_double new_params = { current_params.k, current_params.w0 + delta_p[0], current_params.q + delta_p[1] };
                if (new_params.w0 < 0.0) new_params.w0 = 1e-6;
                if (new_params.q < 0.001) new_params.q = 0.001; if (new_params.q > 100.0) new_params.q = 100.0;

                double new_sse = 0; for (int i = 0; i < num_points; i++) { double w = 2.0 * M_PI * measured_data[i].frequency_hz; double err = (double)measured_data[i].gain - model_function_double(&new_params, w, type); new_sse += err * err; }
                if (new_sse < current_sse) { current_params = new_params; current_sse = new_sse; lambda /= LM_LAMBDA_FACTOR_DOWN; solved = 1; } else { lambda *= LM_LAMBDA_FACTOR_UP; }
            } else { lambda *= LM_LAMBDA_FACTOR_UP; }
            if (lambda > 1e30) goto lm_exit_2p;
        }
        double delta_norm_sq = delta_p[0]*delta_p[0] + delta_p[1]*delta_p[1];
        if (sqrt(delta_norm_sq) < LM_STOP_THRESHOLD) goto lm_exit_2p;
        if (iter > 0 && fabs(current_sse - prev_sse) / (prev_sse + 1e-20) < LM_SSE_STOP_THRESHOLD) goto lm_exit_2p;
    }
lm_exit_2p: *fit_params = current_params; return current_sse;
}

//==============================================================================
// --- Math and Utility Helpers ---
//==============================================================================

static void lm_calc_jacobian_3_params_double(const FilterParams_t_double* params, double w, FilterType_t type, double jacobian_row[3]) {
    FilterParams_t_double p_plus_h, p_minus_h;
    const double REL_STEP = 1e-7; const double ABS_EPSILON = 1e-9;
    double h_k = fabs(params->k * REL_STEP) + ABS_EPSILON;
    p_plus_h = *params; p_plus_h.k += h_k; p_minus_h = *params; p_minus_h.k -= h_k;
    jacobian_row[0] = (model_function_double(&p_plus_h, w, type) - model_function_double(&p_minus_h, w, type)) / (2.0 * h_k);
    double h_w0 = fabs(params->w0 * REL_STEP) + ABS_EPSILON;
    p_plus_h = *params; p_plus_h.w0 += h_w0; p_minus_h = *params; p_minus_h.w0 -= h_w0;
    jacobian_row[1] = (model_function_double(&p_plus_h, w, type) - model_function_double(&p_minus_h, w, type)) / (2.0 * h_w0);
    double h_q = fabs(params->q * REL_STEP) + ABS_EPSILON;
    p_plus_h = *params; p_plus_h.q += h_q; p_minus_h = *params; p_minus_h.q -= h_q;
    jacobian_row[2] = (model_function_double(&p_plus_h, w, type) - model_function_double(&p_minus_h, w, type)) / (2.0 * h_q);
}

static void lm_calc_jacobian_2_params_double(const FilterParams_t_double* params, double w, FilterType_t type, double jacobian_row[2]) {
    FilterParams_t_double p_plus_h, p_minus_h;
    const double REL_STEP = 1e-7; const double ABS_EPSILON = 1e-9;
    double h_w0 = fabs(params->w0 * REL_STEP) + ABS_EPSILON;
    p_plus_h = *params; p_plus_h.w0 += h_w0; p_minus_h = *params; p_minus_h.w0 -= h_w0;
    jacobian_row[0] = (model_function_double(&p_plus_h, w, type) - model_function_double(&p_minus_h, w, type)) / (2.0 * h_w0);
    double h_q = fabs(params->q * REL_STEP) + ABS_EPSILON;
    p_plus_h = *params; p_plus_h.q += h_q; p_minus_h = *params; p_minus_h.q -= h_q;
    jacobian_row[1] = (model_function_double(&p_plus_h, w, type) - model_function_double(&p_minus_h, w, type)) / (2.0 * h_q);
}

static int solve_linear_system_3x3_double(const double A[3][3], const double b[3], double x[3]) {
    double detA = A[0][0]*(A[1][1]*A[2][2] - A[2][1]*A[1][2]) - A[0][1]*(A[1][0]*A[2][2] - A[1][2]*A[2][0]) + A[0][2]*(A[1][0]*A[2][1] - A[1][1]*A[2][0]);
    if (fabs(detA) < 1e-40) return -1;
    double inv_detA = 1.0 / detA;
    x[0] = ((A[1][1]*A[2][2] - A[2][1]*A[1][2])*b[0] + (A[0][2]*A[2][1] - A[0][1]*A[2][2])*b[1] + (A[0][1]*A[1][2] - A[0][2]*A[1][1])*b[2]) * inv_detA;
    x[1] = ((A[1][2]*A[2][0] - A[1][0]*A[2][2])*b[0] + (A[0][0]*A[2][2] - A[0][2]*A[2][0])*b[1] + (A[1][0]*A[0][2] - A[0][0]*A[1][2])*b[2]) * inv_detA;
    x[2] = ((A[1][0]*A[2][1] - A[2][0]*A[1][1])*b[0] + (A[2][0]*A[0][1] - A[0][0]*A[2][1])*b[1] + (A[0][0]*A[1][1] - A[1][0]*A[0][1])*b[2]) * inv_detA;
    return 0;
}

static int solve_linear_system_2x2_double(const double A[2][2], const double b[2], double x[2]) {
    double detA = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (fabs(detA) < 1e-40) return -1;
    double inv_detA = 1.0 / detA;
    x[0] = (A[1][1] * b[0] - A[0][1] * b[1]) * inv_detA;
    x[1] = (-A[1][0] * b[0] + A[0][0] * b[1]) * inv_detA;
    return 0;
}

static double log_interp_double(double f1, double g1, double f2, double g2, double target_g) {
    if (fabs(g2 - g1) < 1e-9) return f1;
    return pow(10.0, log10(f1) + (log10(f2) - log10(f1)) * (target_g - g1) / (g2 - g1));
}


//==============================================================================
// --- Feature Analysis Section (Can remain float-based for performance) ---
//==============================================================================

static void generate_frequency_response_for_analysis(const FilterParams_t_double* params, FilterType_t type, MeasurementPoint_t* output_data, int num_points) {
    double f_start_hz = 100.0;
    double f_end_hz = 500000.0;
    double log_f_start = log10(f_start_hz);
    double log_f_end = log10(f_end_hz);

    for (int i = 0; i < num_points; i++) {
        double log_f = log_f_start + (log_f_end - log_f_start) * i / (num_points - 1);
        double current_f = pow(10.0, log_f);
        output_data[i].frequency_hz = (float)current_f;
        output_data[i].gain = (float)model_function_double(params, 2.0 * M_PI * current_f, type);
    }
}

static FilterType_t identify_filter_type_from_response(const MeasurementPoint_t* data, int num_points) {
    if (num_points < 50) return FILTER_TYPE_UNKNOWN;

    int num_avg_points = num_points / 10;
    if (num_avg_points < 2) num_avg_points = 2;
    float gain_low_sum = 0.0f; for (int i = 0; i < num_avg_points; i++) gain_low_sum += data[i].gain;
    float gain_low = gain_low_sum / num_avg_points;
    float gain_high_sum = 0.0f; for (int i = num_points - num_avg_points; i < num_points; i++) gain_high_sum += data[i].gain;
    float gain_high = gain_high_sum / num_avg_points;
    
    float gain_max = 0.0f, gain_min = FLT_MAX;
    int max_gain_index = -1, min_gain_index = -1;
    for (int i = 0; i < num_points; i++) {
        if (data[i].gain > gain_max) { gain_max = data[i].gain; max_gain_index = i; }
        if (data[i].gain < gain_min) { gain_min = data[i].gain; min_gain_index = i; }
    }

    float gain_pp = gain_max - gain_min;
    if (gain_pp < 0.1f) { return (gain_low > gain_high) ? FILTER_TYPE_LPF : FILTER_TYPE_HPF; }

    const float BORDER_RATIO = 0.15f;
    int core_start_index = (int)(num_points * BORDER_RATIO);
    int core_end_index = (int)(num_points * (1.0f - BORDER_RATIO));

    int is_peak_in_middle = (max_gain_index > core_start_index) && (max_gain_index < core_end_index);
    int is_notch_in_middle = (min_gain_index > core_start_index) && (min_gain_index < core_end_index);
    
    const float SIGNIFICANCE_FACTOR = 0.30f;
    float significant_change = gain_pp * SIGNIFICANCE_FACTOR;

    if (is_peak_in_middle) {
        int is_left_wall = (gain_max > gain_low + significant_change);
        int is_right_wall = (gain_max > gain_high + significant_change);
        if (is_left_wall && is_right_wall) return FILTER_TYPE_BPF;
        if (is_right_wall) return FILTER_TYPE_LPF;
        if (is_left_wall) return FILTER_TYPE_HPF;
    }
    
    if (is_notch_in_middle) {
        int is_left_high = (gain_min < gain_low - significant_change);
        int is_right_high = (gain_min < gain_high - significant_change);
        if (is_left_high && is_right_high) return FILTER_TYPE_BSF;
    }

    return (gain_low > gain_high) ? FILTER_TYPE_LPF : FILTER_TYPE_HPF;
}