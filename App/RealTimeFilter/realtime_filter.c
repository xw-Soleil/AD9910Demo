#include "realtime_filter.h"
#include "main.h" // For HAL handles
#include <stdint.h>

// --- DMA Buffers ---
static uint16_t adc_dma_buffer[PING_PONG_SIZE];
static uint16_t dac_dma_buffer[PING_PONG_SIZE];


extern BiquadCoeffs iir_coeffs_corr; // Coefficients for the IIR filter


// --- IIR Filter State Variables ---
#if USE_FLOAT_IIR
// --- Float implementation (Recommended) ---
static float b0, b1, b2, a1, a2;
static float x_n1 = 0.0f, x_n2 = 0.0f;
static float y_n1 = 0.0f, y_n2 = 0.0f;
#else
// --- Fixed-point implementation ---
#define IIR_SHIFT 15
static int32_t b0_q15, b1_q15, b2_q15, a1_q15, a2_q15;
static int32_t x_n1 = 0, x_n2 = 0;
static int32_t y_n1 = 0, y_n2 = 0;
#endif

// --- Private Function Prototypes ---
static uint16_t process_sample(uint16_t sample);

void RealtimeFilter_Init(const BiquadCoeffs* coeffs) {
    printf("\r\n--- Real-Time IIR Filter Initializing ---\r\n");
    printf("Coeffs: b0=%.4f, b1=%.4f, b2=%.4f, a1=%.4f, a2=%.4f\r\n",
           coeffs->b0, coeffs->b1, coeffs->b2, coeffs->a1, coeffs->a2);

    // Initialize DAC buffer to silence (DC offset)
    for (int i = 0; i < PING_PONG_SIZE; i++) {
        dac_dma_buffer[i] = DC_OFFSET;
    }

#if USE_FLOAT_IIR
    b0 = (float)coeffs->b0;
    b1 = (float)coeffs->b1;
    b2 = (float)coeffs->b2;
    a1 = (float)coeffs->a1;
    a2 = (float)coeffs->a2;
#else
    b0_q15 = (int32_t)(coeffs->b0 * (1 << IIR_SHIFT));
    b1_q15 = (int32_t)(coeffs->b1 * (1 << IIR_SHIFT));
    b2_q15 = (int32_t)(coeffs->b2 * (1 << IIR_SHIFT));
    a1_q15 = (int32_t)(coeffs->a1 * (1 << IIR_SHIFT));
    a2_q15 = (int32_t)(coeffs->a2 * (1 << IIR_SHIFT));
#endif

    // Reset filter history
    x_n1 = x_n2 = 0;
    y_n1 = y_n2 = 0;
}

void RealtimeFilter_Start(void) {
    printf("Starting real-time filter pipeline at %.1f kHz...\r\n", REALTIME_SAMPLING_RATE / 1000.0f);

    // --- Configure Timers for Synchronous Operation ---
    // For synchronous ADC/DAC, both triggers should have the same rate.
    uint32_t tim_clk = HAL_RCC_GetPCLK1Freq() * 2;
    uint32_t arr_val = (uint32_t)(tim_clk / REALTIME_SAMPLING_RATE) - 1;
    
    // Configure TIM2 for ADC trigger
    __HAL_TIM_SET_AUTORELOAD(&htim2, arr_val);
    
    // BUG FIX: Configure TIM6 for DAC trigger with the same rate
    __HAL_TIM_SET_AUTORELOAD(&htim6, arr_val);

    // --- Start Peripherals ---
    // Start the DAC peripheral itself to enable conversions
    if (HAL_DAC_Start(&hdac, DAC_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }

    // Start DMA channels for both ADC and DAC
    if (HAL_DAC_Start_DMA(&hdac, DAC_CHANNEL_1, (uint32_t*)dac_dma_buffer, PING_PONG_SIZE, DAC_ALIGN_12B_R) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_ADC_Start_DMA(&adciir, (uint32_t*)adc_dma_buffer, PING_PONG_SIZE) != HAL_OK) {
        Error_Handler();
    }

    // --- Start Timers ---
    // Start both timers to begin triggering ADC and DAC
    if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_Base_Start(&htim6) != HAL_OK) {
        Error_Handler();
    }
}

void RealtimeFilter_Stop(void) {
    // Stop timers first to cease triggers
    HAL_TIM_Base_Stop(&htim2);
    HAL_TIM_Base_Stop(&htim6);

    // Stop DMA channels
    HAL_ADC_Stop_DMA(&adciir);
    HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

    // Stop the DAC peripheral
    HAL_DAC_Stop(&hdac, DAC_CHANNEL_1);
    
    printf("Real-time filter pipeline stopped.\r\n");
}

