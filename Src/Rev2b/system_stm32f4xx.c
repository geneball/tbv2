/**
  ******************************************************************************
  * @file    system_stm32f4xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M4 Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from 
  *   user application:
  *      - SystemInit(): This function is called at startup just after reset and 
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32f4xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick 
  *                                  timer or configure other parameters.
  *                                     
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */


#include "stm32f4xx.h"

// JEB Mar2020 -- HSE is 8000000 ( 8MHz crystal on TBookRev2b )
#if !defined  (HSE_VALUE) 
  #define HSE_VALUE    ((uint32_t)8000000) /*!< Default value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)16000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use external SRAM or SDRAM as data memory  */
#if defined(STM32F405xx) || defined(STM32F415xx) || defined(STM32F407xx) || defined(STM32F417xx)\
 || defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)\
 || defined(STM32F469xx) || defined(STM32F479xx) || defined(STM32F412Zx) || defined(STM32F412Vx)
/* #define DATA_IN_ExtSRAM */
#endif /* STM32F40xxx || STM32F41xxx || STM32F42xxx || STM32F43xxx || STM32F469xx || STM32F479xx ||\
          STM32F412Zx || STM32F412Vx */
 
#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx)\
 || defined(STM32F446xx) || defined(STM32F469xx) || defined(STM32F479xx)
/* #define DATA_IN_ExtSDRAM */
#endif /* STM32F427xx || STM32F437xx || STM32F429xx || STM32F439xx || STM32F446xx || STM32F469xx ||\
          STM32F479xx */

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x00 /*!< Vector Table base offset field. 
                                   This value must be a multiple of 0x200. */
/******************************************************************************/


  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
uint32_t SystemCoreClock = 16000000;		// defaults to HSI clock = 16MHz
const uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};		// SystemCoreClock = AHB >> AHBPrescTable[ RCC->HPRE ]
const uint8_t APBPrescTable[8]  = {0, 0, 0, 0, 1, 2, 3, 4};													
// APB2 = AHB >> APBPrescTable[ RCC->CFGR.PPRE2 ]
// APB1 = AHB >> APBPrescTable[ RCC->CFGR.PPRE1 ]


#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
  static void SystemInit_ExtMemCtl(void); 
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */


