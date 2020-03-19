

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include <stdint.h>
#include <stdbool.h>

#define     __IO    volatile             /*!< Defines 'read / write' permissions */

// set switches for hardware platform & conditional software inclusions

// GPIO Signal IDs
// 	  TalkingBook keypad has 10 keys, each connected to a line of the keypad cable-- these have to be 0..9 to match enum KEY
//    10 is reserved (for an invalid KEY)
//    other IDs are assigned for GPIO pins used in different implementations
typedef enum {			// GPIO_ID -- gpio signal IDs  
	gORANGE, gBLUE,		// LEDs on STM3210E_EVAL
  gRED,	gGREEN,  		// LEDs on TBook keypad & STM3210E_EVAL
	gINVALID,		// marker for end of definition array
	gHOME, gCIRCLE, gPLUS, gMINUS, gTREE, gLHAND, gRHAND, gPOT, gSTAR, gTABLE,
// defs for all the other TBV2B gpio assignments
	gADC_LI_ION,		// TBV2B: 	PA2
	gADC_PRIMARY,		// TBV2B: 	PA3
	gPWR_FAIL_N,		// TBV2B: 	PD0
	gSC_ENABLE,			// TBV2B: 	PD1
	gPA_EN,					// TBV2B: 	PD3
	gEN_5V,					// TBV2B: 	PD4
	gNLEN01V8,			// TBV2B: 	PD5
	g3V3_SW_EN,			// TBV2B: 	PD6
	gEMMC_RSTN,			// TBV2B: 	PE10
	gSPI4_NSS,			// TBV2B: 	PE11
	gSPI4_SCK,			// TBV2B: 	PE12
	gSPI4_MISO,			// TBV2B: 	PE13
	gSPI4_MOSI,			// TBV2B: 	PE14
	gADC_ENABLE,		// TBV2B: 	PE15
	gEN_1V8,				// TBV2B: 	PD5
	gUSB_DM,				// TBV2B: 	PA11
	gUSB_DP,				// TBV2B: 	PA12
	gUSB_ID,				// TBV2B: 	PA10
	gI2S2ext_SD,		// TBV2B: 	PB14
	gI2S2_SD,				// TBV2B: 	PB15
	gI2S2_WS,				// TBV2B: 	PB12
	gI2S2_CK,				// TBV2B: 	PB13
	gI2S2_MCK,			// TBV2B: 	PC6
	gI2S3_MCK,			// TBV2B: 	PC7
	gI2C1_SDA,			// TBV2B: 	PB9		STM3210E_EVAL: PB6
	gI2C1_SCL,			// TBV2B: 	PB8		STM3210E_EVAL: PB7
	gQSPI_CLK,			// TBV2B: 	PB1
	gQSPI_BK1_IO0,	// TBV2B: 	PC9
	gQSPI_BK1_IO1,	// TBV2B: 	PC10
	gQSPI_BK1_IO2,	// TBV2B: 	PC8
	gQSPI_BK1_IO3,	// TBV2B: 	PA1
	gQSPI_BK1_NCS,	// TBV2B: 	PB6
	gQSPI_BK2_IO0,	// TBV2B: 	PA6
	gQSPI_BK2_IO1,	// TBV2B: 	PA7
	gQSPI_BK2_IO2,	// TBV2B: 	PC4
	gQSPI_BK2_IO3,	// TBV2B: 	PC5
	gQSPI_BK2_NCS,	// TBV2B: 	PC11
	gSDIO_DAT0,			// TBV2B: 	PB4
	gSDIO_DAT1,			// TBV2B: 	PA8
	gSDIO_DAT2,			// TBV2B: 	PA9
	gSDIO_DAT3,			// TBV2B: 	PB5
	gSDIO_CLK,			// TBV2B: 	PC12
	gSDIO_CMD,			// TBV2B: 	PD2
	gSWDIO,					// TBV2B: 	PA13
	gSWCLK,					// TBV2B: 	PA14
	gSWO,						// TBV2B: 	PB3
	gNRST,					// TBV2B: 	NRST
	gBOOT0,					// TBV2B: 	BOOT0
	gBOOT1_PDN,			// TBV2B: 	PB2		codec PDN   STM3210E_EVAL: PG11
	gOSC32_OUT,			// TBV2B: 	PC15
	gOSC32_IN,			// TBV2B: 	PC14
	gOSC_OUT,				// TBV2B: 	PH1
	gOSC_IN,				// TBV2B: 	PH0
	gBAT_STAT1,			// TBV2B: 	PD8
	gBAT_STAT2,			// TBV2B: 	PD9
	gBAT_PG_N,			// TBV2B: 	PD10
	gBAT_CE,				// TBV2B: 	PD11
	gBAT_TE_N,			// TBV2B: 	PD12	
  
} GPIO_ID;

	typedef struct	{	// GPIO_Def_t -- define GPIO port/pins/interrupt #, signal name, & st when pressed for a GPIO_ID
		GPIO_ID					id;			// GPIO_ID connected to this line
		char *					name;		// "PC7_" => { GPIOC, 7, EXTI15_10_IRQn, "PC7_", 0 }
	} GPIO_Signal;

