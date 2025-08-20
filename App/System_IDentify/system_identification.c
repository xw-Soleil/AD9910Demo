#include "system_identification.h"
#include "expert_fitter_double.h"
#include "accurate_fft.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <float.h>
#include "Correct.h"
#include "AD9910.h"
#include "app_config.h"

#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "process.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"


float getCorrectCoe(float target_freq);


// --- Module-internal State Definition ---
typedef enum {
    STATE_IDLE,
    STATE_START_SWEEP,
    STATE_MEASURING,
    STATE_PROCESS_RESULTS, // New state to handle processing in main loop
    STATE_IDENTIFY_AND_FIT,
    STATE_DONE
} SystemState_t;

// --- Private Variables ---
static volatile SystemState_t g_system_state = STATE_IDLE;
static uint16_t g_sine_table[SINE_TABLE_SIZE];
static uint16_t g_adc_buf[ADC_BUFFER_SIZE];
static MeasurementPoint_t g_measured_data[SWEEP_POINTS];
static uint32_t g_current_sweep_index = 0;


///////////////////////////////////////////
// This single struct will hold all results: params, SSE, and both filter types.
static ExpertFitResult_t_double g_fit_result; 
////////////////////////////////////////////

static volatile uint32_t g_avg_sample_count = 0;
static float g_accumulated_gain = 0.0f;

static AccurateFFT_Handle g_fft_handle;
static float32_t g_fft_input_buf[ADC_BUFFER_SIZE];
static float32_t g_fft_input_buf2[ADC_BUFFER_SIZE*2];

// --- Private Function Prototypes ---
static void start_measurement(void);
static void stop_measurement(void);

static void process_fft_results(void);
static void identify_and_fit(void);
static const char* get_filter_type_string(FilterType_t type);
static void update_adc_sampling_rate(float new_rate);

// --- Public Function Implementations ---

void SysId_Init(void) {
    printf("\r\n--- System Identification Module Initializing ---\r\n");
    generate_sine_table(DAC_AMP_DEFAULT / 2.0f);

    printf("Initializing FFT module...\r\n");
    arm_status fft_status = AccurateFFT_Init(&g_fft_handle, ADC_BUFFER_SIZE, MIN_ADC_SAMPLING_FREQ, WINDOW_TYPE_FLATTOP);
    if (fft_status != ARM_MATH_SUCCESS) {
        printf("FATAL: FFT Module Init Failed! Status: %d\r\n", fft_status);
        Error_Handler();
    }
    
    g_current_sweep_index = 0;
    g_system_state = STATE_START_SWEEP;
    printf("Initialization complete. Starting sweep...\r\n");
}

