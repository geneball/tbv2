/**
  ******************************************************************************
  * @file    ak4343.h
  * @author  MCD Application Team
  * @version V2.0.0
  * @date    11-April-2016
  * @brief   This file contains all the functions prototypes for the ak4343.c driver.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AK4xxx_H
#define __AK4xxx_H

#include "tbook.h"

#include "audio.h"

//**************************  Codec User defines ******************************/
/* Codec output DEVICE */
#define OUTPUT_DEVICE_OFF							0x00
#define OUTPUT_DEVICE_SPEAKER         0x01
#define OUTPUT_DEVICE_HEADPHONE       0x02
#define OUTPUT_DEVICE_BOTH            (OUTPUT_DEVICE_SPEAKER | OUTPUT_DEVICE_HEADPHONE)

/* Volume Levels values */
#define DEFAULT_VOLMIN                0x00
#define DEFAULT_VOLMAX                0xFF
#define DEFAULT_VOLSTEP               0x04

#define AUDIO_PAUSE                   0
#define AUDIO_RESUME                  1

/* Codec POWER DOWN modes */
#define CODEC_PDWN_HW                 1
#define CODEC_PDWN_SW                 2

/* MUTE commands */
#define AUDIO_MUTE_ON                 1
#define AUDIO_MUTE_OFF                0

