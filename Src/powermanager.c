// TBookV2  powermanager.c
//   Apr 2018

#include "powerMgr.h"
#include "tbook.h"
//#include "stm32f4xx_hal_dma.h"
//#include "stm32f4xx_hal_adc.h"

extern fsTime					RTC_initTime;					// time from status.txt 
extern bool						firstBoot;						// true if 1st run after data loaded


// pwrEventFlags--  PM_NOPWR  PM_PWRCHK   PM_ADCDONE		-- signals from IRQ to powerThread
#define 									PM_NOPWR  1											// id for osEvent signaling NOPWR interrupt
#define 									PM_PWRCHK 2											// id for osTimerEvent signaling periodic power check
#define 									PM_ADCDONE  4										// id for osEvent signaling period power check interrupt
#define 									PWR_TIME_OUT 30000							// 30sec periodic power check

static osTimerId_t				pwrCheckTimer = NULL;
static osEventFlagsId_t 	pwrEvents = NULL;
static osThreadAttr_t 		pm_thread_attr;

//
//  **************************  ADC: code to read ADC values on gADC_LI_ION & gADC_PRIMARY
static volatile uint32_t				adcVoltage;											// set by ADC ISR
static ADC_TypeDef *						Adc = (ADC_TypeDef *) ADC1_BASE;
static ADC_Common_TypeDef *			AdcC = (ADC_Common_TypeDef *) ADC1_COMMON_BASE;

static uint16_t									rawVREFINT;
static uint16_t									calibrated_VREF = 0;

// ADC channels
const int ADC_LI_ION_chan 			= 2;   			// PA2 = ADC1_2		(STM32F412xE/G datasheet pg. 52)
const int ADC_PRIMARY_chan 			= 3;   			// PA3 = ADC1_3
const int ADC_VREFINT_chan 			= 17;				// VREFINT (internal) = ADC1_17 if ADC_CCR.TSVREFE = 1
const int ADC_VBAT_chan 				= 18;   		// VBAT = ADC1_18  if ADC_CCR.VBATE = 1
const int ADC_TEMP_chan 				= 18;   		// TEMP = ADC1_18  if ADC_CCR.TSVREFE = 1
const int gADC_THERM_chan				= 12;				// PC2 = ADC_THERM = ADC1_12

