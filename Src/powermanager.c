// TBookV2  powermanager.c
//   Apr 2018

#include "powerMgr.h"
#include "tbook.h"
//#include "vsCodec.h"

// pwrEventFlags--  PM_NOPWR  PM_PWRCHK   PM_ADCDONE
#define 									PM_NOPWR  1											// id for osEvent signaling NOPWR interrupt
#define 									PM_PWRCHK 2											// id for osTimerEvent signaling periodic power check
#define 									PM_ADCDONE  4										// id for osEvent signaling period power check interrupt
#define 									PWR_TIME_OUT 30000							// 30sec periodic power check

//static HPE 								powerHandlers[20];
//static uint8_t 						refCount = 0;
static osTimerId_t				pwrCheckTimer = NULL;
static osEventFlagsId_t 	pwrEvents = NULL;

static osThreadAttr_t 		pm_thread_attr;

static void 							powerThreadProc( void *arg );		// forward
static void 							initPwrSignals( void );					// forward

void checkPowerTimer(void *arg);                          // forward for timer callback function

void											initPowerMgr( void ){						// initialize PowerMgr & start thread
	pwrEvents = osEventFlagsNew(NULL);
	if ( pwrEvents == NULL )
		tbErr( "pwrEvents not alloc'd" );
	
	osTimerAttr_t 	timer_attr;
	timer_attr.name = "PM Timer";
	timer_attr.attr_bits = 0;
	timer_attr.cb_mem = 0;
	timer_attr.cb_size = 0;
	pwrCheckTimer = osTimerNew(	checkPowerTimer, osTimerPeriodic, 0, &timer_attr);
	if ( pwrCheckTimer == NULL ) 
		tbErr( "pwrCheckTimer not alloc'd" );
	
	initPwrSignals();		// setup power pins & NO_PWR interrupt

  if (true) return;
		
	pm_thread_attr.name = "PM Thread";	
	pm_thread_attr.stack_size = POWER_STACK_SIZE; 
	osThreadId_t pm_thread =  osThreadNew( powerThreadProc, 0, &pm_thread_attr ); //&pm_thread_attr );
	if ( pm_thread == NULL ) 
		tbErr( "powerThreadProc not created" );
		
	int timerMS = TB_Config.powerCheckMS;
	if ( timerMS==0 ) 
		timerMS = PWR_TIME_OUT;
	osTimerStart( pwrCheckTimer, timerMS );
	dbgLog( "PowerMgr OK \n" );
}
void 											enableSleep( void ){						// low power mode -- CPU stops till interrupt
	__WFE();	// CMSIS sleep till interrupt
}
void 											enableStandby( void ){					// power off-- reboot on wakeup from NRST
}
void											powerDownTBook( void ){					// shut down TBook
	logPowerDown();		// flush & close logs
	ledBg("_");
	ledFg("_");
	enableStandby();
}
void 											initPwrSignals( void ){					// configure power GPIO pins, & EXTI on NOPWR 	
	// power supply signals
	gConfigOut( gADC_ENABLE );	// 1 to enable battery voltage levels
	gSet( gADC_ENABLE, 0 );
	gConfigOut( gSC_ENABLE );		// 1 to enable Solar Charging
	gSet( gSC_ENABLE, 0 );
	
	gConfigADC( gADC_LI_ION );	// rechargable battery voltage level
	gConfigADC( gADC_PRIMARY );	// disposable battery voltage level

	gConfigIn( gPWR_FAIL_N, false );	// PD0 -- input power fail signal, with pullup
	
	gConfigIn( gBAT_PG_N,   false );	// MCP73871 PG_    IN:  0 => power is good  (configured active high)
	gConfigIn( gBAT_STAT1,  false );  // MCP73871 STAT1  IN:  open collector with pullup
	gConfigIn( gBAT_STAT2,  false );  // MCP73871 STAT2  IN:  open collector with pullup

	gConfigOut( gBAT_CE );  	// OUT: 1 to enable charging 			MCP73871 CE 	  (powermanager.c)
	gConfigOut( gBAT_TE_N );  // OUT: 0 to enable safety timer 	MCP73871 TE_	  (powermanager.c)
	
	// power controls to memory devices
	gConfigOut( gEMMC_RSTN );		// 0 to reset? eMMC
	gConfigOut( g3V3_SW_EN );		// 1 to enable power to SDCard & eMMC
	gSet( gEMMC_RSTN, 1 );			// enable at power up?
	gSet( g3V3_SW_EN, 1 );			// enable at start up, for FileSys access
	
  // Configure audio power control 
	gConfigOut( gEN_5V );				// 1 to supply 5V to codec-- enable AP6714 regulator
	gConfigOut( gNLEN01V8 );		// 1 to supply 1.8 to codec-- enable TLV74118 regulator	
	gConfigOut( gBOOT1_PDN );		// _: 0 to power down codec-- boot1_pdn on AK4637
	gConfigOut( gPA_EN );				// 1 to power headphone jack
	gSet( gEN_5V, 0 );					// initially codec OFF
	gSet( gNLEN01V8, 0 );				// initially codec OFF
	gSet( gBOOT1_PDN, 1 );			// initially codec OFF
	gSet( gPA_EN, 0 );					// initially headphones OFF
	
	tbDelay_ms( 3 );		// wait 3 msec to make sure everything is stable
}