static uint16_t process_sample(uint16_t sample) {
#if USE_FLOAT_IIR
    float x_n0 = (float)sample - (float)DC_OFFSET;
    float y_n0 = b0 * x_n0 + b1 * x_n1 + b2 * x_n2 - a1 * y_n1 - a2 * y_n2;
    
    // Update state variables
    x_n2 = x_n1; x_n1 = x_n0;
    y_n2 = y_n1; y_n1 = y_n0;
    
    // Add DC offset back and clamp the output
    float dac_output = y_n0 + (float)DC_OFFSET;
    if (dac_output < 0.0f) dac_output = 0.0f;
    if (dac_output > 4095.0f) dac_output = 4095.0f;
    
    return (uint16_t)dac_output;
#else
    int32_t x_n0 = (int32_t)sample - DC_OFFSET;
    
    // Use 64-bit accumulator to prevent overflow during intermediate calculations
    int64_t acc = (int64_t)b0_q15 * x_n0 + (int64_t)b1_q15 * x_n1 + (int64_t)b2_q15 * x_n2
                  - (int64_t)a1_q15 * y_n1 - (int64_t)a2_q15 * y_n2;
    
    int32_t y_n0 = (int32_t)(acc >> IIR_SHIFT);

    // Update state variables
    x_n2 = x_n1; x_n1 = x_n0;
    y_n2 = y_n1; y_n1 = y_n0;

    // Add DC offset back and clamp
    int32_t dac_output = y_n0 + DC_OFFSET;
    if (dac_output > 4095) dac_output = 4095;
    if (dac_output < 0) dac_output = 0;
    
    return (uint16_t)dac_output;
#endif
}

// --- Callback Implementations ---

void RealtimeFilter_ADCHalfCpltCallback(void) {
    // Process the first half of the buffer (Ping)
    for (int i = 0; i < REALTIME_BUFFER_SIZE; i++) {
        dac_dma_buffer[i] = process_sample(apply_hs_corr(adc_dma_buffer[i]));
    }
}

void RealtimeFilter_ADCFullCpltCallback(void) {
    // Process the second half of the buffer (Pong)
    for (int i = 0; i < REALTIME_BUFFER_SIZE; i++) {
        dac_dma_buffer[REALTIME_BUFFER_SIZE + i] = process_sample(apply_hs_corr(adc_dma_buffer[REALTIME_BUFFER_SIZE + i]));
    }
}

void RealtimeFilter_ADCErrorCallback(void) {
    // Dynamic recovery from ADC overrun error
    if (HAL_IS_BIT_SET(adciir.ErrorCode, HAL_ADC_ERROR_OVR)) {
        HAL_ADC_Stop_DMA(&adciir);
        HAL_ADC_Start_DMA(&adciir, (uint32_t*)adc_dma_buffer, PING_PONG_SIZE);
    }
}



static float cb0, cb1, cb2, ca1, ca2;
static float cx_n1 = 0.0f, cx_n2 = 0.0f;
static float cy_n1 = 0.0f, cy_n2 = 0.0f;


void Correct_HSCorr_Init(void) {
    // Initialize the filter coefficients
    

    cb0 = (float)iir_coeffs_corr.b0;
    cb1 = (float)iir_coeffs_corr.b1;
    cb2 = (float)iir_coeffs_corr.b2;
    ca1 = (float)iir_coeffs_corr.a1;
    ca2 = (float)iir_coeffs_corr.a2;
    cx_n1 = cx_n2 = 0.0f;
    cy_n1 = cy_n2 = 0.0f;
}

#define USE_HS
#ifdef USE_HS
uint16_t apply_hs_corr(uint16_t sample) {
    float cx_n0 = (float)sample - (float)DC_OFFSET;

    float cy_n0 = cb0 * cx_n0 + cb1 * cx_n1 + cb2 * cx_n2 - ca1 * cy_n1 - ca2 * cy_n2;
    
    // Update state variables
    cx_n2 = cx_n1; cx_n1 = cx_n0;
    cy_n2 = cy_n1; cy_n1 = cy_n0;
    
    // Add DC offset back and clamp the output
    float dac_output = cy_n0 + (float)DC_OFFSET;
    if (dac_output < 0.0f) dac_output = 0.0f;
    if (dac_output > 4095.0f) dac_output = 4095.0f;
    
    return (uint16_t)dac_output;
}

#else
uint16_t apply_hs_corr(uint16_t sample) {
    // No correction applied, just return the sample
    return sample;
}
#endif // USE_HS




