//*************************
//     gpio_signals.h
//

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef GPIO_SIGNALS_H
#define GPIO_SIGNALS_H

// GPIO Signal IDs
// 	  TalkingBook keypad has 10 keys, each connected to a line of the keypad cable-- these have to be 0..9 to match enum KEY
//    10 is reserved (for an invalid KEY)
//    other IDs are assigned for GPIO pins used in different implementations
typedef enum {			// GPIO_ID -- gpio signal IDs  
	gORANGE, gBLUE,		// LEDs on STM3210E_EVAL & STM32F412_DISCOG
    gRED,	gGREEN,  	// LEDs on TBook keypad & STM3210E_EVAL & STM32F412_DISCOG
	gINVALID,			// marker for end of definition array
	gHOME, gCIRCLE, gPLUS, gMINUS, gTREE, gLHAND, gRHAND, gPOT, gSTAR, gTABLE,
// defs for all the other TBV2B gpio assignments
	gADC_LI_ION,		// TBV2B: 	PA2
	gADC_PRIMARY,		// TBV2B: 	PA3
	gPWR_FAIL_N,		// TBV2B: 	PD0
	gSC_ENABLE,			// TBV2B: 	PD1
	gPA_EN,					// TBV2B: 	PD3
	gEN_5V,					// TBV2B: 	PD4
	gEN1V8,					// TBV2B: 	PD5
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
	gMCO2,					// TBV2B:   also PC9
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

#endif

