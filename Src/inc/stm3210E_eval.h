
// **************************************************************************************
// *****	Dec2019  STM3210E_EVAL board 
//  stm3210e_eval.h
//	pins & switches for testing on STM3210E_EVAL board 
// ********************************* 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM3210E_EVAL_H
#define STM3210E_EVAL_H

#define STM3210E_EVAL

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

#include "gpio_signals.h"		// GPIO_IDs

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