extern void 							ADC_IRQHandler( void );					// override default (weak) ADC_Handler
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
	gConfigOut( gSC_ENABLE );		// 1 to enable SuperCap
	gSet( gSC_ENABLE, 0 );
	
	gConfigADC( gADC_LI_ION );	// rechargable battery voltage level
	gConfigADC( gADC_PRIMARY );	// disposable battery voltage level
	gConfigADC( gADC_THERM );	  // thermistor (R54) by LiIon battery
	
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
	gConfigOut( gEN1V8 );				// 1 to supply 1.8 to codec-- enable TLV74118 regulator	
	gConfigOut( gBOOT1_PDN );		// _: 0 to power down codec-- boot1_pdn on AK4637
	gConfigOut( gPA_EN );				// 1 to power headphone jack
	gSet( gEN_5V, 0 );					// initially codec OFF
	gSet( gEN1V8, 0 );					// initially codec OFF
	gSet( gBOOT1_PDN, 1 );			// initially codec OFF
	gSet( gPA_EN, 0 );					// initially audio external amplifier OFF
	
	tbDelay_ms( 3 );		// wait 3 msec to make sure everything is stable
}
void											startADC( int chan ){						// set up ADC for single conversion on 'chan', then start 
	AdcC->CCR &= ~ADC_CCR_ADCPRE;		// CCR.ADCPRE = 0 => PClk div 2

	// Reset ADC.CR1  		//  0==>  Res=12bit, no watchdogs, no discontinuous, 
	Adc->CR1 = 0;  				//        no auto inject, watchdog, scanning, or interrupts

	// Reset ADC.CR2      					//  no external triggers, injections,
	Adc->CR2 = ADC_CR2_ADON;				//  DMA, or continuous conversion -- only power on
	
	Adc->SQR1 = 0;									// SQR1.L = 0 => 1 conversion 
	Adc->SQR3 = chan;								// chan is 1st (&only) conversion sequence
	
	Adc->SMPR1 = 0x0EFFFFFF;				// SampleTime = 480 for channels 10..18
	Adc->SMPR2 = 0x3FFFFFFF;				// SampleTime = 480 for channels 0..9
	
	Adc->SR  &= ~(ADC_SR_EOC | ADC_SR_OVR);	// Clear regular group conversion flag and overrun flag 
	Adc->CR1 |= ADC_CR1_EOCIE;									// Enable end of conversion interrupt for regular group 
  Adc->CR2 |= ADC_CR2_SWSTART; 								// Start ADC software conversion for regular group 
}
short											readADC( int chan ){						// readADC channel 'chan' value & return calibrated voltage
  startADC( chan );

	// wait for signal from ADC_IRQ on EOC interrupt
	if ( osEventFlagsWait( pwrEvents, PM_ADCDONE, osFlagsWaitAny, 10000 ) != PM_ADCDONE )
			logEvt( "ADC timed out" );

	Adc->CR1 &= ~ADC_CR1_EOCIE;  								// Disable ADC end of conversion interrupt for regular group 
	
	uint32_t adcRaw = Adc->DR;  								// read converted value (also clears SR.EOC)
	if ( calibrated_VREF==0 ) return adcRaw;
	
	adcVoltage = 3300 * adcRaw/calibrated_VREF;			// convert to actual voltage, using internal reference calibration
	return adcVoltage;
} 

static void								EnableADC( bool enable ){				// power up & enable ADC interrupts
	if ( enable ){
		gSet( gADC_ENABLE, 1 );									// power up the external analog sampling circuitry
		NVIC_EnableIRQ( ADC_IRQn );
		RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;			// enable clock for ADC
		Adc->CR2 |= ADC_CR2_ADON; 							// power up ADC
		tbDelay_ms( 1 );
		
		if ( calibrated_VREF==0 ){		// first time-- read value that corresponds to VREF = 1.2V
			AdcC->CCR |= ADC_CCR_TSVREFE;											// enable RefInt on channel 17 -- typically 1.21V (1.18..1.24V)
			rawVREFINT = readADC( ADC_VREFINT_chan );		// got 05D5  on TBookRev2
			AdcC->CCR &= ~ADC_CCR_TSVREFE;					// disable RefInt channel
			logEvtNI( "PwrCheck","rawVRefInt",		rawVREFINT ); 
		}

	} else {
		gSet( gADC_ENABLE, 0 );
		NVIC_DisableIRQ( ADC_IRQn );
		RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;			// disable clock for ADC
		Adc->CR2 &= ~ADC_CR2_ADON; 								// power down ADC
	} 
}
void 											ADC_IRQHandler( void ){ 				// ISR for ADC end of conversion interrupts
  if ( (Adc->SR & ADC_SR_EOC)==0 ) tbErr("ADC no EOC");
	
	osEventFlagsSet( pwrEvents, PM_ADCDONE );					// wakeup powerThread
	Adc->SR = ~(ADC_SR_STRT | ADC_SR_EOC);    // Clear regular group conversion flag 
}