#if defined( AK4343 )			/* AK4343 register map */
	/* AK4343 register map */
	#define   AK_Power_Management_1			0x00
	#define   AK_Power_Management_2			0x01
	#define   AK_Signal_Select_1				0x02
	#define   AK_Signal_Select_2				0x03
	#define   AK_Mode_Control_1					0x04
	#define   AK_Mode_Control_2					0x05
	#define   AK_Timer_Select						0x06
	#define   AK_ALC_Mode_Control_1			0x07
	#define   AK_ALC_Mode_Control_2			0x08
	#define   AK_Lch_Input_Volume_Control	0x09
	#define   AK_Lch_Digital_Volume_Control	0x0A
	#define   AK_ALC_Mode_Control_3			0x0B
	#define   AK_Rch_Input_Volume_Control	0x0C
	#define   AK_Rch_Digital_Volume_Control	0x0D
	#define   AK_Mode_Control_3					0x0E
	#define   AK_Mode_Control_4					0x0F
	#define   AK_Power_Management_3			0x10
	#define   AK_Digital_Filter_Select	0x11
	#define   AK_FIL3_Coefficient_0			0x12
	#define   AK_FIL3_Coefficient_1			0x13
	#define   AK_FIL3_Coefficient_2			0x14
	#define   AK_FIL3_Coefficient_3			0x15
	#define   AK_EQ_Coefficient_0				0x16
	#define   AK_EQ_Coefficient_1				0x17
	#define   AK_EQ_Coefficient_2				0x18
	#define   AK_EQ_Coefficient_3				0x19
	#define   AK_EQ_Coefficient_4				0x1A
	#define   AK_EQ_Coefficient_5				0x1B
	#define   AK_FIL1_Coefficient_0			0x1C
	#define   AK_FIL1_Coefficient_1			0x1D
	#define   AK_FIL1_Coefficient_2			0x1E
	#define   AK_FIL1_Coefficient_3			0x1F
	#define   AK_Power_Management_4			0x20
	#define   AK_Mode_Control_5					0x21
	#define   AK_Lineout_Mixing_Select	0x22
	#define   AK_HP_Mixing_Select				0x23
	#define   AK_SPK_Mixing_Select			0x24
	#define		AK_LAST_RST_REG		0x11

	// values for AK_Power_Management_1   (ak4343: R00) 
	#define AK_Power_Management_1_RST  0x0
	#define AK_PM1_D0			0x01		/* 0 -- default 0 */
	#define AK_PM1_D1			0x02		/* 0 -- default 0 */
	#define AK_PM1_DAC		0x04		/* 1 to power up DAC -- default 0 */
	#define AK_PM1_LO			0x08		/* 1 to power up Line Out -- default 0 */
	#define AK_PM1_SPK		0x10		/* 1 to power up Speaker Amp -- default 0 */
	#define AK_PM1_MIN		0x20		/* 1 to power up MIN Input -- default 0 */
	#define AK_PM1_VCM		0x40		/* must be 1 if any are -- default 0  */
	#define AK_PM1_D7 		0x80		/* 0 -- default 0 */

	// values for AK_Power_Management_2  (ak4343: R01)
	#define AK_Power_Management_2_RST  0x0
	#define AK_PM2_PLL		0x01		/* 1 for PLL Mode -- default 0 */
	#define AK_PM2_MCK0		0x02		/* 1: o/p freq select by PS1-0 bits -- default 0  */
	#define AK_PM2_D2			0x04		/* 0 -- default 0 */
	#define AK_PM2_M_S		0x08		/* 0: Slave mode, 1:Master mode  -- default 0 */
	#define AK_PM2_HPR 		0x10		/* 1 to power RCh Headphone Amp -- default 0 */
	#define AK_PM2_HPL 		0x20		/* 1 to power LCh Headphone Amp -- default 0 */
	#define AK_PM2_MTN 		0x40		/* 0 to Mute Headphone Amp  -- default 0 */
	#define AK_PM2_D7 		0x80		/* 0 -- default 0 */

	// values for AK_Signal_Select_1  (ak4343: R02)  
	#define AK_Signal_Select_1_RST  0x1
	#define AK_SS1_MGAIN1_0	0x01	/* Input Gain Control 0 -- default 1 */
	#define AK_SS1_D1			0x02		/* 0 -- default 0 */
	#define AK_SS1_D2			0x04		/* 0 -- default 0 */
	#define AK_SS1_D3			0x08		/* 0 -- default 0 */
	#define AK_SS1_DACL 	0x10		/* 1 & PMLO=1: DAC out => LOUT/ROUT -- default 0 */
	#define AK_SS1_DACS 	0x20		/* 1: DAC out => Speaker Amp -- default 0 */
	#define AK_SS1_MINS 	0x40		/* 1: mono signal to speaker amp -- default 0 */
	#define AK_SS1_SPPSN	0x80		/* 0: speaker amp power save mode -- default 0 */

	// values for AK_Signal_Select_2  (ak4343: R03)
	#define AK_Signal_Select_2_RST  0x0
	#define AK_SS2_D0			0x01		/* 0 -- default 0 */
	#define AK_SS2_D1			0x02		/* 0 -- default 0 */
	#define AK_SS2_MINL		0x04		/* 1 & PMLO=1: MIN => LOUT/ROUT -- default 0 */
	#define AK_SS2_SPKG0	0x08		/* Speaker Amp Out Gain- bit 0 -- default 0 */
	#define AK_SS2_SPKG1 	0x10		/* Speaker Amp Out Gain- bit 1 -- default 0 */
	#define AK_SS2_MGAIN1 0x20		/* Input Gain Control 1 -- default 0 */
	#define AK_SS2_LOPS 	0x40		/* Stereo Line Output Power-Save Mode -- default 0 */
	#define AK_SS2_LOVL 	0x80		/* Stereo Line Output / Receiver Output Gain Select -- default 0 */

	// values for AK_Mode_Control_1  (ak4343: R04)
	#define AK_Mode_Control_1_RST  0x2
	#define AK_MC1_DIF0		0x01		/* Audio Interface Format 0 -- default 0 */
	#define AK_MC1_DIF1		0x02		/* Audio Interface Format 1 -- default 1 */
	#define AK_MC1_D2			0x04		/* 0 -- default 0 */
	#define AK_MC1_BCKO		0x08		/* BICK Output Freq Select at Master Mode -- default 0 */
	#define AK_MC1_PLL0 	0x10		/* PLL Reference Clock Select 0 -- default 0 */
	#define AK_MC1_PLL1 	0x20		/* PLL Reference Clock Select 1 -- default 0 */
	#define AK_MC1_PLL2 	0x40		/* PLL Reference Clock Select 2 -- default 0 */
	#define AK_MC1_PLL3 	0x80		/* PLL Reference Clock Select 3 -- default 0 */

	// values for AK_Mode_Control_2		(ak4343: R05)
	#define AK_Mode_Control_2_RST  0x0  
	#define AK_MC2_FS0		0x01		/* Sampling Freqency Select 0 -- default 0 */
	#define AK_MC2_FS1		0x02		/* Sampling Freqency Select 1 -- default 0 */
	#define AK_MC2_FS2		0x04		/* Sampling Freqency Select 2 -- default 0 */
	#define AK_MC2_BCKP		0x08		/* BICK Polarity at DSP Mode -- default 0 */
	#define AK_MC2_MSBS 	0x10		/* LRCK Polarity at DSP Mode -- default 0 */
	#define AK_MC2_FS3 		0x20		/* Sampling Freqency Select 2 -- default 0 */
	#define AK_MC2_PS0 		0x40		/* MCKO Output Freq Select 0 -- default 0 */
	#define AK_MC2_PS1 		0x80		/* MCKO Output Freq Select 1 -- default 0 */

	// values for AK_Timer_Select  (ak4343: R06)
	#define AK_Timer_Select_RST  0x0
	#define AK_TS_RFST0		0x01		/* ALC First Recovery Speed 0 -- default 0 */
	#define AK_TS_RFST1		0x02		/* ALC First Recovery Speed 1 -- default 0 */
	#define AK_TS_WTM0		0x04		/* ALC Recovery Waiting Period 0 -- default 0 */
	#define AK_TS_WTM1		0x08		/* ALC Recovery Waiting Period 1 -- default 0 */
	#define AK_TS_ZTM0 		0x10		/* ALC Limiter/Recover Op Zero Crossing Timeout Period 0 -- default 0 */
	#define AK_TS_ZTM1 		0x20		/* ALC Limiter/Recover Op Zero Crossing Timeout Period 1 -- default 0 */
	#define AK_TS_WTM2 		0x40		/* ALC Recovery Waiting Period 2 -- default 0 */
	#define AK_TS_DVTM 		0x80		/* Digital Volume Transition Time Setting -- default 0 */

	// values for AK_ALC_Mode_Control_1  (ak4343: R07)
	#define AK_ALC_Mode_Control_1_RST  0x0
	#define AK_AMC1_LMTH0	0x01		/* ALC Limiter Detection Level / Recovery Counter Reset Level 0  -- default 0 */
	#define AK_AMC1_RGAIN0 0x02		/* ALC Recovery GAIN Step 0 -- default 0 */
	#define AK_AMC1_LMAT0	0x04		/* ALC Limiter ATT Step 0 -- default 0 */
	#define AK_AMC1_LMAT1	0x08		/* ALC Limiter ATT Step 1 -- default 0 */
	#define AK_AMC1_ZELMN	0x10		/* Zero Crossing Detection Enable at ALC Limiter Operation -- default 0 */
	#define AK_AMC1_ALC 	0x20		/* ALC Enable -- default 0 */
	#define AK_AMC1_D6 		0x40		/* 0 -- default 0 */
	#define AK_AMC1_D7 		0x80		/* 0 -- default 0 */

	// values for AK_ALC_Mode_Control_2  (ak4343: R08)
	#define AK_ALC_Mode_Control_2_RST  0xE1
	#define AK_AMC2_REF0	0x01		/* Reference Value at ALC Recovery Operation 0 -- default 1 */
	#define AK_AMC2_REF1	0x02		/* Reference Value at ALC Recovery Operation 1 -- default 0 */
	#define AK_AMC2_REF2	0x04		/* Reference Value at ALC Recovery Operation 2 -- default 0 */
	#define AK_AMC2_REF3	0x08		/* Reference Value at ALC Recovery Operation 3 -- default 0 */
	#define AK_AMC2_REF4 	0x10		/* Reference Value at ALC Recovery Operation 4 -- default 0 */
	#define AK_AMC2_REF5 	0x20		/* Reference Value at ALC Recovery Operation 5 -- default 1 */
	#define AK_AMC2_REF6 	0x40		/* Reference Value at ALC Recovery Operation 6 -- default 1 */
	#define AK_AMC2_REF7 	0x80		/* Reference Value at ALC Recovery Operation 7 -- default 1 */

	// values for AK_Lch_Input_Volume_Control  (ak4343: R09)
	#define AK_Lch_Input_Volume_Control_RST  0xE1
	#define AK_LIVC_AVL0	0x01		/* ALC Block Digital Volume L0 -- default 1 */
	#define AK_LIVC_AVL1	0x02		/* ALC Block Digital Volume L1 -- default 0 */
	#define AK_LIVC_AVL2	0x04		/* ALC Block Digital Volume L2 -- default 0 */
	#define AK_LIVC_AVL3	0x08		/* ALC Block Digital Volume L3 -- default 0 */
	#define AK_LIVC_AVL4 	0x10		/* ALC Block Digital Volume L4 -- default 0 */
	#define AK_LIVC_AVL5 	0x20		/* ALC Block Digital Volume L5 -- default 1 */
	#define AK_LIVC_AVL6 	0x40		/* ALC Block Digital Volume L6 -- default 1 */
	#define AK_LIVC_AVL7 	0x80		/* ALC Block Digital Volume L7 -- default 1 */

	// values for AK_Lch_Digital_Volume_Control  (ak4343: R0a)
	#define AK_Lch_Digital_Volume_Control_RST  0x18
	#define AK_LDVC_DVL0	0x01		/* Output Digital Volume L0 -- default 0 */
	#define AK_LDVC_DVL1	0x02		/* Output Digital Volume L1 -- default 0 */
	#define AK_LDVC_DVL2	0x04		/* Output Digital Volume L2 -- default 0 */
	#define AK_LDVC_DVL3	0x08		/* Output Digital Volume L3 -- default 1 */
	#define AK_LDVC_DVL4 	0x10		/* Output Digital Volume L4 -- default 1 */
	#define AK_LDVC_DVL5 	0x20		/* Output Digital Volume L5 -- default 0 */
	#define AK_LDVC_DVL6 	0x40		/* Output Digital Volume L6 -- default 0 */
	#define AK_LDVC_DVL7 	0x80		/* Output Digital Volume L7 -- default 0 */

	// values for AK_ALC_Mode_Control_3  (ak4343: R0b)
	#define AK_ALC_Mode_Control_3_RST  0x0
	#define AK_AMC3_D0		0x01		/* 0 -- default 0 */
	#define AK_AMC3_VBAT	0x02		/* HP-Amp Common Voltage -- default 0 */
	#define AK_AMC3_D2		0x04		/* 0 -- default 0 */
	#define AK_AMC3_D3		0x08		/* 0 -- default 0 */
	#define AK_AMC3_D4 		0x10		/* 0 -- default 0 */
	#define AK_AMC3_D5 		0x20		/* 0 -- default 0 */
	#define AK_AMC3_LMTH1 0x40		/* ALC Limiter Detection Level 1 / Recovery Counter Reset Level 1 -- default 0 */
	#define AK_AMC3_RGAIN1 0x80		/* ALC Recovery GAIN Step 1 -- default 0 */

	// values for AK_Rch_Input_Volume_Control  (ak4343: R0c)
	#define AK_Rch_Input_Volume_Control_RST  0xE1
	#define AK_RIVC_AVR0	0x01		/* ALC Block Digital Volume R0 -- default 1 */
	#define AK_RIVC_AVR1	0x02		/* ALC Block Digital Volume R0 -- default 0 */
	#define AK_RIVC_AVR2	0x04		/* ALC Block Digital Volume R0 -- default 0 */
	#define AK_RIVC_AVR3	0x08		/* ALC Block Digital Volume R0 -- default 0 */
	#define AK_RIVC_AVR4 	0x10		/* ALC Block Digital Volume R0 -- default 0 */
	#define AK_RIVC_AVR5 	0x20		/* ALC Block Digital Volume R0 -- default 1 */
	#define AK_RIVC_AVR6 	0x40		/* ALC Block Digital Volume R0 -- default 1 */
	#define AK_RIVC_AVR7 	0x80		/* ALC Block Digital Volume R0 -- default 1 */

	// values for AK_Rch_Digital_Volume_Control  (ak4343: R0d)
	#define AK_Rch_Digital_Volume_Control_RST  0x18
	#define AK_RDVC_DVR0	0x01		/* Output Digital Volume R0 -- default 0 */
	#define AK_RDVC_DVR1	0x02		/* Output Digital Volume R1 -- default 0 */
	#define AK_RDVC_DVR2	0x04		/* Output Digital Volume R2 -- default 0 */
	#define AK_RDVC_DVR3	0x08		/* Output Digital Volume R3 -- default 1 */
	#define AK_RDVC_DVR4 	0x10		/* Output Digital Volume R4 -- default 1 */
	#define AK_RDVC_DVR5 	0x20		/* Output Digital Volume R5 -- default 0 */
	#define AK_RDVC_DVR6 	0x40		/* Output Digital Volume R6 -- default 0 */
	#define AK_RDVC_DVR7 	0x80		/* Output Digital Volume R7 -- default 0 */

	// values for AK_Mode_Control_3  (ak4343: R0e)
	#define AK_Mode_Control_3_RST  0x11
	#define AK_MC3_DEM0		0x01		/* De-emphasis Frequency Select 0 -- default 1 */
	#define AK_MC3_DEM1		0x02		/* De-emphasis Frequency Select 1 -- default 0 */
	#define AK_MC3_BST0		0x04		/* Bass Boost Function Select 0 -- default 0 */
	#define AK_MC3_BST1		0x08		/* Bass Boost Function Select 1 -- default 0 */
	#define AK_MC3_DVOLC 	0x10		/* Output Digital Volume Control Mode Select -- default 1 */
	#define AK_MC3_SMUTE 	0x20		/* 1: DAC outputs Soft Muted  -- default 0 */
	#define AK_MC3_LOOP 	0x40		/* Digital Loopback Mode 1: SDTO=>DAC -- default 0 */
	#define AK_MC3_D7 		0x80		/* 0 -- default 0 */

	// values for AK_Mode_Control_4  (ak4343: R0f)
	#define AK_Mode_Control_4_RST  0x08
	#define AK_MC4_DACH		0x01		/* Switch Control from DAC to Headphone-Amp -- default 0 */
	#define AK_MC4_MINH		0x02		/* Switch Control from MIN pin to Headphone-Amp -- default 0 */
	#define AK_MC4_HPM		0x04		/* Headphone-Amp Mono Output Select -- default 0 */
	#define AK_MC4_AVOLC	0x08		/* ALC Block Digital Volume Control Mode Select -- default 1 */
	#define AK_MC4_D4 		0x10		/* 0 -- default 0 */
	#define AK_MC4_D5 		0x20		/* 0 -- default 0 */
	#define AK_MC4_D6 		0x40		/* 0 -- default 0 */
	#define AK_MC4_D7 		0x80		/* 0 -- default 0 */

	// values for AK_Power_Management_3  (ak4343: R10)
	#define AK_Power_Management_3_RST  0x0
	#define AK_PM3_D0			0x01		/* 0 -- default 0 */
	#define AK_PM3_INL0		0x02		/* Gain-Amp Lch Input Source Select 0 -- default 0 */
	#define AK_PM3_INR0		0x04		/* Gain-Amp Rch Input Source Select 0 -- default 0 */
	#define AK_PM3_MDIF1	0x08		/* Single-ended / Full-differential Input Select 1 -- default 0 */
	#define AK_PM3_MDIF2 	0x10		/* Single-ended / Full-differential Input Select 2 -- default 0 */
	#define AK_PM3_HPG 		0x20		/* Headphone-Amp Gain Select -- default 0 */
	#define AK_PM3_INL1 	0x40		/* Gain-Amp Lch Input Source Select 1 -- default 0 */
	#define AK_PM3_INR1 	0x80		/* Gain-Amp Rch Input Source Select 1 -- default 0 */

	// values for AK_Digital_Filter_Select  (ak4343: R11)
	#define AK_Digital_Filter_Select_RST  0x0
	#define AK_DFS_D0			0x01		/* 0 -- default 0 */
	#define AK_DFS_D1			0x02		/* 0 -- default 0 */
	#define AK_DFS_FIL3		0x04		/* (Stereo Separation Emphasis Filter) Coefficient Setting Enable -- default 0 */
	#define AK_DFS_EQ			0x08		/* (Gain Compensation Filter) Coefficient Setting Enable -- default 0 */
	#define AK_DFS_FIL1 	0x10		/* (Wind-noise Reduction Filter) Coefficient Setting Enable -- default 0 */
	#define AK_DFS_D5 		0x20		/* 0 -- default 0 */
	#define AK_DFS_GN0 		0x40		/* Gain Select at GAIN block 0 -- default 0 */
	#define AK_DFS_GN1 		0x80		/* Gain Select at GAIN block 1 -- default 0 */

	// values for AK_FIL3_Coefficient_0	  F3A7..F3A0				-- default 0
	// values for AK_FIL3_Coefficient_1	  F3AS, 0, F3A13..F3A8	    -- default 0
	// values for AK_FIL3_Coefficient_2	  F3B7..F3B0			    -- default 0
	// values for AK_FIL3_Coefficient_3	  0,0, F3B13..F3B8			-- default 0
	// values for AK_EQ_Coefficient_0	  EQA7..EQA0		        -- default 0
	// values for AK_EQ_Coefficient_1     EQA15..EQA8			    -- default 0
	// values for AK_EQ_Coefficient_2	  EQB7..EQB0	            -- default 0
	// values for AK_EQ_Coefficient_3	  EQB15..EQB8	            -- default 0
	// values for AK_EQ_Coefficient_4	  EQC7..EQC0				-- default 0
	// values for AK_EQ_Coefficient_5	  EQC15..EQC8			    -- default 0
	// values for AK_FIL1_Coefficient_0	  F1A7..F1A0			    -- default 0
	// values for AK_FIL1_Coefficient_1	  F1AS, 0, F1A13..F1A8		-- default 0	
	// values for AK_FIL1_Coefficient_2	  F1B7..F1B0				-- default 0	
	// values for AK_FIL1_Coefficient_3	  0,0, F1B13..F1B8			-- default 0	

	// values for AK_Power_Management_4		   
	#define AK_PM4_PMMICL		0x01		/* Gain-Amp Lch Power Management -- default 0 */
	#define AK_PM4_PMMICR		0x02		/* Gain-Amp Rch Power Management -- default 0 */
	#define AK_PM4_PMAINL2	0x04		/* LIN2 Mixing Circuit Power Management -- default 0 */
	#define AK_PM4_PMAINR2	0x08		/* RIN2 Mixing Circuit Power Management -- default 0 */
	#define AK_PM4_PMAINL3 	0x10		/* LIN3 Mixing Circuit Power Management -- default 0 */
	#define AK_PM4_PMAINR3 	0x20		/* RIN3 Mixing Circuit Power Management -- default 0 */
	#define AK_PM4_D6 			0x40		/* 0 -- default 0 */
	#define AK_PM4_D7 			0x80		/* 0 -- default 0 */
											 
	// values for AK_Mode_Control_5				                    
	#define AK_MC5_RCV		0x01		/* Receiver Select -- default 0 */
	#define AK_MC5_AIN3		0x02		/* Analog Mixing Select -- default 0 */
	#define AK_MC5_D2			0x04		/* -- default 0 */
	#define AK_MC5_D3			0x08		/* -- default 0 */
	#define AK_MC5_MICL3 	0x10		/* Switch Control from Gain-Amp Lch to Analog Output -- default 0 */
	#define AK_MC5_MICR3 	0x20		/* Switch Control from Gain-Amp Rch to Analog Output -- default 0 */
	#define AK_MC5_D6 		0x40		/* 0 -- default 0 */
	#define AK_MC5_D7 		0x80		/* 0 -- default 0 */

	// values for AK_Lineout_Mixing_Select
	#define AK_LMS_LINL2	0x01		/* Switch Control from LIN2 pin to Stereo Line Output (without Gain-Amp) -- default 0 */
	#define AK_LMS_RINL2	0x02		/* Switch Control from RIN2 pin to Stereo Line Output (without Gain-Amp) -- default 0 */
	#define AK_LMS_LINL3	0x04		/* Switch Control from LIN3 pin (or Gain-Amp Lch) to Stereo Line Output -- default 0 */
	#define AK_LMS_RINL3	0x08		/* Switch Control from LIN3 pin (or Gain-Amp Lch) to Stereo Line Output -- default 0 */
	#define AK_LMS_D4	 		0x10		/* 0 -- default 0 */
	#define AK_LMS_D5	 		0x20		/* 0 -- default 0 */
	#define AK_LMS_D6			0x40		/* 0 -- default 0 */
	#define AK_LMS_D7			0x80		/* 0 -- default 0 */

	// values for AK_HP_Mixing_Select			
	#define AK_HMS_LINH2	0x01		/* Switch Control from LIN2 pin to Headphone Output (without Gain -Amp) -- default 0 */
	#define AK_HMS_RINH2	0x02		/* Switch Control from RIN2 pin to Headphone Output (without Gain -Amp) -- default 0 */
	#define AK_HMS_LINH3	0x04		/* Switch Control from LIN3 pin (or Gain-Amp Lch) to Headphone Output -- default 0 */
	#define AK_HMS_RINH3	0x08		/* Switch Control from RIN3 pin (or Gain-Amp Rch) to Headphone Output -- default 0 */
	#define AK_HMS_D4	 		0x10		/* 0 -- default 0 */
	#define AK_HMS_D5	 		0x20		/* 0 -- default 0 */
	#define AK_HMS_D6			0x40		/* 0 -- default 0 */
	#define AK_HMS_D7			0x80		/* 0 -- default 0 */

	// values for AK_SPK_Mixing_Select		
	#define AK_SMS_LINS2	0x01		/* Switch Control from LIN2 pin to Speaker Output (without Gain-Amp) -- default 0 */
	#define AK_SMS_RINS2	0x02		/* Switch Control from RIN2 pin to Speaker Output (without Gain-Amp) -- default 0 */
	#define AK_SMS_LINS3	0x04		/* Switch Control from LIN3 pin (or Gain-Amp Lch) to Speaker Output -- default 0 */
	#define AK_SMS_RINS3	0x08		/* Switch Control from RIN3 pin (or Gain-Amp Rch) to Speaker Output -- default 0 */
	#define AK_SMS_D4	 		0x10		/* 0 -- default 0 */
	#define AK_SMS_D5	 		0x20		/* 0 -- default 0 */
	#define AK_SMS_D6			0x40		/* 0 -- default 0 */
	#define AK_SMS_D7			0x80		/* 0 -- default 0 */

	// AK_Mode_Control_1  AK_MC1_DIF0..1 values
	#define AK_I2S_STANDARD_PHILIPS     0x03
	#define AK_I2S_STANDARD_MSB         0x02
	#define AK_I2S_STANDARD_LSB         0x01