void SysId_RunStateMachine(void) {
    switch (g_system_state) {
        case STATE_START_SWEEP:
            if (g_current_sweep_index < SWEEP_POINTS) {
                float log_start = log10f(SWEEP_FREQ_START);
                float log_end = log10f(SWEEP_FREQ_END);
                float current_freq = powf(10, log_start + (log_end - log_start) * g_current_sweep_index / (float)(SWEEP_POINTS - 1));
                g_measured_data[g_current_sweep_index].frequency_hz = current_freq;

                float ideal_sampling_freq = (ADC_BUFFER_SIZE * current_freq) / TARGET_CYCLES_IN_BUFFER;
                float new_sampling_freq = fmaxf(ideal_sampling_freq, MIN_ADC_SAMPLING_FREQ);
                new_sampling_freq = fminf(new_sampling_freq, MAX_ADC_SAMPLING_FREQ);

                printf("Sweep [%2lu/%d]: %.2f Hz (ADC Fs: %.1f kHz)\r\n", g_current_sweep_index + 1, SWEEP_POINTS, current_freq, new_sampling_freq / 1000.0f);

                update_adc_sampling_rate(new_sampling_freq);
                g_fft_handle.sampling_rate = new_sampling_freq;

                g_avg_sample_count = 0;
                g_accumulated_gain = 0.0f;
                
                // start_sine_output(current_freq);
#ifdef USE_DAC_OUTPUT
                start_sine_output(current_freq);
#else
                //DDSOutputCorrSamInBord(current_freq, DAC_AMP_DEFAULT);
                SetDacOutputVpp(3.0f, current_freq);
#endif
                HAL_Delay(50);
                
                g_system_state = STATE_MEASURING;
                start_measurement();

            } else {
                stop_measurement();
#ifdef USE_DAC_OUTPUT   
                stop_sine_output();
#else
                DDS_Stop();
#endif
                printf("Sweep finished. Starting identification and fitting...\r\n");
                g_system_state = STATE_IDENTIFY_AND_FIT;
            }
            break;

        case STATE_MEASURING:
            // This state now simply waits for the ISR to change the state.
            // No action needed here in the main loop.
            break;

        case STATE_PROCESS_RESULTS:
            // The ISR has flagged that data is ready. Process it here in the main loop.
            stop_measurement(); // Safely stop ADC and Timer from non-ISR context.
            process_fft_results(); // Process the collected data.
            break;

        case STATE_IDENTIFY_AND_FIT:
            identify_and_fit();
            printf("========================================\r\n");
            printf("Identification and Fitting Complete!\r\n");
            
            // Print the two different filter types as requested
            printf("1. Preliminary Type (from fit): %s\r\n", Expert_Fitter_GetTypeString(g_fit_result.preliminary_type));
            printf("2. Final Type (from analysis):  %s\r\n", Expert_Fitter_GetTypeString(g_fit_result.final_type));
            
            // Calculate f0 in Hz from w0 in rad/s for printing
            double fitted_f0_hz = g_fit_result.params.w0 / (2.0 * M_PI);

            printf("Fitted Parameters (double precision):\r\n");
            // NOTE: Use "%f" or "%lf" for printing doubles, ensure printf float support is enabled in project settings.
            printf("  k  (Gain)    : %.4f\r\n", g_fit_result.params.k);
            printf("  f0 (Freq)    : %.2f Hz\r\n", fitted_f0_hz);
            printf("  Q  (Q-Factor): %.4f\r\n", g_fit_result.params.q);
            printf("  Final SSE    : %.6e\r\n", g_fit_result.sse);
            printf("========================================\r\n");
            g_system_state = STATE_DONE;
            break;
            
        case STATE_IDLE:
        case STATE_DONE:
        default:
            // Do nothing
            break;
    }
}

bool SysId_IsDone(void) {
    return g_system_state == STATE_DONE;
}



// This function needs to be updated to return the new double-precision struct
FilterParams_t_double SysId_GetFittedParams(void) {
    return g_fit_result.params;
}

// Returns the type determined by the preliminary fitting process
FilterType_t SysId_GetPreliminaryFilterType(void) {
    return g_fit_result.preliminary_type;
}

// Returns the final, more robust type from feature analysis
FilterType_t SysId_GetFinalFilterType(void) {
    return g_fit_result.final_type;
}

// --- Callbacks to be called from ISRs ---

void SysId_ADCCallback(void) {
    if (g_system_state == STATE_MEASURING) {
        // Data is ready. Set a flag for the main loop to process it.
        // DO NOT call HAL functions directly in ISR.
        g_system_state = STATE_PROCESS_RESULTS;
    }
}

void SysId_ADCErrorCallback(void) {
    if (HAL_IS_BIT_SET(adciir.ErrorCode, HAL_ADC_ERROR_OVR)) {
        printf("\r\n!!! ADC Overrun Error at sweep point %lu !!!\r\n", g_current_sweep_index);
        
        // Stop everything to ensure a clean state
        stop_measurement();
        stop_sine_output();
        
        // Mark the current measurement as failed and skip to the next point
        g_measured_data[g_current_sweep_index].gain = 0.0f;
        printf("  -> Measurement failed. Marking gain as 0. Moving to next point.\r\n");
        
        g_current_sweep_index++;
        g_system_state = STATE_START_SWEEP; // Go to the next sweep point
    } else {
         printf("\r\n!!! ADC Error (Code: %lu) at sweep point %lu !!!\r\n", adciir.ErrorCode, g_current_sweep_index);
    }
}


// --- Private Function Implementations ---