uint16_t									readPVD( void ){								// read PVD level 
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;						// start clocking power control 
	
	PWR->CR |= ( 0 << PWR_CR_PLS_Pos );						// set initial PVD threshold to 2.2V
	PWR->CR |= PWR_CR_PVDE;												// enable Power Voltage Detector
	
	int pvdMV = 2100;							// if < 2200, report 2100 
	for (int i=0; i< 8; i++){
		PWR->CR = (PWR->CR & ~PWR_CR_PLS_Msk) | ( i << PWR_CR_PLS_Pos );
		tbDelay_ms(1);
		
		if ( (PWR->CSR & PWR_CSR_PVDO) != 0 ) break;
		pvdMV = 2200 + i*100;
	}
93	return pvdMV;
}
void											setupRTC( void ){								// init RTC on first boot after data Load
	int cnt = 0;
	RCC->APB1ENR |= ( RCC_APB1ENR_PWREN | RCC_APB1ENR_RTCAPBEN );				// start clocking power control & RTC_APB
	PWR->CSR 	|= PWR_CSR_BRE;											// enable backup power regulator
	while ( (RCC->CSR & PWR_CSR_BRR)==0 ) cnt++;  //  & wait till ready

	PWR->CR 	|= PWR_CR_DBP;			// enable access to Backup Domain to configure RTC
	
	RCC->BDCR |= RCC_BDCR_LSEON;												// enable 32KHz LSE clock
	while ( (RCC->BDCR & RCC_BDCR_LSERDY)==0 ) cnt++; 	// & wait till ready

	// configure RTC  (per RM0402 22.3.5)
	// RCC->BDCR |= RCC_BDCR_PDRST;   				// do backup domain reset first?
	RCC->BDCR |= RCC_BDCR_RTCSEL_0;					// select LSE as RTC source
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;		// un write-protect the RTC

	RTC->ISR |= RTC_ISR_INIT;													// set init mode
	while ( (RTC->ISR & RTC_ISR_INITF)==0 ) cnt++;		// & wait till ready
	
	RTC->PRER  = (256 << RTC_PRER_PREDIV_S_Pos);				// Synchronous prescaler = 256  MUST BE FIRST
	RTC->PRER |= (127 << RTC_PRER_PREDIV_A_Pos);				// THEN asynchronous = 127 for 32768Hz => 1Hz
	
	uint8_t hr = RTC_initTime.hr,   min = RTC_initTime.min, sec = RTC_initTime.sec;
	uint8_t ht = hr/10, hu = hr%10, mnt = min/10, mnu = min%10, st = sec/10, su = sec%10;
	RTC->TR   =  0;			// reset & 24hr format
	RTC->TR 	|= (ht << RTC_TR_HT_Pos) 	| (hu << RTC_TR_HU_Pos);
	RTC->TR 	|= (mnt << RTC_TR_MNT_Pos)| (mnu << RTC_TR_MNU_Pos);
	RTC->TR 	|= (st << RTC_TR_ST_Pos) 	| (su << RTC_TR_SU_Pos);

	uint8_t day = RTC_initTime.day, mon = RTC_initTime.mon, yr = RTC_initTime.year;
	uint8_t dt = day/10, du = day%10, mt = mon/10, mu = mon%10, yt = yr/10, yu = yr%10;
	
	uint8_t anchors[] = { 3, 2, 0, 5 };	// for 19, 20, 21, 22
	uint8_t cen = yr/100, y = yr%100, s4 = anchors[cen-19];
	uint8_t s1 = y/12, s2 = y - s1*12, s3 = s2/4;
	uint8_t wkday = (s1+s2+s3+s4) % 7;
	if (wkday==0) wkday += 7;
	
	RTC->DR   =  wkday << RTC_DR_WDU_Pos;			// set day of week
	RTC->DR 	|= (dt << RTC_DR_DT_Pos) | (du << RTC_DR_DU_Pos);
	RTC->DR 	|= (mt << RTC_DR_MT_Pos) | (mu << RTC_DR_MU_Pos);
	RTC->DR 	|= (yt << RTC_DR_YT_Pos) | (yu << RTC_DR_YU_Pos);

	RTC->CR |= RTC_CR_FMT;
	RTC->ISR 	&= ~RTC_ISR_INIT;												// leave RTC init mode
	
	PWR->CR 	&= ~PWR_CR_DBP;													// disable access to Backup Domain 
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
	
	EnableADC( true );			// enable AnalogEN power & ADC clock & power, & ADC INTQ 
	
	AdcC->CCR |= ADC_CCR_TSVREFE;							// enable Temperature on channel 18
	int TempMV = readADC( ADC_TEMP_chan );   	// ADC_CHANNEL_TEMPSENSOR in hal_adc_ex.h = 16, but RM0402 says ADC_IN18
	AdcC->CCR &= ~ADC_CCR_TSVREFE;						// disable Temp channel
		
	AdcC->CCR |= ADC_CCR_VBATE;								// enable VBAT on channel 18
	int VBatMV = readADC( ADC_VBAT_chan ); 		// got 249 (TBook with no CR2032)
	AdcC->CCR &= ~ADC_CCR_VBATE;							// disable VBAT on channel 18
	
	const int VBAT_PRESENT	= 2000;						// ~2V is where end-of-life drop-off occurs
	if ( (VBatMV > VBAT_PRESENT) && firstBoot )
		setupRTC();

	int LiThermMV = readADC( gADC_THERM_chan );		// 0..3.3V ==> 0..4.2V  -- probably doesn't mean much
	
	int LiMV = readADC( ADC_LI_ION_chan );		// 0..3.3V ==> 0..4.2V  -- probably doesn't mean much

//	LiMV = -0.011 + LiMV*0.021;   // best fit: V = 0.021 * LiMV - 0.011
	
	int PrimaryMV = readADC( ADC_PRIMARY_chan );		// 0..3.3V
	// TBrev2b got 02 wo Primary
	EnableADC( false );		// turn ADC off
	
	logEvtNI( "PwrCheck","Status",		pstat ); 
	logEvtNI( "PwrCheck","TempMV",  	TempMV );
	logEvtNI( "PwrCheck","VBatMV",  	VBatMV );
	logEvtNI( "PwrCheck","LiThermMV",	LiThermMV );
	logEvtNI( "PwrCheck","LiMV",	  	LiMV );
	logEvtNI( "PwrCheck","PrimaryMV", PrimaryMV );
	
//	const int BATT_DROPPING_VOLTAGE	= 2200;			// https://www.powerstream.com/AA-tests.htm discharge graph for AA cells
	const int LOW_BATT_VOLTAGE	= 2000;				// ~2V is where end-of-life drop-off occurs
//	bool shouldChargeCap = 0;  
	switch ( pstat ){
		case CHARGED:		
			sendEvent( BattCharged, 0 ); 
			break;
		
		case CHARGING:		
			sendEvent( BattCharging, LiMV );	
			break;
		
		case NOUSBPWR:
		case NOLITH:		
		case LOWBATT:	// rechargeable is low or missing -- report disposable status
//			shouldChargeCap = PrimaryMV < BATT_DROPPING_VOLTAGE? 1 : 0; 		// charge superCap when disposable battery starts to drop	
//			logEvtS( "PwrCheck Cap? ", shouldChargeCap?"Y":"N" );
			if ( PrimaryMV < LOW_BATT_VOLTAGE )						// goes south quickly from here	
				sendEvent( LowBattery,  PrimaryMV );  
			break;
			
		default:
			break;
	}
//	gSet( gSC_ENABLE, shouldChargeCap );
}
/*   // *************  Handler for PowerFail interrupt
void 											EXTI4_IRQHandler(void){  				// NOPWR ISR
  	if ( __HAL_GPIO_EXTI_GET_IT( NOPWR_pin ) != RESET ){
		__HAL_GPIO_EXTI_CLEAR_IT( NOPWR_pin );  // clear EXTI Pending register
		HAL_NVIC_DisableIRQ( NOPWR_EXTI );		// don't keep getting them

		bool NoPower = HAL_GPIO_ReadPin( NOPWR_port, NOPWR_pin ) == RESET;	// LOW if no power
		if ( NoPower )
			osEventFlagsSet( pwrEvents, PM_NOPWR );		// wakeup powerThread
		else
			HAL_NVIC_EnableIRQ( NOPWR_EXTI );
	}
}
*/


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