// **************************************************************************************
// ********************************* 	Feb2020 Rev2B board
	
// define TBOOKREV2B for Feb2020 Rev2B board
// #define TBOOKREV2B	
#if defined( TBOOKREV2B )
	#define UID_BASE                     0x1FFF7A10U           /*STM32F412 !< Unique device ID register base address */
//	#include "I2C_STM32F4xx.h"			// STM32F412 on TBOOKREV2B
	#include "stm32f4xx.h"
	#include "GPIO_STM32F4xx.h"
	
	// select audio codec device
	#define AK4637
	// I2C slave address for ak4637	--  shifted <<1 by I2C, so differs from HAL 0x27 which is masked with 0xFFFE
	#define AUDIO_I2C_ADDR                     0x12
	
// #define USE_LCD
// define NO_PRINTF if user printf (retargetio.c) not included
// #define NO_PRINTF

// define USE_FILESYS if filesystem is included
#define USE_FILESYS

	#define KEY_PRESSED  0
// TBOOKREV2B  EXTI ints 0 Hom, 1 Pot, 3 Tab, 4 Plu, 5-9 Min/LHa/Sta/Cir, 10-15 RHa/Tre
static GPIO_Signal gpioSignals[] = {  	// GPIO signal definitions
	// GPIO_ID  signal  								eg   'PA4'   => GPIOA, pin 4, active high  
	//          "Pxdd_|cc" where:				e.g. 'PE12_' => GPIOE, pin 12, active low
	//             x  = A,B,C,D,E,F,G,H -- GPIO port
	//             dd = 0..15  -- GPIO pin #
	//             _  = means 'active low', i.e, 0 if pressed
	//						 cc = 0..15 -- alternate function code
	{ gHOME,	  		"PA0"				}, 	// IN:  SW9_HOUSE,  1 when pressed   configured by inputmanager.c
	{ gCIRCLE,			"PE9"				}, 	// IN:  SW8_CIRCLE, 1 when pressed		configured by inputmanager.c
	{ gPLUS,	  		"PA4"				}, 	// IN:  SW7_PLUS,   1 when pressed		configured by inputmanager.c	
	{ gMINUS,				"PA5"				}, 	// IN:  SW6_MINUS,  1 when pressed		configured by inputmanager.c	
	{ gTREE,	  		"PA15"			}, 	// IN:  SW5_TREE,   1 when pressed		configured by inputmanager.c
	{ gLHAND,				"PB7"				}, 	// IN:  SW4_LEFT,   1 when pressed		configured by inputmanager.c
	{ gRHAND,				"PB10"			}, 	// IN:  SW3_RIGHT,  1 when pressed		configured by inputmanager.c
	{ gPOT,	  			"PC1"				}, 	// IN:  SW2_POT,    1 when pressed		configured by inputmanager.c
	{ gSTAR,	  		"PE8"				}, 	// IN:  SW1_STAR,   1 when pressed		configured by inputmanager.c
	{ gTABLE,				"PE3"				}, 	// IN:  SW0_TABLE,  1 when pressed		configured by inputmanager.c
	{	gRED,					"PE1"				}, 	// OUT: 1 to turn Red LED ON		 	configured by ledmanager.c
	{ gGREEN, 			"PE0"				}, 	// OUT: 1 to turn Green LED ON		configured by ledmanager.c
	
	{ gADC_LI_ION, 	"PA2"				},	// IN:  analog rechargable battery voltage level 	(powermanager.c)
	{ gADC_PRIMARY,	"PA3"				},	// IN:  analog disposable battery voltage level	 	(powermanager.c)
	{ gPWR_FAIL_N, 	"PD0_"			},	// IN:  0 => power fail signal 										(powermanager.c)
	{ gADC_ENABLE, 	"PE15"			},	// OUT: 1 to enable battery voltage measurement 	(powermanager.c)
	{ gSC_ENABLE, 	"PD1"				},	// OUT: 1 to enable SuperCap			 								(powermanager.c)
	{ gPA_EN, 			"PD3"				},	// OUT: 1 to power speaker & hphone								(powermanager.c)
	{ gEN_5V, 			"PD4"				},	// OUT: 1 to supply 5V to codec		AP6714 EN				(powermanager.c)
	{ gNLEN01V8, 		"PD5"				},	// OUT: 1 to supply 1.8 to codec  TLV74118 EN			(powermanager.c)	
	{ g3V3_SW_EN, 	"PD6"				},	// OUT: 1 to power SDCard & eMMC
	{ gEMMC_RSTN, 	"PE10"			},	// OUT: 0 to reset eMMC?		MTFC2GMDEA eMMC RSTN	(powermanager.c)	
	
	{ gBAT_STAT1,		"PD8"				},  // IN:  charge_status[0]     			MCP73871 STAT1  (powermanager.c)
	{ gBAT_STAT2,		"PD9"				},  // IN:  charge_status[1]     			MCP73871 STAT2  (powermanager.c)
	{ gBAT_PG_N,		"PD10"			},  // IN:  0 => power is good   			configured as active High to match MCP73871 Table 5.1
	{ gBAT_CE,	  	"PD11"			},  // OUT: 1 to enable charging 			MCP73871 CE 	  (powermanager.c)
	{ gBAT_TE_N,		"PD12"			},  // OUT: 0 to enable safety timer 	MCP73871 TE_	  (powermanager.c)

	{ gBOOT1_PDN, 	"PB2_" 			},	// OUT: 0 to power_down codec 	 AK4637 PDN 			(powermanager.c)
	{ gI2C1_SCL, 		"PB8|4" 		},	// AK4637 I2C_SCL 								(RTE_Device.h I2C1 altFn=4)
	{ gI2C1_SDA, 		"PB9|4" 		},	// AK4637 I2C_SDA 								(RTE_Device.h I2C1 altFn=4)

	{ gI2S2_SD,			"PB15|5"		},	// AK4637 SDTI == MOSI 						(RTE_I2SDevice.h I2S0==SPI2 altFn=5)
	{ gI2S2ext_SD,	"PB14|6"		},	// AK4637 SDTO  									(RTE_I2SDevice.h I2S0==SPI2 altFn=6)
	{ gI2S2_WS,			"PB12|5"		},	// AK4637 FCK  == NSS == WS 			(RTE_I2SDevice.h I2S0==SPI2 altFn=5)
	{ gI2S2_CK,			"PB13|5"		},	// AK4637 BICK == CK == SCK 			(RTE_I2SDevice.h I2S0==SPI2 altFn=5) 
	{ gI2S2_MCK,		"PC6|5"			},	// AK4637 MCLK   									(RTE_I2SDevice.h I2S0==SPI2 altFn=5)
	{ gI2S3_MCK,		"PC7|6"			},	// AK4637 MCLK 	 									(RTE_I2SDevice.h I2S3==SPI2 altFn=6)

	{ gSPI4_NSS, 		"PE11|5"		}, 	// W25Q64JVSS  NOR flash U8 			(RTE_Device.h SPI4 altFn=5)
	{ gSPI4_SCK, 		"PE12|5"		}, 	// W25Q64JVSS  NOR flash U8 			(RTE_Device.h SPI4 altFn=5)
	{ gSPI4_MISO, 	"PE13|5"		}, 	// W25Q64JVSS  NOR flash U8 			(RTE_Device.h SPI4 altFn=5)
	{ gSPI4_MOSI, 	"PE14|5"		}, 	// W25Q64JVSS  NOR flash U8 			(RTE_Device.h SPI4 altFn=5)
	
	{ gUSB_DM, 			"PA11|10"		},	// std pinout assumed by USBD (altFn=10)
	{ gUSB_DP, 			"PA12|10"		},	// std pinout assumed by USBD (altFn=10)
	{ gUSB_ID, 			"PA10|10"		},	// std pinout assumed by USBD (altFn=10)
	
	{ gQSPI_CLK, 			"PB1|9"		},	// W25Q64JVSS & MT29F4G01 clock for NOR and NAND Flash (altFn=9)
	
	{ gQSPI_BK1_IO0, 	"PC9|9"		},	// W25Q64JVSS  NOR flash  U5 (altFn=9)
	{ gQSPI_BK1_IO1, 	"PC10|9"	},	// W25Q64JVSS  NOR flash  U5 (altFn=9)
	{ gQSPI_BK1_IO2, 	"PC8|9"		},	// W25Q64JVSS  NOR flash  U5 (altFn=9)
	{ gQSPI_BK1_IO3, 	"PA1|9"		},	// W25Q64JVSS  NOR flash  U5 (altFn=9)
	{ gQSPI_BK1_NCS, 	"PB6|10"	},	// W25Q64JVSS  NOR flash  U5 chip sel (altFn=10)
	
	{ gQSPI_BK2_IO0, 	"PA6|10"	},	// MT29F4G01 NAND flash  U4   (altFn=10)
	{ gQSPI_BK2_IO1, 	"PA7|10"	},	// MT29F4G01 NAND flash  U4   (altFn=10) 
	{ gQSPI_BK2_IO2, 	"PC4|10"	},	// MT29F4G01 NAND flash  U4   (altFn=10) 
	{ gQSPI_BK2_IO3, 	"PC5|10"	},	// MT29F4G01 NAND flash  U4   (altFn=10)
	{ gQSPI_BK2_NCS, 	"PC11|9"	},	// MT29F4G01 NAND flash  U4  chip sel (altFn=9)
		
	{ gSDIO_DAT0, 		"PB4|12"	},  // SDcard & eMMC SDIO D0  (RTE_Device.h MCI0)  (altFn=12)
	{ gSDIO_DAT1, 		"PA8|12"	},  // SDcard & eMMC SDIO D1  (RTE_Device.h MCI0)  (altFn=12)
	{ gSDIO_DAT2, 		"PA9|12"	},  // SDcard & eMMC SDIO D2  (RTE_Device.h MCI0)  (altFn=12)
	{ gSDIO_DAT3, 		"PB5|12"	},  // SDcard & eMMC SDIO D3  (RTE_Device.h MCI0)  (altFn=12)
	{ gSDIO_CLK, 			"PC12|12"	},  // SDcard & eMMC SDIO CLK (RTE_Device.h MCI0)  (altFn=12)
	{ gSDIO_CMD, 			"PD2|12"	},	// SDcard & eMMC SDIO CMD (RTE_Device.h MCI0) 	  (altFn=12)

	{ gSWDIO, 		"PA13"		},	// bootup function 
	{ gSWCLK, 		"PA14"		},	// bootup function 
	{ gSWO, 			"PB3"			},	// bootup function

	{ gOSC32_OUT,	"PC15"		},	// bootup function
	{ gOSC32_IN,	"PC14"		},	// bootup function
	{ gOSC_OUT,		"PH1"			},	// bootup function
	{ gOSC_IN,		"PH0"			},	// bootup function
	
	{ gINVALID, 	""  			}	// end of list
};
#endif



