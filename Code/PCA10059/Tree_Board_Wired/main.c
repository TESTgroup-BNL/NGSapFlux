/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "nrf.h"
#include "nrfx_gpiote.h"
#include "app_error.h"
#include "boards.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrf_sortlist.h"
#include "nrfx_pwm.h"


///////////////////////////// User Defined Varibles /////////////////////////////////////
// Site number (0-9)
const uint32_t site = 5;
// Subsite number (0-9)
const uint32_t sub_site = 2;
// Tree
const uint32_t Tree = 5;

// Resistance of heater
const float Heat_R = 14.54;

// Min Voltage before disabling heater and giving error
const float Min_Volt = 21.0;
// Watts to deliver to heater
const uint8_t Heat_Watts = 8;
// How long to run the heater in milliseconds
const uint16_t Heat_time = 2000;
// PWM frequency in hz, max 10,000, min 500
const uint16_t PWMfreq = 10000;
// How long between pulses in seconds
const uint32_t Meas_Freq = 1785 * 1000;  // 29min 45sec = 1785

/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Define pins ///////////////////////////////////////////////
/// P0.XX = XX
/// P1.XX = XX + 32
#define HT_EN 47
#define HT_CTL 45
#define HT_Sig 31
#define BATT_V 2  //AIN0
#define SD_EN 42
#define SD_CS 10
#define MISO 24
#define MOSI 9
#define SCK 32
#define SDA 13
#define SCL 15
#define MUX_EN 17
#define MUX_A 22
#define MUS_B 20
#define Button 38
/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Define Other Global Varibles //////////////////////////////
// Conversion factor for ADC value to volts for battery rail (R1+R2)/R2 * Vref/ADCres
const float V_per_lsb_BATT = (51.7F/4.7F) * (3.0F/4096.0F);
// Battery rail voltage
volatile float V_BATT;
// PWM scaling varible
volatile float PWM_Scale;
// PWM Freq = 1/CounterTop * 16,000,000/2^PWM_Prescaler
// 10kHz PWM = 1/1600 * 16,000,000/2^0
// counter top of PWM, min value 3, max value 32,767
uint16_t CounterTop;
// PWM clock prescaler, base clock 16MHz, integer 0-7
const uint8_t PWM_Prescaler = 0;
// PWM value
static uint16_t PWM_Value;
// calculate PWM playback count
const uint16_t Heat_CNT = (Heat_time * PWMfreq) / 1000;
// Start PWM_flag
volatile bool startPWM;
// Full tree ID
static uint32_t Full_Tree_ID;
// HT Wait Time
const uint32_t HT_Wait_Time = ((Tree - 1) * 2) * 1000;

volatile bool waiting = false;
volatile bool sleep = false;
volatile bool Heater_Armed = false;
volatile bool HT_Start = false;
volatile bool HT_Wait_flag = true;

APP_TIMER_DEF(Heater_Arm_Timer);
APP_TIMER_DEF(Sleep);
APP_TIMER_DEF(HT_Wait);

volatile bool Button_int = false;
static nrfx_pwm_t m_pwm0 = NRFX_PWM_INSTANCE(0);
/////////////////////////// Functions ////////////////////////////////////////

void Button_ISR(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){
    HT_Start = true;
    Heater_Armed = true;
}

void HT_Sig_ISR(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){
    HT_Start=true;
    //nrf_gpio_pin_toggle(LED1_G);
}

void HT_Wait_ISR(void * p_contex){
    HT_Wait_flag=false;
}

void Heater_Arm_ISR(void * p_contex){
    Heater_Armed = true;
}

void sleep_timeout_ISR(void * p_contex){
    sleep = false;
}


