

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>

#define     __IO    volatile             /*!< Defines 'read / write' permissions */

// **************************************************************************************
//      select board pins & options
	
// define TBOOKREV2B for Feb2020 Rev2B board -- here or in target_options/C/Define
// #define TBOOKREV2B	
#if defined( TBOOKREV2B )
	#include "tbook_rev2.h"
#endif

// define STM3210E_EVAL for STM3210E_EVAL board  -- here or in target_options/C/Define
// #define STM3210E_EVAL
#if defined( STM3210E_EVAL )
	#include "stm3210e_eval.h"
#endif

// define STM32F412_DISCOG for Stm32F412_DiscoveryG board   -- here or in target_options/C/Define
// #define STM32F412_DISCOG
#if defined( STM32F412_DISCOG )
	#include "stm32f412_discog.h"
#endif


// define TBOOKREV2A for Sep2018 RevA board -- here or in target_options/C/Define
// #define TBOOKREV2A	
#if defined( TBOOKREV2A )
	#include "tbook_rev2a.h"
#endif

#endif /* __MAIN_H */

