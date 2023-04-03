#include "boards.h"
#include <stdint.h>
#include <stdbool.h>

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