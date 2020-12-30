//******* codecI2C.c   
//  I2C communication to audio codec, using CMSIS-Driver/I2C
//  extracted from ak4xxx.c for use by ti_aic3100

/* Includes ------------------------------------------------------------------*/
#include "Driver_I2C.h"			// CMSIS-Driver I2C interface
#include "codecI2C.h"				// I2C externals

//*****************  CMSIS I2C driver interface
extern ARM_DRIVER_I2C     Driver_I2C1;
static ARM_DRIVER_I2C *		I2Cdrv = 				&Driver_I2C1;

#if !defined (VERIFY_WRITTENDATA)  
// Uncomment this line to enable verifying data sent to codec after each write operation (for debug purpose)
#define VERIFY_WRITTENDATA 
#endif /* VERIFY_WRITTENDATA */

enum { 
 			I2C_Xmt = 0,
 			I2C_Rcv = 1,
 			I2C_Evt = 2,		// errors reported from ISR 
 			Cdc_DefReg = 3,
 			Cdc_WrReg = 4
} errCntTypes;

static int 			I2C_ErrCnt[ ]= { 0,0,0,0,0 };
static char * 	cdcGrp[] = { "Xmt", "Rcv", "Evt", "Def", "Wr" };

// AIC_REG{ pg, reg, reset_val, W1_allowed, W1_required, nm, curr_val, next_val }
// defined in ti_aic3100:   static AIC_REG	codec_regs[];

static int 											codecCurrPg = 0;			// keeps track of currently selected register page

static volatile int32_t 				I2C_Event = 0;
static int											MaxBusyWaitCnt = 0;
static int											eCnt = 0;
const  int 											maxErrLog	= 10;				// max errors to add to log

static int 											reinitCnt = 0;	

static int 											WatchReg   = -1;			//DEBUG: log register read/write ops for a specific codec register

static bool											codecDummyWrite = false;		// if true, expect first I2C write to fail

int 						regIdx( uint8_t pg, uint8_t reg ){					// search for & return index of codec_regs[] entry for pg/reg
	for (int i=0; i < codecNREGS; i++ )
		if ( codec_regs[i].pg==pg && codec_regs[i].reg==reg ) return i;
	errLog("codec reg not found pg%d reg%d", pg,reg ); return 0;
}

void 						Codec_WrReg( uint8_t Reg, uint8_t Value);		// FORWARD