void HFCLK_int(){
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

void LFCLK_int(){
    // use 32khz crystal
    NRF_CLOCK->LFCLKSRC = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

void PWM_int(){
    nrfx_pwm_config_t const config0 =
    {
        .output_pins =
        {
            HT_CTL,
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

void start_pwm(){
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

uint32_t saadc_read(){
    static uint32_t output;

    NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_12bit << SAADC_RESOLUTION_VAL_Pos;
    NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos;
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;
    NRF_SAADC->CH[0].CONFIG = SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos |
                                SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos |
                                SAADC_CH_CONFIG_GAIN_Gain1_5 << SAADC_CH_CONFIG_GAIN_Pos |
                                SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos |
                                SAADC_CH_CONFIG_TACQ_40us << SAADC_CH_CONFIG_TACQ_Pos|
                                SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos |
                                SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos;
    NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_AnalogInput0 << SAADC_CH_PSELP_PSELP_Pos;

    NRF_SAADC->RESULT.PTR = (uint32_t)&output;
    NRF_SAADC->RESULT.MAXCNT = 1U;

    // Calibrate the SAADC
    NRF_SAADC->EVENTS_CALIBRATEDONE = 0U;
    NRF_SAADC->TASKS_CALIBRATEOFFSET = 1U;
    while (NRF_SAADC->EVENTS_CALIBRATEDONE == 0U){
        //wait
    }

    while (NRF_SAADC->STATUS == (SAADC_STATUS_STATUS_Busy << SAADC_STATUS_STATUS_Pos)){
        //wait
    }

    NRF_SAADC->EVENTS_STOPPED = 0U;
    NRF_SAADC->TASKS_STOP = 1;
    while (NRF_SAADC->EVENTS_STOPPED == 0){
        //wait
    }
 
    //Start SAADC
    NRF_SAADC->EVENTS_STARTED = 0U;
    NRF_SAADC->TASKS_START = 1U;
    while(NRF_SAADC->EVENTS_STARTED == 0U){
        //wait
    }

    //Sample
    NRF_SAADC->EVENTS_END = 0U;
    NRF_SAADC->TASKS_SAMPLE = 1U;
    while(NRF_SAADC->EVENTS_END == 0U){
        //wait
    }

    //Stop SAADC
    NRF_SAADC->EVENTS_STOPPED = 0U;
    NRF_SAADC->TASKS_STOP = 1U;
    while(NRF_SAADC->EVENTS_STOPPED == 0U){
        //wait
    }

    //Disable SAADC
    NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos;

    return(output);
    
}

void setup(){
    // turn on DC/DC buck converters and enable low power mode
    NRF_POWER->DCDCEN0 = POWER_DCDCEN0_DCDCEN_Enabled << POWER_DCDCEN0_DCDCEN_Pos;
    NRF_POWER->DCDCEN = POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos;
    NRF_POWER->TASKS_LOWPWR = POWER_TASKS_LOWPWR_TASKS_LOWPWR_Trigger << POWER_TASKS_LOWPWR_TASKS_LOWPWR_Pos;

    // calculate Full Tree ID
    Full_Tree_ID = site*100 + sub_site*10 + Tree;
    // calculate PWM CounterTop    
    CounterTop = (uint16_t) round(16000000 / PWMfreq);

    //setup output pins
    nrf_gpio_cfg_output(HT_EN);
    nrf_gpio_pin_write(HT_EN, false);
    nrf_gpio_cfg_output(HT_CTL);
    nrf_gpio_pin_write(HT_CTL, false);

    //setup LED's
    nrf_gpio_cfg_output(LED2_R);
    nrf_gpio_pin_write(LED2_R, true);
    nrf_gpio_cfg_output(LED1_G);
    nrf_gpio_pin_write(LED1_G, true);

    //setup PWM
    PWM_int();
    
    //setup timers
    app_timer_init();
    app_timer_create(&Heater_Arm_Timer, APP_TIMER_MODE_SINGLE_SHOT, Heater_Arm_ISR);
    app_timer_create(&Sleep, APP_TIMER_MODE_SINGLE_SHOT, sleep_timeout_ISR);
    app_timer_create(&HT_Wait, APP_TIMER_MODE_SINGLE_SHOT, HT_Wait_ISR);

     //enable gpioe interupts
    nrfx_gpiote_init();

    // Setup Button interupt
    nrfx_gpiote_in_config_t button_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    button_config.pull = NRF_GPIO_PIN_PULLUP;
    nrfx_gpiote_in_init(Button, &button_config, Button_ISR);
    nrfx_gpiote_in_event_enable(Button, true);

    nrfx_gpiote_in_config_t HT_Sig_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    nrfx_gpiote_in_init(HT_Sig, &HT_Sig_config, HT_Sig_ISR);
    nrfx_gpiote_in_event_enable(HT_Sig, true);

}

int main(void){
    // This code runs once
    setup();

    // Start LFCLK
    LFCLK_int();

    //Wait for settling
    nrf_delay_ms(5000);

    Heater_Armed = true;
    HT_Start = false;

    while(true){
        if(HT_Start){
            // Turn on Heater power rail
            nrf_gpio_pin_write(HT_EN, true);

            // start Heater Arm timer
            app_timer_start(Heater_Arm_Timer, APP_TIMER_TICKS(Meas_Freq), NULL);

            // start HFCLK
            HFCLK_int();

            // wait for rail to settle
            nrf_delay_ms(10);

            // Get voltage of heater power rail
            V_BATT = saadc_read() * V_per_lsb_BATT;

            if(V_BATT > Min_Volt){
                //calculate PWM duty cycle
                PWM_Scale = (float) Heat_Watts / ((V_BATT*V_BATT)/Heat_R);
                PWM_Value = (uint32_t) CounterTop - round(PWM_Scale * CounterTop) + 25; // +160 => subtracts 10us to account for fall time of fet
                // One heater at a time
                if(Tree == 1){
                    // no wait
                } else {
                    HT_Wait_flag = true;
                    app_timer_start(HT_Wait, APP_TIMER_TICKS(HT_Wait_Time), NULL);
                    while(HT_Wait_flag){
                        __WFI();
                    }
                }
                // start heater
                start_pwm();
                //nrf_gpio_pin_toggle(LED1_G);
            } else {
                nrf_gpio_pin_write(LED2_R, false);
            }

            //Wait for pwm to stop
            app_timer_start(Sleep, APP_TIMER_TICKS(2100), NULL);
            sleep = true;
            while(sleep){
                __WFI();
            }

            // Turn off error LED
            nrf_gpio_pin_write(LED2_R, true);

            // Stop HFCLK
            NRF_CLOCK->TASKS_HFCLKSTOP = 1U;

            // Turn off Heater power rail
            nrf_gpio_pin_write(HT_EN, false);

            //reset flags
            Heater_Armed = false;
            HT_Start = false;
        }
        // wait for interupt
        __WFI();
    }
}

