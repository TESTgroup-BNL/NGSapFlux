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
#include "nrf.h"
#include "nrfx_gpiote.h"
#include "app_error.h"
#include "boards.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrf_sortlist.h"


//////////////////////////// User Defined Varibles ////////////////////////////////////
// How long to keep voltmeter display on (in seconds)
const uint8_t display_on_time = 30;
// Site number (0-9)
const uint32_t site = 1;
// Subsite number (0-9)
const uint32_t sub_site = 1;
// Tree Array (0-9)
const uint32_t Tree_Array[5] = {1, 2, 3, 4, 5};
//////////////////////////////////////////////////////////////////////////////////////

// Define pins
/// P0.XX = XX
/// P1.XX = XX + 32
#define OK_LED 24
#define V_MTR_EN 32
#define CR_C1 42
#define CR_C2 10
#define Button 38
#define HT 9
//#define HT 31

// Define global varibles
const uint32_t display_time_ms = display_on_time * 1000; // convert display on time to ms
volatile bool Button_int = false;
volatile bool HT_Start = false;
volatile bool V_MTR_Disable = false;
volatile bool LED_Timer_ON = false;

// Define arrays
uint8_t LED_Array[6] = {24, 22, 20, 17, 15, 13};
volatile bool Error_Array[5] = {false, false, false, false, false};
static uint32_t Full_Tree_Array[5];

APP_TIMER_DEF(LED_Timer);
/////////////////////////// Functions ////////////////////////////////////////
void LED_Timer_ISR(void * p_contex){
    V_MTR_Disable = true;
}

void check_status_ISR(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){
    if(!Button_int){
        Button_int = true;
    }
}

void Heater_Start_ISR(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action){
        HT_Start = true;
}

void LFCLK_int(){
     /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

void display_status(){
    nrf_gpio_pin_write(V_MTR_EN, true);
    bool AllOK = true;

    for(int i = 0; i < 5; i++){
        if(Error_Array[i]){
            nrf_gpio_pin_write(LED_Array[i+1], Error_Array[i]);
            AllOK = false;
        }
    }
    if(AllOK) nrf_gpio_pin_write(OK_LED, true);
}


void HT_ON(){
    nrf_gpio_pin_write(HT, false);
    nrf_delay_ms(1);
    nrf_gpio_pin_write(HT, true);
    nrf_gpio_pin_write(LED1_G, false);
    nrf_delay_ms(1000);
    nrf_gpio_pin_write(LED1_G, true);

}

void setup(){
    for(int i=0; i<5; i++){
        Full_Tree_Array[i] = site*100 + sub_site*10 + Tree_Array[i];
    }

    nrf_gpio_cfg(HT, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, 
                NRF_GPIO_PIN_H0D1, NRF_GPIO_PIN_NOSENSE);
    nrf_gpio_pin_write(HT, true);


    nrf_gpio_cfg(CR_C2, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, 
                NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE);
    nrf_gpio_pin_write(CR_C2, true);

    nrf_gpio_cfg_output(V_MTR_EN);
    nrf_gpio_pin_write(V_MTR_EN, false);

    nrf_gpio_cfg_output(LED1_G);
    nrf_gpio_pin_write(LED1_G, true);

    for(int j = 0; j < 6; j++){
        nrf_gpio_cfg_output(LED_Array[j]);
        nrf_gpio_pin_write(LED_Array[j], false);
    } 

    LFCLK_int();
    app_timer_init();
    app_timer_create(&LED_Timer, APP_TIMER_MODE_SINGLE_SHOT, LED_Timer_ISR);
}


static void gpio_init(void){
    nrfx_gpiote_init();

    // Setup Button interupt
    nrfx_gpiote_in_config_t button_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    button_config.pull = NRF_GPIO_PIN_PULLUP;
    nrfx_gpiote_in_init(Button, &button_config, check_status_ISR);
    nrfx_gpiote_in_event_enable(Button, true);

    // Setup CR1000 interrupt 
    nrfx_gpiote_in_config_t cr_c1_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);
    nrfx_gpiote_in_init(CR_C1, &cr_c1_config, Heater_Start_ISR);
    nrfx_gpiote_in_event_enable(CR_C1, true);


}

int main(void)
{
    // This code runs once
    setup();
    gpio_init();
    nrf_delay_ms(1000);
    HT_Start = false;

    while (true){
        // This code runs every time the pin is toggled
        if(!LED_Timer_ON){
            if(Button_int){

                app_timer_start(LED_Timer, APP_TIMER_TICKS(display_time_ms), NULL);
                LED_Timer_ON = true;
                display_status();

                //HT_ON();
            }
        }

        if(V_MTR_Disable){
            nrf_gpio_pin_write(V_MTR_EN, false);

            for(int j = 0; j < 6; j++){
                nrf_gpio_pin_write(LED_Array[j], false);
            }

            Button_int = false;
            V_MTR_Disable = false;
            LED_Timer_ON = false;
        }

        if(HT_Start){
            HT_ON();
            HT_Start = false;
        }
        
        __WFI();
    }
}