#ifdef VERIFY_WRITTENDATA	
	const uint8_t AK_LAST_REG_MKR = 0xff;
	struct {  uint8_t reg; uint8_t def; } RegDefault[] = {
	#if defined( AK4343 )
		{ AK_Power_Management_1, 				AK_PM1_VCM }, // instead of AK_Power_Management_1_RST }, because VCM set before checkRegs
		{ AK_Power_Management_2, 				AK_Power_Management_2_RST },
		{ AK_Signal_Select_1, 					AK_Signal_Select_1_RST },
		{ AK_Signal_Select_2, 					AK_Signal_Select_2_RST },
		{ AK_Mode_Control_1, 						AK_Mode_Control_1_RST },
		{ AK_Mode_Control_2, 						AK_Mode_Control_2_RST },
		{ AK_Timer_Select, 							AK_Timer_Select_RST },
		{ AK_ALC_Mode_Control_1, 				AK_ALC_Mode_Control_1_RST },
		{ AK_ALC_Mode_Control_2, 				AK_ALC_Mode_Control_2_RST },
		{ AK_Lch_Input_Volume_Control, 	AK_Lch_Input_Volume_Control_RST },
		{ AK_Lch_Digital_Volume_Control,AK_Lch_Digital_Volume_Control_RST },
		{ AK_ALC_Mode_Control_3, 				AK_ALC_Mode_Control_3_RST },
		{ AK_Rch_Input_Volume_Control, 	AK_Rch_Input_Volume_Control_RST },
		{ AK_Rch_Digital_Volume_Control,AK_Rch_Digital_Volume_Control_RST },
		{ AK_Mode_Control_3, 						AK_Mode_Control_3_RST },
		{ AK_Mode_Control_4, 						AK_Mode_Control_4_RST },
		{ AK_Power_Management_3, 				AK_Power_Management_3_RST },
		{ AK_Digital_Filter_Select, 		AK_Digital_Filter_Select_RST },
	#endif
	#if defined( AK4637 )
		{ AK_Power_Management_1, 				AK_Power_Management_1_RST }, 
		{ AK_Power_Management_2, 				AK_Power_Management_2_RST },
		{ AK_Signal_Select_1, 					AK_Signal_Select_1_RST },
		{ AK_Signal_Select_2, 					AK_Signal_Select_2_RST },
		{ AK_Signal_Select_3, 					AK_Signal_Select_3_RST },
		{ AK_Mode_Control_1, 						AK_Mode_Control_1_RST },
		{ AK_Mode_Control_2, 						AK_Mode_Control_2_RST },
		{ AK_Mode_Control_3, 						AK_Mode_Control_3_RST },
		{ AK_Digital_MIC, 							AK_Digital_MIC_RST },
		{ AK_Timer_Select, 							AK_Timer_Select_RST },
		{ AK_ALC_Timer_Select,					AK_ALC_Timer_Select_RST },
		{ AK_ALC_Mode_Control_1, 				AK_ALC_Mode_Control_1_RST },
		{ AK_ALC_Mode_Control_2, 				AK_ALC_Mode_Control_2_RST },
		{ AK_Input_Volume_Control, 			AK_Input_Volume_Control_RST },
		{ AK_ALC_Volume, 								AK_ALC_Volume_RST },
		{ AK_BEEP_Control, 							AK_BEEP_Control_RST },
		{ AK_EQ_Common_Gain_Select, 		AK_EQ_Common_Gain_Select_RST },
		{ AK_EQ2_Gain_Setting, 					AK_EQ2_Gain_Setting_RST },
		{ AK_EQ3_Gain_Setting, 					AK_EQ3_Gain_Setting_RST },
		{ AK_EQ4_Gain_Setting, 					AK_EQ4_Gain_Setting_RST },
		{ AK_EQ5_Gain_Setting, 					AK_EQ5_Gain_Setting_RST },
		{ AK_Digital_Filter_Select_1, 	AK_Digital_Filter_Select_1_RST },
		{ AK_Digital_Filter_Select_2, 	AK_Digital_Filter_Select_2_RST },
		{ AK_Digital_Filter_Mode, 			AK_Digital_Filter_Mode_RST },
	#endif  
	{ AK_LAST_REG_MKR,							AK_LAST_REG_MKR }
};
#endif