// SystemInit -- called by reset handler before main()
void SystemInit(void)			// Setup MPU:  FPU setting, vector table loc & External mem config
{
  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  #endif
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;
	
	// turn on HSE since were going to use it as PLL src
	RCC->CR |= RCC_CR_HSEON;

	//  RCC_PLLCFGR configuration register:
	// 		PLLR  	2..7  		( << 28)
	// 		PLLQ  	2..15 		( << 24)
	// 		PLLSRC 	0..1   		0: HSI@16MHz  1: HSE@8MHz	( << 22 )
	//    PLLP 		0..3 			0: 2  1: 4  2: 6   3: 8   ( << 16 )
	// 		PLLN  	50..432 	( << 6 )
	// 		PLL_M		2..63			( << 0 ) 
	//  VCO_clk 			= SRC * PLLN / PLLM 		
  //  Sys_clk				= VCO_clk / PLLP
	//  USB/SDIO clk 	= VCO_clk / PLLQ	(must be 48MHz)
	// 	I2S_clk 			= VCO_clk / PLLR
	
  /* Reset PLLCFGR register
  // RCC->PLLCFGR = 0x24003010;		// R=2 Q=4 SRC=0 P=0 N=192 M=16  //default
	// VCO_clk 	= ~16MHz * 192/16 = ~192MHz
	// Sys_clk 	= ~96MHz
	// USB/SDIO = ~48MHz
	// I2S_clk  = ~96MHz
	*/

	/* configure PLL to  SRC = 1 (8MHz HSE) N=96 M=4 P=2 Q=4 R=2 
	//  HSE 			= 8000000
	//  VCO_in    = HSE / M = 2MHz  (per RM0402 6.3.2 pg124)
	//  VCO_out   = VCO_in * N = 2MHz * 96 = 192MHz		(HSE based)
	//  Sys_clk 	= VCO_out / P = 96MHz
	//  USB/SDIO  = VCO_out / Q = 48MHz
	// 	I2S_clk 	= VCO_out / R = 96MHz
	*/
	uint32_t cfg = 0;
//	( 2 << 28 ) | ( 4 << 24 ) | ( 1 << 22 ) | ( 0 << 16 ) | ( 0x60 << 6 ) | 4;
	cfg |=  	RCC_PLLCFGR_PLLSRC_HSE;							// input to PLL is HSE
	cfg |=  ( 96 << RCC_PLLCFGR_PLLN_Pos );				// N = 96
	cfg |=  (  4 << RCC_PLLCFGR_PLLM_Pos );				// M = 4
	cfg |=  (  0 << RCC_PLLCFGR_PLLP_Pos );				// PLLP=0 => P = 2
	cfg |=  (  4 << RCC_PLLCFGR_PLLQ_Pos );				// Q = 4
	cfg |=  (  2 << RCC_PLLCFGR_PLLR_Pos );				// R = 2
	RCC->PLLCFGR = cfg; 		// R=2 Q=4 SRC=1 P=0 N=96 M=4
	// PLLCFGR:		 _RRR QQQQ _S__ __PP _NNN NNNN NNMM MMMM
	// 0x24401804  0010 0100 0100 0000 0001 1000 0000 0100 R=2 Q=4 S=1 P=0 N=0x60 M=4


  RCC->CR &= (uint32_t)0xFFFBFFFF; 		// /* Reset HSEBYP bit */

	RCC->CR |= RCC_CR_PLLON;						// JEB Mar2020:  turn on PLL  (based on HSE)

	/* Configure RCC->CFGR to set SYSCLK, AHB, APB2, & APB1 clock speeds
	//  SYSCLK = PLL 								(CFGR.SW  System clock switch)
	//  AHB  = SYSCLK / HPRE  			(CFGR.HPRE  AHB prescaler)
	//  APB2 = AHB / PPRE2					(CFGR.PPRE2 APB2 prescaler)
	//  APB1 = AHB / PPRE1					(CFGR.PPRE1 APB1 prescaler)
  */	
  cfg =  0;
  cfg |=	RCC_CFGR_SW_PLL;        // PLL selected as SYSCLK									(System clock Switch)  
//  cfg |=  RCC_CFGR_HPRE_DIV1;     // SYSCLK not divided 	=> AHB = PLL			(AHB prescaler)		AHB  = 96MHz
  cfg |=  RCC_CFGR_HPRE_DIV4;     // SYSCLK divided by 4 	=> AHB = PLL / 4	(AHB prescaler)  	AHB  = 24MHz
  cfg |=  RCC_CFGR_PPRE2_DIV1;		// HCLK not divided    	=> APB2 = AHB			(APB2 prescaler)	APB2 = 24MHz
  cfg |=  RCC_CFGR_PPRE1_DIV2;    // HCLK divided by 2 		=> APB1 = AHB	/2	(APB1 prescaler)	APB1 = 12MHz
	SystemCoreClock = 96000000;			// = 96MHz

	/* DEBUG -- send SYSCLK/4 to PC9
	//     PC9 shoud be configured as ModeAF, AF=0
	*/
	cfg |=  0;																				// MCO = SYSCLK  	(MCO2 configuration)	
	cfg |= (RCC_CFGR_MCO2PRE_1 | RCC_CFGR_MCO2PRE_2); // PC9 = MCO / 4	(MCO2 prescaler)			
	// DEBUG  */
	
	RCC->CFGR = cfg;			// Src=PLL HPRE=4 PPRE2=0 PPRE1=2  => SYSCLK=96MHz, AHB=24MHz, APB2=24MHz, APB1=12MHz,  PC9=24MHz
//	RCC->CFGR = cfg;			// Src=PLL HPRE=0 PPRE2=0 PPRE1=2  => SYSCLK=96MHz, AHB=96MHz, APB2=96MHz, APB1=24MHz,  PC9=24MHz

	/* Disable all interrupts */
  RCC->CIR = 0x00000000;

#if defined (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM)
  SystemInit_ExtMemCtl(); 
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
   * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *           
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.         
  *     
  * @note   - The system frequency computed by this function is not the real 
  *           frequency in the chip. It is calculated based on the predefined 
  *           constant and the selected clock source:
  *             
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(*)
  *                                              
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(**)
  *                          
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(**) 
  *             or HSI_VALUE(*) multiplied/divided by the PLL factors.
  *         
  *         (*) HSI_VALUE is a constant defined in stm32f4xx_hal_conf.h file (default value
  *             16 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.   
  *    
  *         (**) HSE_VALUE is a constant defined in stm32f4xx_hal_conf.h file (its value
  *              depends on the application requirements), user has to ensure that HSE_VALUE
  *              is same as the real frequency of the crystal used. Otherwise, this function
  *              may have wrong result.
  *                
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *     
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t tmp = 0, pllvco = 0, pllp = 2, pllsource = 0, pllm = 2;
  
  /* Get SYSCLK source -------------------------------------------------------*/
  tmp = RCC->CFGR & RCC_CFGR_SWS;

  switch (tmp)
  {
    case 0x00:  /* HSI used as system clock source */
      SystemCoreClock = HSI_VALUE;
      break;
    case 0x04:  /* HSE used as system clock source */
      SystemCoreClock = HSE_VALUE;
      break;
    case 0x08:  /* PLL used as system clock source */

      /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N
         SYSCLK = PLL_VCO / PLL_P
         */    
      pllsource = (RCC->PLLCFGR & RCC_PLLCFGR_PLLSRC) >> 22;
      pllm = RCC->PLLCFGR & RCC_PLLCFGR_PLLM;
      
      if (pllsource != 0)
      {
        /* HSE used as PLL clock source */
        pllvco = (HSE_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }
      else
      {
        /* HSI used as PLL clock source */
        pllvco = (HSI_VALUE / pllm) * ((RCC->PLLCFGR & RCC_PLLCFGR_PLLN) >> 6);
      }

      pllp = (((RCC->PLLCFGR & RCC_PLLCFGR_PLLP) >>16) + 1 ) *2;
      SystemCoreClock = pllvco/pllp;
      break;
    default:
      SystemCoreClock = HSI_VALUE;
      break;
  }
  /* Compute HCLK frequency --------------------------------------------------*/
  /* Get HCLK prescaler */
  tmp = AHBPrescTable[((RCC->CFGR & RCC_CFGR_HPRE) >> 4)];
  /* HCLK frequency */
  SystemCoreClock >>= tmp;
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