#endif
    

#if defined( AK4637 )		/* AK4637 register map & values */
#define AK_NREGS 35
typedef  union {
	uint8_t	reg[ AK_NREGS ];			// each register & struct is 32bits
	struct {
		__packed struct { // 00H	Power Management 1
			unsigned int PMADC 	: 1; 
			unsigned int Z1 		: 1; 
			unsigned int PMDAC 	: 1; 
			unsigned int LOSEL 	: 1; 
			unsigned int Z4 		: 1; 
			unsigned int PMBP 	: 1; 
			unsigned int PMVCM 	: 1; 
			unsigned int PMPFIL : 1; 
		} PwrMgmt1;
		__packed struct { // 01H	Power Management 2
			unsigned int Z0 		: 1; 
			unsigned int PMSL 	: 1; 
			unsigned int PMPLL 	: 1; 
			unsigned int M_S 		: 1; 
			unsigned int Z7_4		: 4; 
		} PwrMgmt2;
		__packed struct { // 02H  Signal Select 1 
			unsigned int MGAIN2_0 : 3;
			unsigned int PMMP			: 1;
			unsigned int Z4				: 1;
			unsigned int DACS			: 1;
			unsigned int MGAIN3		: 1;
			unsigned int SLPSN		: 1;
		} SigSel1;
		__packed struct { // 03H  Signal Select 2 
			unsigned int MDIF			: 1;
			unsigned int Z3_1			: 3;
			unsigned int MICL			: 1;
			unsigned int Z5				: 1;
			unsigned int SPKG1_0	: 2;
		} SigSel2;
		__packed struct { // 04H Signal Select 3 
			unsigned int Z4_0			: 5;
			unsigned int DACL			: 1;
			unsigned int LVCM1_0	: 2;
		} SigSel3; 
		__packed struct { // 05H Mode Control 
			unsigned int BCKO1_0	: 2;
			unsigned int CKOFF		: 1;
			unsigned int Z3				: 1;
			unsigned int PLL3_0		: 4;
		} MdCtr1;   
		__packed struct { // 06H Mode Control 2
			unsigned int FS3_0		: 4;
			unsigned int Z5_4			: 1;
			unsigned int CM1_0		: 2;
		} MdCtr2; 
		__packed struct { // 07H Mode Control 3  
			unsigned int DIF1_0			: 2;
			unsigned int BCKP				: 1;
			unsigned int MSBS				: 1;
			unsigned int Z4					: 1;
			unsigned int SMUTE			: 1;
			unsigned int THDET			: 1;
			unsigned int TSDSEL			: 1;
		} MdCtr3;
		__packed struct { // 08H Digital MIC
			unsigned int DMIC				: 1;
			unsigned int DCLKP			: 1;
			unsigned int Z2					: 1;
			unsigned int DCLKE			: 1;
			unsigned int PMDM				: 1;
			unsigned int Z7_5				: 3;
		} DMIC;
		__packed struct { // 09H Timer Select 
			unsigned int DVTM				: 1;
			unsigned int Z3_1				: 3;
			unsigned int FRN				: 1;
			unsigned int FRATT			: 1;
			unsigned int ADRST1_0		: 2;
		} TimSel;
		__packed struct { // 0AH ALC Timer Select 
			unsigned int RFST1_0		: 2;
			unsigned int WTM1_0			: 2;
			unsigned int EQFC1_0		: 2;
			unsigned int IVTM				: 1;
			unsigned int Z7					: 1;
		} AlcTimSel;
		__packed struct { // 0BH ALC Mode Control 1 
			unsigned int LMTH1_0		: 2;
			unsigned int RGAIN2_0		: 3;
			unsigned int ALC				: 1;
			unsigned int LMTH2			: 1;
			unsigned int ALCEQN			: 1;
		} AlcMdCtr1;
		__packed struct { // 0CH ALC Mode Control 2 
			unsigned int REF7_0			: 8;
		} AlcMdCtr2;
		__packed struct { // 0DH  Input Volume Control 
			unsigned int IVOL7_0		: 8;
		} InVolCtr;
		__packed struct { // 0EH ALC Volume 
			unsigned int VOL7_0			: 8;
		} AlcVol;
		__packed struct { // 0FH BEEP Control 
			unsigned int BPLVL3_0		: 4;
			unsigned int Z4					: 1;
			unsigned int BEEPS			: 1;
			unsigned int BPVCM			: 1;
			unsigned int Z7					: 1;
		} BpCtr;
		__packed struct { // 10H Digital Volume Control 
			unsigned int DVOL7_0		: 8;
		} DigVolCtr;
		__packed struct { // 11H EQ Common Gain Select 
			unsigned int Z0					: 1;
			unsigned int EQC5_2			: 4;
			unsigned int Z7_5				: 3;
		} EqComGn;
		__packed struct { // 12H EQ2 Gain Setting 
			unsigned int EQ2T1_0		: 2;
			unsigned int EQ2G5_0		: 6;
		} EqGn2;
		__packed struct { // 13H EQ3 Gain Setting
			unsigned int EQ3T1_0		: 2;
			unsigned int EQ3G5_0		: 6;
		} EqGn3;
		__packed struct { // 14H EQ4 Gain Setting
			unsigned int EQ4T1_0		: 2;
			unsigned int EQ4G5_0		: 6;
		} EqGn4;
		__packed struct { // 15H EQ5 Gain Setting
			unsigned int EQ5T1_0		: 2;
			unsigned int EQ5G5_0		: 6;
		} EqGn5;
		__packed struct { // 16H Digital Filter Select 1
			unsigned int HPFAD			: 1;
			unsigned int HPFC1_0		: 2;
			unsigned int Z7_3				: 5;
		} DigFilSel1;
		__packed struct { // 17H Digital Filter Select 2 
			unsigned int HPF				: 1;
			unsigned int LPF				: 1;
			unsigned int Z7_2				: 6;
		} DigFilSel2;
		__packed struct { // 18H Digital Filter Mode 
			unsigned int PFSDO			: 1;
			unsigned int ADCPF			: 1;
			unsigned int PFDAC1_0		: 2;
			unsigned int PFVOL1_0		: 2;
			unsigned int Z7_6				: 2;
		} DigFilMd;
		__packed struct { // 19H HPF2 Co-efficient 0 
			unsigned int F1A7_0			: 8;
		} HpfC0;
		__packed struct { // 1AH HPF2 Co-efficient 1
			unsigned int F1A13_8		: 6;
			unsigned int Z7_6				: 2;
		} HpfC1;
		__packed struct { // 1BH HPF2 Co-efficient 2 
			unsigned int F1B7_0			: 8;
		} HpfC2;
		__packed struct { // 1CH HPF2 Co-efficient 3
			unsigned int F1B13_8		: 6;
			unsigned int Z7_6				: 2;
		} HpfC3;
		__packed struct { // 1DH  LPF Co-efficient 0
			unsigned int F2A7_0			: 8;
		} LpfC0;
		__packed struct { // 1EH LPF Co-efficient 1 
			unsigned int F2A13_8		: 6;
			unsigned int Z7_6				: 2;
		} LpfC1;
		__packed struct { // 1FH LPF Co-efficient 2 
			unsigned int F2B7_0			: 8;
		} LpfC2;
		__packed struct { // 20H LPF Co-efficient 3 
			unsigned int F2B13_8		: 6;
			unsigned int Z7_6				: 2;
		} LpfC3;
		__packed struct { // 21H Digital Filter Select 3 
			unsigned int EQ5_1			: 5;
			unsigned int Z7_5				: 3;
		} DigFilSel3;
	} R;
} AK4637_Registers;

