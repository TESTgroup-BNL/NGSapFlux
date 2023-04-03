#include <stdint.h>
#include <stdbool.h>
#include "boards.h"
#include "nrfx_pwm.h"

static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);

void PWM_int(uint16_t pin, uint8_t PWM_Prescaler, uint16_t CounterTop){
    nrfx_pwm_config_t const config0 =
    {
        .output_pins =
        {
            pin,
            LED1_G | NRFX_PWM_PIN_INVERTED,     // channel 1
            NRFX_PWM_PIN_NOT_USED,             // channel 2
            NRFX_PWM_PIN_NOT_USED,             // channel 3
        },
        .irq_priority = APP_IRQ_PRIORITY_LOWEST,
        .base_clock   = PWM_Prescaler,
        .count_mode   = NRF_PWM_MODE_UP,
        .top_value    = CounterTop,
        .load_mode    = NRF_PWM_LOAD_COMMON,
        .step_mode    = NRF_PWM_STEP_AUTO
    };
    nrfx_pwm_init(&m_pwm0, &config0, NULL);
    //nrfx_pwm_init(&m_pwm0, &config0, PWM_ISR)
}

void start_pwm(uint16_t PWM_Value, uint16_t Heat_CNT){
    // This array cannot be allocated on stack (hence "static") and it must
    // be in RAM (hence no "const", though its content is not changed).
    uint16_t seq_values[] = {PWM_Value} ;
    nrf_pwm_sequence_t const seq =
    {
        .values.p_common = seq_values,
        .length          = NRF_PWM_VALUES_LENGTH(seq_values),
        .repeats         = 0,
        .end_delay       = 0
    };

    (void)nrfx_pwm_simple_playback(&m_pwm0, &seq, Heat_CNT, NRFX_PWM_FLAG_STOP);
}