void 		 				cntErr( int8_t grp, int8_t exp, int8_t got, int32_t reg, int16_t caller ){  // count and/or report ak errors
	if ( got == exp || codecDummyWrite ) return;
	codecDummyWrite = false;
	I2C_ErrCnt[ grp ]++;
	eCnt++;
	dbgEvt( TB_i2cErr, (caller << 8) + grp, exp, got, reg );
	if ( eCnt < maxErrLog && grp!=I2C_Evt )	// can't report I2C_Evt (from ISR)
		errLog("AK %s err, exp=0x%x, got=0x%x reg=%d caller=%d", cdcGrp[grp], exp, got, reg, caller );
}
void 						i2c_Sig_Event( uint32_t event ){														// called from ISR for I2C errors or complete
	I2C_Event = event;
	cntErr( I2C_Evt, ARM_I2C_EVENT_TRANSFER_DONE, event, I2Cdrv->GetDataCount(), 1 );
}
void		 				i2c_initialize() {																					// Initialize I2C device
//	I2Cdrv->Control( ARM_I2C_BUS_CLEAR, 0 );  // resets GPIO, etc. -- if needed, should be before initialization
  I2Cdrv->Initialize( i2c_Sig_Event );					// sets up SCL & SDA pins, evt handler
  I2Cdrv->PowerControl( ARM_POWER_FULL );		// clocks I2C device & sets up interrupts
  I2Cdrv->Control( ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_STANDARD );		// set I2C bus speed
  I2Cdrv->Control( ARM_I2C_OWN_ADDRESS, 0 );		// set I2C own address-- REQ to initialize properly
}
void 						i2c_Init() {																								// Initialize I2C connection to reset codec & write a reg
  i2c_initialize();				// init device
}
void						i2c_Reinit(int lev ){																				// lev&1 SWRST, &2 => RCC reset, &4 => device re-init
	if ( lev & 4 ){
		I2Cdrv->Uninitialize( );					// deconfigures SCL & SDA pins, evt handler
		i2c_initialize();
	}
	if ( lev & 2 ){
		RCC->APB1RSTR |= RCC_APB1RSTR_I2C1RST;		// set device reset bit for I2C1
		tbDelay_ms( 3 ); //DEBUG  1 );
		RCC->APB1RSTR = 0;		// TURN OFF device reset bit for I2C1
/* reset sequence in PowerControl( ARM_POWER_FULL )
        __HAL_RCC_I2C1_FORCE_RESET();
        __NOP(); __NOP(); __NOP(); __NOP(); 
        __HAL_RCC_I2C1_RELEASE_RESET();
*/		
	}
	if ( lev & 1 ){
		uint32_t cr1 = I2C1->CR1;
		I2C1->CR1 = I2C_CR1_SWRST;			// set Software Reset bit
		tbDelay_ms( 3 ); //DEBUG 1);
		I2C1->CR1 = cr1;			// reset to previous
	}
}
#ifdef VERIFY_WRITTENDATA
uint8_t 				Codec_RdReg( int iReg ){																		// return value of codec register Reg 
	int status;
	uint8_t value;
	
	uint8_t page = codec_regs[ iReg ].pg;
	uint8_t regAddr = codec_regs[ iReg ].reg;
	
	if ( page != codecCurrPg ){		// recursively write register 0 of current page, to switch to new page
		int pgIdx = regIdx( codecCurrPg, 0 );		// find index of page register in current page
		Codec_WrReg( pgIdx, page );
		codecCurrPg = page;
	}
	
  I2C_Event = 0U;  		// Clear event flags before new transfer
  status = I2Cdrv->MasterTransmit( AUDIO_I2C_ADDR, &regAddr, 1, true );		// send register index
	cntErr( I2C_Xmt, ARM_DRIVER_OK, status, 0, 2 );
	
	int waitCnt = 0;
  while (( I2C_Event & ARM_I2C_EVENT_TRANSFER_DONE ) == 0U)
		waitCnt++;  // busy wait until transfer completed
	
	cntErr( I2C_Xmt, ARM_I2C_EVENT_TRANSFER_DONE, I2C_Event, 0, 3 );  // err if any other bits set

  I2C_Event = 0U;  		// Clear event flags before new transfer
	status = I2Cdrv->MasterReceive( AUDIO_I2C_ADDR, &value, 1, false );  // receive register contents into *value
	cntErr( I2C_Rcv, ARM_DRIVER_OK, status, iReg, 4 );

  while (( I2C_Event & ARM_I2C_EVENT_TRANSFER_DONE ) == 0U)
		waitCnt++;  // busy wait until transfer completed
	if ( waitCnt > MaxBusyWaitCnt ) MaxBusyWaitCnt = waitCnt;
	cntErr( I2C_Xmt, ARM_I2C_EVENT_TRANSFER_DONE, I2C_Event, 0, 5 );  // err if any other bits set
	
	if ( iReg==WatchReg )
		dbgLog( " iR%d(%d:%02x) = %02x\n", iReg, codec_regs[iReg].pg, codec_regs[iReg].reg, value );
	
	if ( codec_regs[iReg].curr_val != value ){ // unexpected value read
		// error will be reported by Codec_RdReg() caller
		codec_regs[iReg].curr_val = codec_regs[iReg].next_val = value;  // update both local images to value that was read
	}
	return value;
}
#endif

