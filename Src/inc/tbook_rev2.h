// **************************************************************************************
//  TBookRev2.h
//	Feb2020 Rev2B board
// ********************************* 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef TBOOKREV2B_H
#define TBOOKREV2B_H
	
#define UID_BASE                     0x1FFF7A10U           /*STM32F412 !< Unique device ID register base address */
//	#include "I2C_STM32F4xx.h"			
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

#include "gpio_signals.h"		// GPIO_IDs

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
	{ gEN1V8, 			"PD5"				},	// OUT: 1 to supply 1.8 to codec  TLV74118 EN			(powermanager.c)	
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

	{ gMCO2,					"PC9|0"		},  // DEBUG: MCO2 for external SystemClock/4
	
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

