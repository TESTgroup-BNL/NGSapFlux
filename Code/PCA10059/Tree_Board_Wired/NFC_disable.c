
#include "boards.h"
#include <stdint.h>
#include <stdbool.h>


// Allow NFC pins to be used as GPIO
#if defined(BOARD_PCA10059)
static void nfc_disable(void)
{
  if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk) ==
      (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos))
	{
  		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
   
        NRF_UICR->NFCPINS &= ~UICR_NFCPINS_PROTECT_Msk;
    
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy){}
        NVIC_SystemReset();
    }
}
#endif