void 						Codec_WrReg( uint8_t iReg, uint8_t Value){									// write codec register Reg with Value
	uint8_t page = codec_regs[ iReg ].pg;
	uint8_t regAddr = codec_regs[ iReg ].reg;
	
	dbgEvt( TB_akWrReg, iReg, page, regAddr, Value );
	if ( iReg==WatchReg )
		dbgLog( "Wr iR%d(%d:%02x) <- %02x \n", iReg, codec_regs[iReg].pg, codec_regs[iReg].reg, Value );

	uint32_t status;
	int waitCnt = 0;
  
	if ( page != codecCurrPg ){		// recursively write register 0 of current page, to switch to new page
		int pgIdx = regIdx( codecCurrPg, 0 );		// find index of page register in current page
		Codec_WrReg( pgIdx, page );
		codecCurrPg = page;
	}

  uint8_t buf[2];
  buf[0] = regAddr;
  buf[1] = (Value & codec_regs[iReg].W1_allowed) | codec_regs[iReg].W1_allowed;		// mask off bits that must be 0, and set bits that must be 1
  I2C_Event = 0U;  		// Clear event flags before new transfer
  status = I2Cdrv->MasterTransmit( AUDIO_I2C_ADDR, buf, 2, false );
	cntErr( I2C_Xmt, ARM_DRIVER_OK, status, iReg, 6 );
	
  while (( I2C_Event & ARM_I2C_EVENT_TRANSFER_DONE ) == 0U){
		waitCnt++;  // busy wait until transfer completed
		if ( waitCnt > 1000 ){
			errLog("I2C Tx hang, r=%d v=0x%x", iReg, Value);
			reinitCnt++;
			if ( reinitCnt > 16 ) tbErr( "Can't restart I2C\n" );
			i2c_Reinit( reinitCnt );			// SW, RST, SW+RST, INIT, INIT+SW, ...
			Codec_WrReg( iReg, Value );		// recursive call to retry on reset device
			return;
		}
	}
	if ( waitCnt > MaxBusyWaitCnt ) MaxBusyWaitCnt = waitCnt;
	cntErr( I2C_Xmt, ARM_I2C_EVENT_TRANSFER_DONE, I2C_Event, 0, 7 );  // err if any other bits set
	
//	dbgLog( "W%x:%02x ", Reg, Value );
	#ifdef VERIFY_WRITTENDATA
		/* Verify that the data has been correctly written */  
		int chkVal = Codec_RdReg( iReg );
		cntErr( Cdc_WrReg, Value, chkVal, iReg, 8 );
	#endif /* VERIFY_WRITTENDATA */
	reinitCnt = 0;			// success-- reset recursive counter
}
void  					i2c_ReportErrors(){																					// report I2C error counts
  int tot = 0;
	for ( int i=0; i<= Cdc_WrReg; i++ )
	  tot += I2C_ErrCnt[i];
	if (tot==0) return;
	
	dbgLog( "%d I2C Errs-- Xmt:%d Rcv:%d Evt:%d Def:%d WrReg:%d \n", tot,
		I2C_ErrCnt[ I2C_Xmt ], I2C_ErrCnt[ I2C_Rcv ], I2C_ErrCnt[ I2C_Evt ], I2C_ErrCnt[ Cdc_DefReg ], I2C_ErrCnt[ Cdc_WrReg ] );
}
void						i2c_Upd(){																									// write values of any modified codec registers
	for ( int i=0; i < codecNREGS; i++ ){
		uint8_t nv = codec_regs[i].next_val; 
		uint8_t cv = codec_regs[i].curr_val; 
		if ( nv != cv ){		// register has changed -- write it to codec
			Codec_WrReg( i, nv );
			codec_regs[i].curr_val = nv;		// update stored curr_val
		}
	}
}
void 						i2c_CheckRegs(){																						// Debug -- read codec regs
	#ifdef VERIFY_WRITTENDATA	
		for ( int i = 0; i < codecNREGS; i++ ){
			uint8_t defval = codec_regs[i].reset_val;
			uint8_t val = Codec_RdReg( i );
			cntErr( Cdc_DefReg, defval, val, i, 9 );	
		}
		i2c_ReportErrors();
	#endif
}
//end file **************** codecI2C.c
