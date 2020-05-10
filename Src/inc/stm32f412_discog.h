
// **************************************************************************************
// *****	May2020  Stm32F412_DiscoveryG board 
//  stm32f412_discog.h
//	pins & switches for testing on Stm32F412_DiscoveryG board 
// ********************************* 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32F412_DISCOG_H
#define STM32F412_DISCOG_H
	
#define UID_BASE                     0x1FFF7A10U           /*STM32F412 !< Unique device ID register base address */
//	#include "I2C_STM32F4xx.h"			
#include "stm32f4xx.h"
#include "GPIO_STM32F4xx.h"

// #define AK4343
// I2C slave address for ak4343	--  shifted <<1 by I2C, so differs from HAL 0x27 which is masked with 0xFFFE
#define AUDIO_I2C_ADDRESS                ((uint16_t)0x34)
#define EEPROM_I2C_ADDRESS_A01           ((uint16_t)0xA0)
#define EEPROM_I2C_ADDRESS_A02           ((uint16_t)0xA6)  
#define TS_I2C_ADDRESS                   ((uint16_t)0x70)

// #define USE_LCD
// define NO_PRINTF if user printf (retargetio.c) not included
// #define NO_PRINTF

// define USE_FILESYS if filesystem is included
// #define USE_FILESYS

// #define KEY_PRESSED  1

#include "gpio_signals.h"		// GPIO_IDs

// STM3210E_EVAL  EXTI ints 0 Wk, 3 JDn, 5-9 JCe/Key, 10-15 Tam&JRi/JLe/JUp
static GPIO_Signal gpioSignals[] = {  // GPIO signal definitions
//	{ gHOME,	"PG8_" 		}, //    	Key/User button,  0 when pressed
//	{ gCIRCLE,	""			}, //    	undefined  
//	{ gPLUS,	"PA0" 		}, //    	Wakeup button, 		1 when pressed
	{ gMINUS,	""			}, //    	undefined 
	{ gTREE, 	"PG0_" 		}, //   DISCOG 		JOYSTICK_UP,			0
	{ gLHAND, 	"PF15_" 	}, //   DISCOG 		JOYSTICK_LEFT,		0
	{ gRHAND, 	"PF14_" 	}, //   DISCOG	 	JOYSTICK_RIGHT,		0
	{ gPOT, 	"PA0_" 		}, //   DISCOG 	JOYSTICK_CENTER,	0
	{ gSTAR,	"PC13_" 	}, //   DISCOG 	Tamper button,		0
	{ gTABLE, 	"PG1_" 		}, //   DISCOG 	JOYSTICK_DOWN,		0

	{ gRED,		"PE0_"		}, //   DISCOG 	LED_RED
	{ gGREEN, 	"PE1_"		}, //   DISCOG 	LED_GREEN
	{ gORANGE,  "PE2_"		}, //   DISCOG 	LED_ORANGE
	{ gBLUE, 	"PE3_"		}, //   DISCOG 	LED_BLUE
	
	{ gSD_DETECT, "PD3"		}, //	DISCOG  SD-detect
	{ gTS_INT, 	"PG5"		}, //	DISCOG  TS INT
	{ gTS_RST, 	"PF12"		}, //	DISCOG  TS RST
	{ gCOM1_TX, "PA2|7"		}, //	DISCOG  COM1_TX
	{ gCOM1_RX, "PA3|7"		}, //	DISCOG  COM1_RX
	
//	{ gBOOT1_PDN, "PG11_" },	// codec PDN = PG11
	{ gI2C1_SCL, "PB6|4" 	},	//	DISCOG I2C_SCL = PB6 AF4
	{ gI2C1_SDA, "PB7|4" 	},	//	DISCOG I2C_SDA = PB7 AF4

	{ gUSB_DM, 		"PA11|10"		},	// DISCOG USB_OTGFS_DM (altFn=10)
	{ gUSB_DP, 		"PA12|10"		},	// DISCOG USB_OTGFS_DP (altFn=10)
	{ gUSB_ID, 		"PA10|10"		},	// DISCOG USB_OTGFS_ID (altFn=10)
	{ gUSB_VBUS, 	"PA9|10"		},	// DISCOG USB_OTGFS_VBUS sensing (altFn=10)
	{ gUSB_PWRSW,	"PG8"			},  // DISCOG  USB_OTGFS_PPWR_EN  VBUS generation
	{ gUSB_OVRCR,	"PG7"			},  // DISCOG  USB_OTGFS_OVRCR  FAULT => RED_LED
		
	// DISCOG  SDIO GPIOC  GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	// DISCOG  SDIO GPIOD  GPIO_PIN_2
	// DISCOG  SDIO NVIC_SetPriority(SDIO_IRQn, 0x0E, 0);
	// DISCOG  SDIO DMAx_TX  DMA2_Stream6	DMA_CHANNEL_4
	// DISCOG  SDIO DMAx_RX  DMA2_Stream3	DMA_CHANNEL_4
 	
//	{ gSDIO_DAT0, 		"PC4|12"	},  // SDcard & eMMC SDIO D0  (RTE_Device.h MCI0)  (altFn=12)
//	{ gSDIO_DAT1, 		"PA8|12"	},  // SDcard & eMMC SDIO D1  (RTE_Device.h MCI0)  (altFn=12)
//	{ gSDIO_DAT2, 		"PA9|12"	},  // SDcard & eMMC SDIO D2  (RTE_Device.h MCI0)  (altFn=12)
//	{ gSDIO_DAT3, 		"PB5|12"	},  // SDcard & eMMC SDIO D3  (RTE_Device.h MCI0)  (altFn=12)
//	{ gSDIO_CLK, 		"PC12|12"	},  // SDcard & eMMC SDIO CLK (RTE_Device.h MCI0)  (altFn=12)
//	{ gSDIO_CMD, 		"PD2|12"	},	// SDcard & eMMC SDIO CMD (RTE_Device.h MCI0)  (altFn=12)


	{ gINVALID, ""  }	// end of list
};
#endif