static void process_fft_results(void) {

    float current_corr = getCorrectCoe(g_measured_data[g_current_sweep_index].frequency_hz);
    float32_t dc_offset = 0.0f;
    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
        g_fft_input_buf[i] = ((float32_t)g_adc_buf[i] * ADC_VREF) / ADC_MAX_VAL / current_corr;
        dc_offset += g_fft_input_buf[i];
    }
    dc_offset /= ADC_BUFFER_SIZE;
    for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
        g_fft_input_buf[i] -= dc_offset;
    }

    // //校正:使用采样板
    // for (int i = 0; i < ADC_BUFFER_SIZE; i++) {
    //     g_fft_input_buf[i] = ADCSampleInputCorr(g_measured_data[g_current_sweep_index].frequency_hz, g_fft_input_buf[i]);
    // }

    //float current_corr = getCorrectCoe(g_measured_data[g_current_sweep_index].frequency_hz);
    



    if (AccurateFFT_Measure(&g_fft_handle, g_fft_input_buf) != ARM_MATH_SUCCESS) {
         printf("  -> FFT Measurement Failed!\r\n");
    } else {
        float current_gain = g_fft_handle.result.corrected_amplitude / (3.0f/2); // Scale to 3.0V
        if (isinf(current_gain) || isnan(current_gain)) {
            current_gain = 0.0f;
        }
        g_accumulated_gain += current_gain;
        g_avg_sample_count++;
    }

    if (g_avg_sample_count >= NUM_BUFFERS_FOR_AVG) {
        float average_gain = g_accumulated_gain / g_avg_sample_count;
        g_measured_data[g_current_sweep_index].gain = average_gain;
        printf("  -> Measured Avg. Gain: %.4f (from %.1f Hz)\r\n", average_gain, g_fft_handle.result.measured_frequency);

        g_current_sweep_index++;
        g_system_state = STATE_START_SWEEP;
    } else {
        // Need more samples for averaging, restart the measurement
        g_system_state = STATE_MEASURING;
        start_measurement();
    }
}


static void identify_and_fit(void) {
    printf("Starting expert identification and fitting (double precision)...\r\n");

    // Call the new, all-in-one expert fitter function.
    // It takes the measured data and fills our global result struct.
    Expert_Fitter_Run_Double(g_measured_data, SWEEP_POINTS, &g_fit_result);
    
    // The fitting is now complete. The results are in g_fit_result.
    // The main state machine will handle printing them.
    printf("Expert fitting process finished.\r\n");
}


void generate_sine_table(float peak_voltage) {
    // ... (This function remains unchanged) ...
    uint16_t offset = DAC_MAX_VAL / 2;
    uint16_t amplitude = (uint16_t)((peak_voltage * DAC_MAX_VAL) / DAC_VREF);
    if (amplitude > offset) {
        amplitude = offset;
    }
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        g_sine_table[i] = (uint16_t)(offset + (int16_t)(amplitude * sinf((2 * (float)M_PI * i) / SINE_TABLE_SIZE)));
    }
}

void start_sine_output(float frequency) {
    uint32_t tim_clk = HAL_RCC_GetPCLK1Freq() * 2;
    uint32_t arr_val = (uint32_t)(tim_clk / (frequency * SINE_TABLE_SIZE)) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim6, arr_val);
    HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)g_sine_table, SINE_TABLE_SIZE, DAC_ALIGN_12B_R);
    HAL_TIM_Base_Start(&htim6);
}

void stop_sine_output() {
    HAL_TIM_Base_Stop(&htim6);
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
}

static void start_measurement() {
    HAL_ADC_Start_DMA(&adciir, (uint32_t*)g_adc_buf, ADC_BUFFER_SIZE);
    HAL_TIM_Base_Start(&htim2);
}

static void stop_measurement() {
    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_DMA(&adciir);
}

static void update_adc_sampling_rate(float new_rate) {
    uint32_t tim_clk_adc = HAL_RCC_GetPCLK1Freq() * 2;
    uint32_t arr_val_adc = (uint32_t)(tim_clk_adc / new_rate) - 1;
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr_val_adc);
}

static const char* get_filter_type_string(FilterType_t type) {
    switch(type) {
        case FILTER_TYPE_LPF: return "Low-Pass";
        case FILTER_TYPE_HPF: return "High-Pass";
        case FILTER_TYPE_BPF: return "Band-Pass";
        case FILTER_TYPE_BSF: return "Band-Stop";
        default: return "Unknown";
    }
}
