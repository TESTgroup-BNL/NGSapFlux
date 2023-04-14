#include <stdint.h>
#include <stdbool.h>
#include "boards.h"
#include "nrf_delay.h"
#include "app_timer.h"
#include "nrf_sortlist.h"


///////////////////////////// User Defined Varibles /////////////////////////////////////
// Erase flash page
const bool erase_Flash = true;

// Site number
const uint8_t Site = 1;
// Subsite number
const uint8_t Sub_Site = 2;
// Tree
const uint8_t Tree = 1;

// Resistance of heater
const float Heat_R = 12.14;
/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Define Other Global Varibles //////////////////////////////
// Convert site, subsite, and tree into flash code
const uint32_t Tree_Code = ~ (Site << 6 | Sub_Site << 3 | Tree);
// Convert resistance of heater to flash code
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

volatile bool sleep = false;

APP_TIMER_DEF(Sleep);

int volatile * const Site_Code_Reg = (int *) 0xDF000UL;
int volatile * const Resist_Code_Reg = (int *) 0xDF004UL;

/////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////// Functions /////////////////////////////////////////////////
void sleep_timeout_ISR(void * p_contex){
    sleep = false;
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

void Sleep_Time(uint16_t sleep_ms){
    app_timer_start(Sleep, APP_TIMER_TICKS(sleep_ms), NULL);
    sleep = true;
    while(sleep){
        __WFI();
    }
}

static void Write_Flash(uint32_t Reg0, uint32_t Reg1, bool erase){
  if(erase == true){
    erase = false;
     // Set NVMC to erase
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while(NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    
    // Erase page
    NRF_NVMC->ERASEPAGE = 0xDF000UL;

    // Wait for NVMC to finish
    while(NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    
    // Set NVMC to read-only
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

  }

  // Write to Reg if not already written
  if(*Site_Code_Reg == 0xFFFFFFFF){
    // Set NVMC to write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Write to register
    *Site_Code_Reg = Reg0;
    //NRF_UICR->CUSTOMER[0] = Reg0;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}

    // Set NVMC to read-only
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
  }

  // Write to Reg if not already written
  if(*Resist_Code_Reg == 0xFFFFFFFF){
    // Set NVMC to write
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Write to register
    *Resist_Code_Reg = Reg1;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
    // Set NVMC to read-only
    NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

    // Wait for NVMC to finish
    while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
  }

}

void LED_Blink_Codes(uint8_t value, uint8_t LED, uint16_t Blink_Delay, uint16_t Delay_BTW_Code){
  nrf_gpio_pin_write(LED, false);

  for(int i = 0; i < value; i++){
    Sleep_Time(Blink_Delay);
    nrf_gpio_pin_write(LED1_G, false);
    Sleep_Time(Blink_Delay);
    nrf_gpio_pin_write(LED1_G, true);
  }

  nrf_gpio_pin_write(LED, true);
  Sleep_Time(Delay_BTW_Code);
  
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
  

  LFCLK_int();

  app_timer_init();
  app_timer_create(&Sleep, APP_TIMER_MODE_SINGLE_SHOT, sleep_timeout_ISR);

}

int main(void){
  setup();
  // Write to flash
  Write_Flash(Tree_Code, Heat_Code, erase_Flash);

  // wait
  nrf_delay_ms(1000);

  // Read flash
  Tree_Code_Reg = ~*Site_Code_Reg;
  Heat_Code_Reg = ~*Resist_Code_Reg;

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