/* AK4637 register map */
	#define   AK_Power_Management_1			0x00
	#define   AK_Power_Management_2			0x01
	#define   AK_Signal_Select_1				0x02
	#define   AK_Signal_Select_2				0x03
	#define   AK_Signal_Select_3				0x04
	#define   AK_Mode_Control_1					0x05
	#define   AK_Mode_Control_2					0x06
	#define   AK_Mode_Control_3					0x07
	#define   AK_Digital_MIC						0x08
	#define   AK_Timer_Select						0x09
	#define   AK_ALC_Timer_Select				0x0A
	#define   AK_ALC_Mode_Control_1			0x0B
	#define   AK_ALC_Mode_Control_2			0x0C
	#define   AK_Input_Volume_Control		0x0D
	#define   AK_ALC_Volume							0x0E
	#define   AK_BEEP_Control						0x0F
	#define   AK_Digital_Volume_Control	0x10
	#define   AK_EQ_Common_Gain_Select	0x11
	#define   AK_EQ2_Gain_Setting				0x12
	#define   AK_EQ3_Gain_Setting				0x13
	#define   AK_EQ4_Gain_Setting				0x14
	#define   AK_EQ5_Gain_Setting				0x15
	#define   AK_Digital_Filter_Select_1	0x16
	#define   AK_Digital_Filter_Select_2	0x17
	#define   AK_Digital_Filter_Mode		0x18
	#define   AK_HPF2Coefficient_0			0x19
	#define   AK_HPF2Coefficient_1			0x1A
	#define   AK_HPF2Coefficient_2			0x1B
	#define   AK_HPF2Coefficient_3			0x1C
	#define   AK_LPF_Coefficient_0			0x1D
	#define   AK_LPF_Coefficient_1			0x1E
	#define   AK_LPF_Coefficient_2			0x1F
	#define   AK_LPF_Coefficient_3			0x20
	#define   AK_Digital_Filter_Select_3	0x21
	#define   AK_E1_Coefficient_0				0x22
	#define   AK_E1_Coefficient_1				0x23
	#define   AK_E1_Coefficient_2				0x24
	#define   AK_E1_Coefficient_3				0x25
	#define   AK_E1_Coefficient_4				0x26
	#define   AK_E1_Coefficient_5				0x27
	#define   AK_E2_Coefficient_0				0x28
	#define   AK_E2_Coefficient_1				0x29
	#define   AK_E2_Coefficient_2				0x2A
	#define   AK_E2_Coefficient_3				0x2B
	#define   AK_E2_Coefficient_4				0x2C
	#define   AK_E2_Coefficient_5				0x2D
	#define   AK_E3_Coefficient_0				0x2E
	#define   AK_E3_Coefficient_1				0x2F
	#define   AK_E3_Coefficient_2				0x30
	#define   AK_E3_Coefficient_3				0x31
	#define   AK_E3_Coefficient_4				0x32
	#define   AK_E3_Coefficient_5				0x33
	#define   AK_E4_Coefficient_0				0x34
	#define   AK_E4_Coefficient_1				0x35
	#define   AK_E4_Coefficient_2				0x36
	#define   AK_E4_Coefficient_3				0x37
	#define   AK_E4_Coefficient_4				0x38
	#define   AK_E4_Coefficient_5				0x39
	#define   AK_E5_Coefficient_0				0x3A
	#define   AK_E5_Coefficient_1				0x3B
	#define   AK_E5_Coefficient_2				0x3C
	#define   AK_E5_Coefficient_3				0x3D
	#define   AK_E5_Coefficient_4				0x3E
	#define   AK_E5_Coefficient_5				0x3F
	#define		AK_LAST_RST_REG		0x18
	
 // reset values for AK registers
	#define AK_Power_Management_1_RST  0x0
	#define AK_Power_Management_2_RST  0x0
	#define AK_Signal_Select_1_RST  0x06
	#define AK_Signal_Select_2_RST  0x0
	#define   AK_Signal_Select_3_RST	0x40
	#define   AK_Mode_Control_1_RST		0x50
	#define   AK_Mode_Control_2_RST		0x0B
	#define   AK_Mode_Control_3_RST		0x02
	#define AK_MC3_PHILIPS 0x03		/* DIF0..1 == 3 I2S compatible */
	#define   AK_Digital_MIC_RST			0x00
	#define AK_Timer_Select_RST  0x0
	#define AK_ALC_Timer_Select_RST			0x60
	#define AK_ALC_Mode_Control_1_RST		0x00
	#define AK_ALC_Mode_Control_2_RST		0xE1
	#define AK_Input_Volume_Control_RST		0xE1
	#define   AK_ALC_Volume_RST					0x00
	#define   AK_BEEP_Control_RST				0x00
	#define   AK_Digital_Volume_Control_RST		0x18
	#define   AK_EQ_Common_Gain_Select_RST	 0x00
	#define   AK_EQ1_Gain_Setting_RST	 0x00
	#define   AK_EQ2_Gain_Setting_RST	 0x00
	#define   AK_EQ3_Gain_Setting_RST	 0x00
	#define   AK_EQ4_Gain_Setting_RST	 0x00
	#define   AK_EQ5_Gain_Setting_RST	 0x00
	#define   AK_Digital_Filter_Select_1_RST		0x01
	#define   AK_Digital_Filter_Select_2_RST		0x00
	#define   AK_Digital_Filter_Mode_RST		0x03
#endif

#define MAX_VOLUME   100
#define MIN_VOLUME   0
#define DEFAULT_VOLUME   80

// audio codec functions --  ak4343 or ak4637
void 						ak_Init( void );											// init I2C & codec
void						I2C_Reinit(int lev );									// lev&1 SWRST, &2 => RCC reset, &4 => device re-init
void						ak_PowerUp( void );										// supply power to codec, & disable PDN, before re-init
void 						ak_PowerDown( void );									// power down entire codec, requires re-init
void		 				ak_SetVolume( uint8_t Volume );				// 0..100%
void		 				ak_SetMute( bool muted );							// for temporarily muting, while speaker powered
void 						ak_SetOutputMode( uint8_t Output );
void 						ak_SpeakerEnable( bool enable );			// power up/down lineout, DAC, speaker amp
void						ak_SetMasterFreq( int freq );					// set AK4637 to MasterMode, 12MHz ref input to PLL, audio @ 'freq'
void						ak_MasterClock( bool enable );				// enable/disable PLL (Master Clock from codec) -- fill DataReg before enabling 

#endif /* __AK4xxx_H */
