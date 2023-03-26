#include <stdint.h>
#include <stdbool.h>
#include "boards.h"
#include "nrf_delay.h"

///////////////////////////// User Defined Varibles /////////////////////////////////////
// Site number
const uint8_t Site = 5;
// Subsite number
const uint8_t Sub_Site = 2;
// Tree
const uint8_t Tree = 3;

// Resistance of heater
const float Heat_R = 13.54;
/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Define Other Global Varibles //////////////////////////////
// Convert site, subsite, and tree into UICR code
const uint32_t Tree_Code = ~ (Site << 6 | Sub_Site << 3 | Tree);
// Convert resistance of heater to UICR code
const uint8_t tens = (uint16_t) (Heat_R / 10);
const uint8_t ones = (uint16_t) Heat_R - tens * 10;
const uint8_t tenths = (uint16_t) (10 * Heat_R) - 100 * tens - 10 * ones;
const uint8_t hundredths = (uint16_t) (100 * Heat_R) - 1000 * tens - 100 * ones - 10 * tenths;
const uint32_t Heat_Code = ~ (tens << 12 | ones << 8 | tenths << 4 | hundredths);

// Varibles to read from registers
uint8_t Site_Reg ;
uint8_t Sub_Site_Reg ;
uint8_t Tree_Reg;
uint32_t Tree_Code_Reg ;
uint8_t tens_Reg ;
uint8_t ones_Reg ;
uint8_t tenths_Reg ;
uint8_t hundredths_Reg ;
uint32_t Heat_Code_Reg ;

/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Functions /////////////////////////////////////////////////

static void Write_UICR(uint32_t Reg0, uint32_t Reg1){
  // Write to Reg if not already written
  if(NRF_UICR->CUSTOMER[0] == 0xFFFFFFFF){
    // Set NVMC to write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Write to register
    NRF_UICR->CUSTOMER[0] = Reg0;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Set NVMC to read-only
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Reset NVIC
    NVIC_SystemReset();
  }

  // Write to Reg if not already written
  if(NRF_UICR->CUSTOMER[1] == 0xFFFFFFFF){
    // Set NVMC to write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Write to register
    NRF_UICR->CUSTOMER[1] = Reg1;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Set NVMC to read-only
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Reset NVIC
    NVIC_SystemReset();
  }

}

void LED_Blink_Codes(uint8_t value, uint8_t LED, uint16_t Blink_Delay, uint16_t Delay_BTW_Code){
  nrf_gpio_pin_write(LED, false);

  for(int i = 0; i < value; i++){
    nrf_delay_ms(Blink_Delay);
    nrf_gpio_pin_write(LED1_G, false);
    nrf_delay_ms(Blink_Delay);
    nrf_gpio_pin_write(LED1_G, true);
  }

  nrf_gpio_pin_write(LED, true);
  nrf_delay_ms(Delay_BTW_Code);
  
}

uint8_t Decode(uint32_t Code, uint8_t BitShift, uint32_t Comp){
  uint8_t decoded = (Code >> BitShift) & Comp;
  return(decoded);
}

void setup(void){
  nrf_gpio_cfg_output(LED1_G);
  nrf_gpio_pin_write(LED1_G, true);
  nrf_gpio_cfg_output(LED2_B);
  nrf_gpio_pin_write(LED2_B, true);
  nrf_gpio_cfg_output(LED2_R);
  nrf_gpio_pin_write(LED2_R, true);

}

int main(void){
  setup();
  // Write to UICR
  Write_UICR(Tree_Code, Heat_Code);

  // wait
  nrf_delay_ms(1000);

  // Read UICR
  Tree_Code_Reg = ~NRF_UICR->CUSTOMER[0];
  Heat_Code_Reg = ~NRF_UICR->CUSTOMER[1];

  // Decode Tree Code
  Site_Reg = Decode(Tree_Code_Reg, 6, 0x7);
  Sub_Site_Reg = Decode(Tree_Code_Reg, 3, 0x7);
  Tree_Reg = Decode(Tree_Code_Reg, 0, 0x7);

  // Decode Heat Code
  tens_Reg = Decode(Heat_Code_Reg, 12, 0xF);
  ones_Reg = Decode(Heat_Code_Reg, 8, 0xF);
  tenths_Reg = Decode(Heat_Code_Reg, 4, 0xF);
  hundredths_Reg = Decode(Heat_Code_Reg, 0, 0xF);
  //convert back to resistance
  //const float Heat_R = (float) (tens*1000 + ones*100 + tenths*10 + hundredths) / 100;
  
  while(true){
    LED_Blink_Codes(Site_Reg, LED2_B, 500, 2000);
    LED_Blink_Codes(Sub_Site_Reg, LED2_B, 500, 2000);
    LED_Blink_Codes(Tree_Reg, LED2_B, 500, 2000);
    nrf_delay_ms(5000);
    LED_Blink_Codes(tens_Reg, LED2_R, 500, 2000);
    LED_Blink_Codes(ones_Reg, LED2_R, 500, 2000);
    LED_Blink_Codes(tenths_Reg, LED2_R, 500, 2000);
    LED_Blink_Codes(hundredths_Reg, LED2_R, 500, 2000);
    nrf_delay_ms(5000);

  }
}