void											checkPowerTimer( void *arg ){		// timer to signal periodic power status check
	osEventFlagsSet( pwrEvents, PM_PWRCHK );						// wakeup powerThread for power status check
}
void 											checkPower( ){				// check and report power status
	//  check gPWR_FAIL_N & MCP73871: gBAT_PG_N, gBAT_STAT1, gBAT_STAT2
	bool PwrFail 			= gGet( gPWR_FAIL_N );	// PD0 -- input power fail signal

	//  check MCP73871: gBAT_PG_N, gBAT_STAT1, gBAT_STAT2
	bool PwrGood_N 	  = gGet( gBAT_PG_N );	 // MCP73871 PG_:  0 => power is good   			MCP73871 PG_    (powermanager.c)
	bool BatStat1 		= gGet( gBAT_STAT1 );  // MCP73871 STAT1
	bool BatStat2 		= gGet( gBAT_STAT2 );  // MCP73871 STAT2
	
	// MCP73871 battery charging -- BatStat1==STAT1  BatStat2==STAT2  BatPwrGood==PG
	// Table 5.1  Status outputs
	//   STAT1 STAT2 PG_
	// 		H			H			H		7-- no USB input power
	// 		H			H			L		6-- no LiIon battery
	//		H			L			L		4-- charge completed
	//		L			H			H		3-- low battery output
	//		L			H			L		2-- charging
	// 		L			L			L		0-- temperature (or timer) fault

	enum PwrStat { TEMPFAULT=0, xxx=1, CHARGING=2, LOWBATT=3, CHARGED=4, xxy=5, NOLITH=6, NOUSBPWR=7 };
	enum PwrStat pstat = (enum PwrStat) ((BatStat1? 4:0) + (BatStat2? 2:0) + (PwrGood_N? 1:0));
	dbgLog( "PCk: F%d Bat:%d \n", PwrFail, pstat );
}

void 											wakeup(){												// resume operation after sleep power-down
}
static void 							powerThreadProc( void *arg ){		// powerThread -- catches PM_NOPWR from EXTI: NOPWR interrupt
	while( true ){
		int flg = osEventFlagsWait( pwrEvents, PM_NOPWR | PM_PWRCHK, osFlagsWaitAny, osWaitForever );	
		
		switch ( flg ){
			case PM_NOPWR:
				logEvt( "Powering Down \n");
				enableStandby();			// shutdown-- reboot on wakeup keys
				// never come back  -- just restarts at main()
				break;

			case PM_PWRCHK:
				checkPower( );			// get current power status
				break;
		}
	} 
}
//end  powermanager.c