// **************************************************************************************
// ********************************* 	Dec2019  STM3210E_EVAL board 

// define STM3210E_EVAL for testing on STM3210E_EVAL board 
//#   define STM3210E_EVAL
#if defined( STM3210E_EVAL )
	#define UID_BASE                      0x1FFFF7E8           /*STM32F103 !< Unique device ID register base address */
	#include "stm32f10x.h"
	#include "GPIO_STM32F10x.h"

	#include "I2C_STM32F10x.h"			// STM32F103 on STM3210E_EVAL
	#define AK4343
	// I2C slave address for ak4343	--  shifted <<1 by I2C, so differs from HAL 0x27 which is masked with 0xFFFE
	#define AUDIO_I2C_ADDR                     0x13

	//   STM3210E_EVAL board I2C pin assignments -- AK4343
	#define AUDIO_PDN_PIN                       11
	#define AUDIO_PDN_GPIO                      GPIOG
	// PDN = PG11
	// I2C_SCL = PB7
	// I2C_SDA = PB6

	#define USE_LCD
// define NO_PRINTF if user printf (retargetio.c) not included
// #define NO_PRINTF

// define USE_FILESYS if filesystem is included
#define USE_FILESYS

	#define KEY_PRESSED  1
// STM3210E_EVAL  EXTI ints 0 Wk, 3 JDn, 5-9 JCe/Key, 10-15 Tam&JRi/JLe/JUp
static GPIO_Signal gpioSignals[] = {  // GPIO signal definitions
	{ gHOME,		"PG8_" 		}, //   STM3210E 	Key/User button,  0 when pressed
	{ gCIRCLE,	""				}, //   STM3210E 	undefined  
	{	gPLUS,		"PA0" 		}, //   STM3210E 	Wakeup button, 		1 when pressed
	{ gMINUS,		""				}, //   STM3210E 	undefined 
  { gTREE, 		"PG15_" 	}, //   STM3210E 	JOYSTICK_UP,			0
  { gLHAND, 	"PG14_" 	}, //   STM3210E 	JOYSTICK_LEFT,		0
  { gRHAND, 	"PG13_" 	}, //   STM3210E 	JOYSTICK_RIGHT,		0
  { gPOT, 		"PG7_" 		}, //   STM3210E 	JOYSTICK_CENTER,	0
	{ gSTAR,		"PC13_" 	}, //   STM3210E 	Tamper button,		0
  { gTABLE, 	"PD3_" 		}, //   STM3210E 	JOYSTICK_DOWN,		0

	{	 gRED,		"PF8_"		}, //   STM3210E 	LED_RED
	{  gGREEN, 	"PF6_"		}, //   STM3210E 	LED_GREEN
	{  gORANGE, "PF7_"		}, //   STM3210E 	LED_ORANGE
	{  gBLUE, 	"PF9_"		}, //   STM3210E 	LED_BLUE
	
	{ gBOOT1_PDN, "PG11_" },	// codec PDN = PG11
	{ gI2C1_SCL, 	"PB7" 	},	// I2C_SCL = PB7
	{ gI2C1_SDA, 	"PB6" 	},	// I2C_SDA = PB6


	{ gINVALID, ""  }	// end of list
};
#endif 


