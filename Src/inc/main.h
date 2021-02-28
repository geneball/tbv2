

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>

#define     __IO    volatile             /*!< Defines 'read / write' permissions */

// **************************************************************************************
//      select board pins & options
	

// define TBOOK_V2 for TBook V2 boards (Feb or Dec 2020)-- here or in target_options/C/Define
#define TBOOK_V2	
#if defined( TBOOK_V2 )
	// define TBOOK_V2_Rev3 for Dec2020 TB_V2_Rev3 board
	#define TBook_V2_Rev3

	// define TBOOK_V2_Rev1 for Feb2020 Rev2B board 
//	#define TBook_V2_Rev1

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