// **************************************************************************************
// ********************************* 	Sep2018 Rev2A board
	
// define TBOOKREV2A for Sep2018 Rev2B board
// #define TBOOKREV2A	

/*
#if defined( TBOOKREV2A )
	// TalkingBook V2 Rev2A uses STM32F411RC LQFP64 (Low-profile Quad Flat surface mount Package with 64 leads)
	// rev2:  kHOME  PA0  EXTI0 		kCIRCLE PA1  EXTI1 		kPLUS PA2 EXTI2 	kMINUS PA3  EXTI3 		kTREE  PA15 EXTI15_10
	//        kLHAND PB6  EXTI9_5 		kRHAND  PB8  EXTI19_5 	kPOT  PB9 EXTI9_5 	kSTAR  PB10 EXTI15_10 	kTABLE PB13 EXTI15_10
	//		  RED	 PC13				GREEN	PB14
// Sep-2018 REV2-proto board -- KEYBOARD CABLE REVERSED, then jumpered to swap RED & HOME, and GREEN & CIRCLE so:
	// rev2:  kHOME  PA0  EXTI0 			kCIRCLE PA1  EXTI1 			kPLUS PB13 EXTI2	 		kMINUS PB10 EXTI15_10		kTREE  PB9  EXTI15_10
	//        kLHAND PB8  EXTI9_5 		kRHAND  PB6  EXTI19_5 	kPOT  PA15 EXTI15_10 	kSTAR  PA3  EXTI3	 			kTABLE PA2  EXTI12
	//		  	GREEN	 PC13							RED	PB14
// Sep-22-2018  apparently, KP13_GREEN & KP12_RED, so swapped...
	#define KP12_RED_pin				GPIO_PIN_14		// PB14 -- KP12_RED
	#define KP12_RED_port				GPIOB
	#define KP13_GREEN_pin			GPIO_PIN_13		// PC13 -- KP13_GREEN
	#define KP13_GREEN_port			GPIOC
	#define AMP_EN_pin					GPIO_PIN_2		// PC2 -- AMP_EN digital output			1 turns on the audio amplifier power
	#define AMP_EN_port					GPIOC

// ********** power-related GPIO assignments
//   input interrupt
#define NOPWR_pin							GPIO_PIN_4		// PC4 -- NOPWR  digital input EXTI4	0 when all power sources but SCAP have failed 
#define NOPWR_port						GPIOC
#define NOPWR_EXTI						EXTI4_IRQn

//   digital inputs
#define USBPWR_pin						GPIO_PIN_5		// PC5 -- USBPWR digital input 			0 when TBook has USB power
#define USBPWR_port						GPIOC
#define LICHG_pin							GPIO_PIN_7		// PC7 -- LICHG digital input 			0 when Li-Ion battery is charging
#define LICHG_port						GPIOC
// USB patch  30Sep2019  #define T_FAULT_pin						GPIO_PIN_12		// PA12 - T_FAULT digital input 		0 when Li-Ion battery is too hot
// USB patch  30Sep2019  #define T_FAULT_port					GPIOA
#define T_FAULT_pin						GPIO_PIN_12		// PC12 - T_FAULT digital input 		0 when Li-Ion battery is too hot
#define T_FAULT_port					GPIOC

//   analog inputs
#define LI_ION_V_pin					GPIO_PIN_0		// PC0 -- LI_ION_V analog input 		0-3.3V when ANALOG_EN is active
#define LI_ION_V_port					GPIOC
#define LI_ION_V_channel			ADC_CHANNEL_10
#define EX_BATV_pin						GPIO_PIN_1
#define EX_BATV_port					GPIOC			// PC1 -- EX_BATV analog input 			0-3.0V when ANALOG_EN is active
#define EX_BATV_channel				ADC_CHANNEL_11

//   digital outputs
#define ANALOG_EN_pin					GPIO_PIN_3		// PC3 -- ANALOG_EN digital output		1 enables EX_BATV & LI_ION_V
#define ANALOG_EN_port				GPIOC
// USB patch  30Sep2019  #define S_CAP_EN_pin					GPIO_PIN_11		// PA11 - S_CAP_EN digital output 		? to charge the super-capacitor 
// USB patch  30Sep2019  #define S_CAP_EN_port					GPIOA
#define S_CAP_EN_pin					GPIO_PIN_11		// PC11 - S_CAP_EN digital output 		? to charge the super-capacitor 
#define S_CAP_EN_port					GPIOC

// ********** VS8063 audio chip
#define VS8063_XCS_pin 				GPIO_PIN_4		// PA4
#define VS8063_XCS_port 			GPIOA
#define VS8063_XDCS_pin 			GPIO_PIN_0		// PB0
#define VS8063_XDCS_port 			GPIOB
#define VS8063_DREQ_pin 			GPIO_PIN_1		// PB1
#define VS8063_DREQ_port 			GPIOB
#define VS8063_XRESET_pin 		GPIO_PIN_2		// PB2
#define VS8063_XRESET_port 		GPIOB
#endif
*/



#endif /* __MAIN_H */

