// TBookV2  controlmanager.c
//   Gene Ball  May2018

#include "controlMgr.h"

#include "tbook.h"				// enableMassStorage 
#include "mediaPlyr.h"		// audio operations
#include "inputMgr.h"			// osMsg_TBEvents

// TBook Control State Machine
int 	 					nCSMstates = 0;
csmState *			TBookCSM[ MAX_CSM_STATES ];		// TBook state machine definition

// TBook content 
int							nSubjs = 0;
tbSubject *			TBookSubj[ MAX_SUBJS ];

// TBook configuration variales
TBConfig_t 			TB_Config;


struct {			// CSM state variables
	short 		iCurrSt;	// index of current csmState
	char * 		currStateName;	// str nm of current state
	Event		lastEvent;		// str nm of last event
	char *		lastEventName;	// str nm of last event
	char *		evtNms[ eUNDEF ];
	char *		nxtEvtSt[ eUNDEF ];
	
	short		iPrevSt;	// idx of previous state
	short		iNextSt;	// nextSt from state machine table ( can be overridden by goBack, etc )
	short		iSavedSt[5];	// possible saved states
  
	short		volume;
	short		speed;
  
	short		iSubj;		// index of current Subj
	short		iMsg;		// index of current Msg within Subj

	csmState *  cSt;		// -> TBookCSM[ iCurrSt ]
	
}	TBook;
	
// stm3210E_EVAL board status update on LCD
extern void			 							printHdr( int x, int y, const char *s );
extern void										clearHdr( void );
void 										showCSM(){
	clearHdr();
	
	if (TBook.lastEventName != NULL) printHdr( 2, 1, TBook.lastEventName );
	printHdr( 2, 16, TBook.currStateName );
}


static osTimerId_t  	timers[3]; 	// ShortIdle, LongIdle, Timer
	
// ------------  CSM Action execution
static void 	adjSubj( int adj ){						// adjust current Subj # in TBook
	short nS = TBook.iSubj + adj;
	short numS = nSubjs;
	if ( nS < 0 ) 
		nS = numS-1;
	if ( nS >= numS )
		nS = 0;
	logEvtNI( "changeSubj", "iSubj", nS );
	TBook.iSubj = nS;
}
static void 	adjMsg( int adj ){						// adjust current Msg # in TBook
	short nM = TBook.iMsg + adj; 
	short numM = TBookSubj[ TBook.iSubj ]->NMsgs;
	if ( nM < 0 ) 
		nM = numM-1;
	if ( nM >= numM )
		nM = 0;
	logEvtNI( "changeMsg", "iMsg", nM );
	TBook.iMsg = nM;
}
static void 	playSubjAudio( char *arg ){				// play current Subject: arg must be 'nm', 'pr', or 'msg'
	tbSubject * tbS = TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	char *nm = NULL;
	MsgStats *stats = NULL;
	if ( strcasecmp( arg, "nm" )==0 ){
		nm = tbS->audioName;
	} else if ( strcasecmp( arg, "pr" )==0 ){
		nm = tbS->audioPrompt;
	} else if ( strcasecmp( arg, "msg" )==0 ){
		nm = tbS->msgAudio[ TBook.iMsg ];
		stats = loadStats( tbS->name, TBook.iSubj, TBook.iMsg );	// load stats for message
	}
	buildPath( path, tbS->path, nm, ".wav" ); //".ogg" );
	playAudio( path, stats );
	logEvtNS( "Play", "file", path );
}
static void 	playSysAudio( char *arg ){				// play system file 'arg'
	char path[MAX_PATH];
	buildPath( path, TB_Config.systemAudio, arg, ".wav" ); //".ogg" );
	playAudio( path, NULL );
	logEvtNS( "PlaySys", "file", arg );
}
static void		startRecAudio( char *arg ){
	tbSubject * tbS = TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	buildPath( path, TB_Config.systemAudio, arg, ".wav" ); //".ogg" );
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".wav" ); //".ogg" );		// build file path for next audio msg for S<iS>M<iM>
	logEvtNS( "startRec", "file", fNm );
	MsgStats *stats = loadStats( tbS->name, TBook.iSubj, TBook.iMsg );	// load stats for message
	recordAudio( fNm, stats );
}
static void		saveWriteMsg( char *txt ){				// save 'txt' in Msg file
	tbSubject * tbS = TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".txt" );		// build file path for next text msg for S<iS>M<iM>
	logEvtNSNS( "writeMsg", "file", fNm, "msg", txt );
	FILE* outFP = fopen( fNm, "w" );
	fputs( txt, outFP );
	fclose( outFP ); 
}
static void 	USBmode( bool start ){					// start (or stop) USB storage mode
	if ( start ){
		logEvt( "enterUSB" );
		logPowerDown();				// flush & shut down logs
		enableMassStorage( "M0:", NULL, NULL, NULL );
	} else {	
		logEvt( "exitUSB" );
		disableMassStorage();
		logPowerUp();	
	} 
}
static int		stIdx( int iSt ){
	if ( iSt < 0 || iSt >= nCSMstates )
		tbErr("invalid iSt");
	return iSt;
}
static void 	doAction( Action act, char *arg, int iarg ){	// execute one csmAction
	logEvtNSNS( "Action", "nm", actionNm(act), "arg", arg );
	if (isMassStorageEnabled()){		// if USB MassStorage running: ignore actions referencing files
		switch ( act ){
			case playSys:			
			case playSubj:			
			case startRec:
			case pausePlay:		// pause/play all share
			case resumePlay:		
			case pauseRec:	
			case resumeRec:
			case finishRec:
			case writeMsg:
			case stopPlay:
				return;
			
			default: break;
		}
	}
	switch ( act ){
		case LED:
			ledFg( arg );	
			break;
		case bgLED:
			ledBg( arg );	
			break;
		case playSys:			
			playSysAudio( arg );
			break;
		case playSubj:			
			playSubjAudio( arg );
			break;
		case startRec:
			startRecAudio( arg );
			break;
		case pausePlay:		// pause/play all share
		case resumePlay:		
		case pauseRec:	
		case resumeRec:
			pauseResume();
			break;
		case finishRec:
			stopRecording(); 
			logEvt( "finishRec" );
			break;
		case writeMsg:
			saveWriteMsg( arg );
			break;
		case stopPlay:
			stop(); 
			logEvt( "stopPlay" );
			break;
		case volAdj:			
			adjVolume( iarg );	
			break;
		case spdAdj:			
			adjSpeed( iarg );
			break;
		case posAdj:			
			adjPlayPosition( iarg );
			break;
		case startUSB:
			USBmode( true );
			break;
		case endUSB:
			USBmode( false );
			break; 
		case subjAdj:			
			adjSubj( iarg );	
			break;
		case msgAdj:			
			adjMsg( iarg );		
			break;
		case goPrevSt:			
			TBook.iCurrSt = TBook.iNextSt = stIdx( TBook.iPrevSt );		// return to prevSt without transition
			break;
		case saveSt:
			if ( iarg > 4 ) iarg = 4;
			TBook.iSavedSt[ iarg ] = stIdx( TBook.iPrevSt );
			break;
		case goSavedSt:
			if ( iarg > 4 ) iarg = 4;
			TBook.iNextSt = stIdx( TBook.iSavedSt[ iarg ] );
			break;
		case setTimer:
			osTimerStart( timers[2], iarg );
			break;
		case showCharge:
			ledFg( "G3_G3_G3_R3_R3_" );	
			break;
		case powerDown:		
			powerDownTBook();
			//TODO:  powermanager powerDown
			break;
		default:				break; 
	}
}

// ------------- interpret TBook CSM 
static void		changeCSMstate( short nSt, short lastEvtTyp ){
	if (nSt==TBook.iCurrSt)
		logEvtNSNS( "No-op_evt", "state",TBook.cSt->nm, "evt", eventNm( (Event)lastEvtTyp) );
	while ( nSt != TBook.iCurrSt ){
		TBook.iPrevSt = stIdx( TBook.iCurrSt );
		TBook.iCurrSt = stIdx( nSt );
		
		csmState *st = TBookCSM[ TBook.iCurrSt ];
		TBook.cSt = st;
		TBook.currStateName = st->nm;	//DEBUG -- update currSt string
		showCSM();
		
		for ( short e=(int)eNull; e<(int)eUNDEF; e++ ){	//DEBUG -- update nextSt strings
			TBook.evtNms[ e ] = eventNm( (Event)e );
			short iState = st->evtNxtState[ e ];
			TBook.nxtEvtSt[ e ] = TBookCSM[ iState ]->nm;
		}
		
		TBook.iNextSt = TBook.iCurrSt;   // stay here unless something happens
		
		char Actions[200]; Actions[0] = 0;
		char Act[80];
		int nActs = st->nActions;
		for ( short i=0; i<nActs; i++ ){
			Action act = st->Actions[i].act;
			char * arg = st->Actions[i].arg;
			if (arg==NULL) arg = "";
			sprintf(Act, " %s:%s", actionNm(act), arg );
			strcat(Actions, Act);
			int iarg = arg[0]=='-' || isdigit( arg[0] )? atoi( arg ) : 0;
			doAction( act, arg, iarg );
		}
		logEvtNSNSNS( "CSM_st", "ev", eventNm((Event)lastEvtTyp), "nm", st->nm, "act", Actions );
		
		nSt = TBook.iNextSt;		// in case Action set it
	}
}
static void		tbTimer( void * eNum ){
	switch ((int)eNum){
		case 0:		sendEvent( ShortIdle,  0 );  	break;
		case 1:		sendEvent( LongIdle,  0 );  	break;
		case 2:		sendEvent( Timer,  0 );  		break;
	}
}
static void 	executeCSM( void ){						// execute TBook control state machine
	TB_Event *evt;
	osStatus_t status;

	for ( int it=0; it<3; it++ ){
		timers[it] = osTimerNew( tbTimer, osTimerOnce, (void *)it, NULL );
		if ( timers[it] == NULL ) 
			tbErr( "timer not alloc'd" );
	}
	
	TBook.volume = TB_Config.default_volume;
	TBook.speed = TB_Config.default_speed;
	TBook.iSubj = 0;
	TBook.iMsg = 0;

	// set initialState & do actions
	TBook.iCurrSt = 1;		// so initState (which has been assigned to 0) will be different
	changeCSMstate( stIdx( TB_Config.initState ), 0 );
	
	while (true){
		evt = NULL;
	    status = osMessageQueueGet( osMsg_TBEvents, &evt, NULL, osWaitForever );  // wait for next TB_Event
		if (status != osOK) 
			tbErr(" EvtQGet error");
		showCSM();
		char * eNm = eventNm( evt->typ );
		logEvtNSNI( "csmEvent", "typ", eNm, "dnMS", evt->arg );
		TBook.lastEvent = evt->typ;
		TBook.lastEventName = eNm;
 
		switch ( evt->typ ){
			case AudioDone:
				osTimerStart( timers[0], TB_Config.shortIdleMS );
				osTimerStart( timers[1], TB_Config.longIdleMS );
				break;
			case ShortIdle:
				break;
			default:
				osTimerStop( timers[0] );
				osTimerStop( timers[1] );
				break;
//			case starPlus:  
//				USBmode( true );
//				break;
		}
		short lastEvtTyp = evt->typ;
		short nSt = stIdx( TBook.cSt->evtNxtState[ evt->typ ]);
		osMemoryPoolFree( TBEvent_pool, evt );
		changeCSMstate( nSt, lastEvtTyp );	// only changes if different
	}
}
static void 	runCSM(  ){								// CSM thread -- called by tbook after everything is initialized
	initTknTable();
	readControlDef( );
	readContent( );
	executeCSM();
}


// debug power signals
const int numPwrSig = 7;
GPIO_ID pwrID[] = { gADC_ENABLE, gSC_ENABLE, g3V3_SW_EN, gEN_5V, gNLEN01V8, gBOOT1_PDN, gPA_EN };
								//				PA2		PD1		PD6			PD4		PD5		PB2			PD3
const char * pwrNm[] = { "ADC", "SC", "3V3", "5V", "1V8", "PDN", "PA" };
char pwrVal[] = { '-', '-', '-', '-', '-', '-', '-', 0 };	// => '+' if signal is active
void chkPwrState(){
	for ( int i=0; i<numPwrSig; i++ ){
		bool active = gOutVal( pwrID[ i ] );
		//dbgLog( " %s:%d \n", gpio_def[ pwrID[i] ].signal, active );
		pwrVal[i] = (active? '+' : '-' );
	}
}
void showSig( char * enm, int idx ){	// 	8/4/2/1 bits 		R/g															2,2,2,2_2  	3,3,3,3_8
	chkPwrState();
	char *pin = gpio_def[ pwrID[idx] ].signal;
	pwrVal[idx] = (pwrVal[idx]=='-'? '_' : '*' );
	dbgLog( "%s: %s %s %s\n", enm, pwrNm[idx], pin, pwrVal );  // e.g. "05T: P1 SC PD1=+ -+++----"
	const char * codeFmt = "%c4_2%c4_2%c4_2%c4_5%c5_5";   // 8_4_2_1_VV__ == bits either r or g
	char fg[30];
	sprintf(fg, codeFmt, (idx&8)?'r':'g', (idx&4)?'r':'g', (idx&2)?'r':'g', (idx&1)?'r':'g', pwrVal[idx]=='+'? 'R':'G' );
	ledFg( fg );
}
void dbgTgl( char * enm,  int i ){
	chkPwrState();
	char v = pwrVal[i];
	v = ( v=='-'? '+' : '-' );
	pwrVal[i] = v;
	GPIO_ID id = pwrID[i];
	gSet( id, v=='+'? 1 : 0 );
	showSig( enm, i );
}


static void 	controlTest(  ){					// CSM test procedure
	TB_Event *evt;
	osStatus_t status;
	MediaState mediaStatus;
	int audioSeconds = 0;
	TBook.iCurrSt = stIdx( TB_Config.initState );
	TBook.cSt = TBookCSM[ TBook.iCurrSt ];
	TBook.currStateName = TBook.cSt->nm;	//DEBUG -- update currSt string

	dbgLog( "CTest: \n" );
	while (true) {
	  status = osMessageQueueGet( osMsg_TBEvents, &evt, NULL, osWaitForever );  // wait for next TB_Event
		if (status != osOK) 
			tbErr(" EvtQGet error");
		
	  TBook.lastEventName = eventNm( evt->typ );
		//dbgLog( " %s ", eventNm( evt->typ ));
		
		if (isMassStorageEnabled()){		// if USB MassStorage running: ignore events
			if ( evt->typ==starHome || evt->typ==starCircle )
				USBmode( false );
			else
				dbgLog("starHome to exit USB \n" );
		} else {
			tbSubject * tbS = TBookSubj[ TBook.iSubj ];
			switch (evt->typ){
				case Tree:
					playSysAudio( "welcome" );
					dbgLog( "PlaySys welcome...\n" );
					logEvtNS( "Play", "file", "welcome" );
					break; 
				case Pot:
					dbgLog( "Playing msg...\n" );
					playSubjAudio( "msg" );
					logEvt( "PlaySubj" );
					TBook.iMsg++;
					if ( TBook.iMsg >= tbS->NMsgs ) TBook.iMsg = 0;
					break; 
				case Plus:
					adjVolume( 10 );
					logEvt( "Louder" );
					break;
				case Minus:
					adjVolume( -10 );
					logEvt( "Softer" );
					break;
				case Lhand:
					adjPlayPosition( -2 );
					logEvt( "JumpBack2" );
					break;
				case starLhand:
					adjPlayPosition( 2 );
					logEvt( "JumpFwd2" );
					break;
				case Circle:
					showSig("Pwr", 2 );
					break;
				
				case Star:
					mediaStatus = getStatus();
					if ( mediaStatus==Playing ){
						pauseResume();
						//stop( );
						audioSeconds = playPosition();
						logEvtNI( "stopPlay", "pos", audioSeconds );
					} else if ( mediaStatus==Recording ){
						pauseResume();
						logEvtNI( "stopRecord", "pos", 0 );
					}
					break;
					
				case Table:		// Record
					mediaStatus = getStatus();
					startRecAudio( NULL );
					logEvt( "Record" );
					break;
				case starPlus:			
					stopRecording( );
					break;
				
				case starCircle:
				case starHome:
					dbgLog( "going to USB mass-storage mode \n");
					USBmode( true );
					break;
					
				case starMinus: 
					executeCSM();
					break;
				case starTable:
					break;
				
				case FirmwareUpdate:
					dbgLog( "rebooting to system bootloader for DFU... \n" );
					ledFg( "R5_5 R5_3 R5_1 R5_1"); 
					break;
				default:
					dbgLog( "evt: %d %s \n", evt->typ, eventNm( evt->typ ) );
			}
		}
		osMemoryPoolFree( TBEvent_pool, evt );
	}
}

// Debug LED patterns
const int  mxClr = 8, mxSpd = 4;
const char* clrs[ mxClr+1 ] = { "Rr","Gg","Oo", "Rg", "Gr", "RGR", "RGO", "GRG", "GRO" };
/*const char * LEDtsts[] = { 	// distinctive repeating LED patterns
		"R10r10!", 		// Minus__	pulsing Red .5Hz	
		"G5g5!",  		// Tree__		pulsing Green 1Hz
		"O3o2!",   		// Circle__	pulsing Orange 2Hz
		"g5R2_3!",   	// LHand__	grn/Red flash 1Hz
		"G2r3!", 			// Pot__ 		alt Grn/Red 2Hz
		"O3_7!", 			// RHand__	Orange flash 1Hz
		"r2G2O2_4!", 	// Table__	Red/Green/Orange flash 1Hz
		"G4r4G4_8!",	// Star  		Grn/red/Grn flash .5Hz  	
		"Rr!"					// Star__  	pulsing Red 5Hz
}; */
void ledSig( bool flash, int iClrs, int iSpd ){  // flash/pulse,  clrs 0..8, spd 0..4
	// 																	5Hz	    3Hz	   2Hz			1Hz					.5Hz
	//								period in .1sec:	 2				3			5		 		10					20	
	// 1 clr pulse dim/bright  	R/G/O		1,1			2,1		 3,2	 		5,5	 				10,10
	// 1 clr flash color/off    R/G/O  	1,1			2_1		 3_2	 		3_7	  			5_15
	// 2 clr pulse							RG/GR  	1,1		  2,1		 3,2	 		5,5	 				0,10	
	// 2 clr flash		 					RG/GR   				1,1_1	 2,2_1 		3,3_4 			5,5,10
	// 3 clr pulse							RGO/GRO 				1,1,1	 1,2,2 		3,3,4 			7,7,8 
	// 3 clr flash							rGr/GrG/rgO 					 1,1,1_2	2,2,2_4			4,4,4_8
	const char* clrs[ mxClr+1 ] = { "Rr","Gg","Oo", "Rg", "Gr", "RGR", "RGO", "GRG", "GRO" };
	
	//  speed/period:				 sHalf=20				sOne=10					sTwo=5					sThree=3			sFive=2
	const char* f1Fmt[] = { "%c5_15!", 			"%c3_7!", 			"%c3_2!", 			"%c2_1!", 		"%c1_1!" };
	const char* p2Fmt[] = { "%c10%c10!", 		"%c5%c5!", 			"%c3%c2!", 			"%c2%c1!", 		"%c1%c1!" };
	const char* f2Fmt[] = { "%c5%c5_10!", 	"%c3%c3_4!", 		"%c1%c2_2!", 		"%c1%c1_1!", 	"" };
	const char* p3Fmt[] = { "%c7%c7%c8!", 	"%c3%c3%c4!", 	"%c1%c2%c2!", 	"%c1%c1%c1!", "" };
	const char* f3Fmt[] = { "%c4%c4%c4_8!", "%c2%c2%c2_4!", "%c1%c1%c1_2!", NULL, 				NULL };

	iClrs =  iClrs < 0? 0 : (iClrs > mxClr? mxClr : iClrs);
	const char* clr = clrs[ iClrs ];

	char fg[20];
  fg[0] = 0;
	iSpd = iSpd < 0? 0 : (iSpd > mxSpd? mxSpd : iSpd);
	switch ( strlen( clr ) ){
		default:
		case 0: break;
		case 1: sprintf( fg, f1Fmt[ iSpd ], clr[0] ); break;
		case 2: sprintf( fg, flash? f2Fmt[ iSpd ] : p2Fmt[ iSpd ], clr[0], clr[1] ); break;
		case 3: sprintf( fg, flash? f3Fmt[ iSpd ] : p3Fmt[ iSpd ], clr[0], clr[1], clr[2] ); break;
	}
	if ( strlen(fg) > 0 )
		ledFg( fg );
}

static int cClr = 0, cSpd = 0;
bool cFlash = false;

void dbgLED( char * enm, int i ){		// 0: flash/pulse, 1: nxt colors, 2: nxt speed
	if ( i==0 ) cFlash = !cFlash;
	if ( i==1 ) cClr = cClr==mxClr? 0 : cClr+1; 
	if ( i==2 ) cSpd = cSpd==mxSpd? 0 : cSpd+1;

	dbgLog( "%s: %s%s@%d \n", enm, cFlash? "F_":"P:", clrs[cClr], cSpd  );
	ledSig( cFlash, cClr, cSpd );
}
		
char * eNm[] = {	
	"00N","01H", "02C", "03+", "04-", "05T", "06L", "07R", "08P", "09S", "10t",
				"11_H","12_C","13_+","14_-","15_T","16_L","17_R","18_P","19_S","20_t",
				"21*H","22*C","23*+","24*-","25*T","26*L","27*R","28*P","29*S","30*t",
				"31aD","32sI","33LI","34lB","35Bc","36BC","37FU","38Tm","39ak","40ud" 
};

static void 	debugTest(  ){						// hardware test without FS
	TB_Event *evt;
	osStatus_t status;
	
	dbgLog( "TB_Dbg:  \n" );
	while (true) {
	  status = osMessageQueueGet( osMsg_TBEvents, &evt, NULL, osWaitForever );  // wait for next TB_Event
		if (status != osOK) 
			tbErr(" EvtQGet error");
		
		FILE * f = NULL;
		char str[30] = {0};
		int stat, stat1, stat2;
		char * enm = eNm[ evt->typ ];
		//  Plus		LED			Home
		// Minus		Tree		 Circle
		//   LHand	Pot	 RHand
		// Star    Table
		switch (evt->typ){
				case Home:  		dbgLog( "%s: finit() *H='M0:' _H='F0' \n", enm );	break;
				case Home__: 		dbgLog( "%s: finit('F0:') = %d \n", enm, finit("F0:") ); break;
				case starHome: 	dbgLog( "%s: finit('M0:') = %d \n", enm, finit("M0:") ); break;

				case Plus:  		dbgLog( "%s:  *+: WFI() \n", enm );	break;
			//	case Plus__: 		dbgLog( "%s:  \n", enm );  break;
				case starPlus: 	dbgLog( "%s: WFI() \n", enm ); enableSleep(); break;
				
				case Minus:			showSig( enm, 0 ); 	break;
				case Minus__:		dbgLED(  enm, 0 ); 	break;
				case starMinus:	dbgTgl(  enm, 0 ); 	break;
			
				case Tree:			showSig( enm, 1 ); 	break;
				case Tree__:		dbgLED(  enm, 1 ); 	break;
				case starTree:	dbgTgl(  enm, 1 ); 	break;

				case Circle:		showSig( enm, 2 ); 	break;
				case Circle__:	dbgLED(  enm, 2 ); 	break;
				case starCircle: dbgTgl( enm, 2 ); 	break;

				case Lhand:			showSig( enm, 3 ); 	break;
				case Lhand__:		
					stat = finit( "F0:" );
				  stat1 = fmount( "F0:" );
					f = fopen( "F0:test.txt", "a" ); 
					fprintf( f, "test\n" );
					stat2 = fclose( f );
					dbgLog( "%s: Wr %d %d %d %d \n", enm, stat, stat1, f==NULL?1:0, stat2 );
					break;
				case starLhand:	dbgTgl(  enm, 3 ); 	break;

				case Pot:				showSig( enm, 4 ); 	break;
				case Pot__:			dbgLog( "%s: welcome \n", enm ); playSysAudio( "welcome" );	break;
				case starPot:		dbgTgl(  enm, 4 ); 	break;

				case Rhand:			showSig( enm, 5 ); 	break;
				case Rhand__:		
					f = fopen( "F0:test.txt", "r" ); 
					stat = fscanf( f, "%s", str );
					stat2 = fclose( f );
					dbgLog( "%s: Rd %s %d %d %s \n", enm, f==NULL?"null":"ok", stat, stat2, str );
					break;
				case starRhand:	dbgTgl(  enm, 5 ); 	break;
				
				case Table:			showSig( enm, 6 ); 	break;
			//	case Table__:		dbgLED(  enm, 6 ); 	break;
				case starTable:	dbgTgl(  enm, 6 ); 	break;

			//	case Star:			dbgLED( enm, 7 );	break;
			//	case Star__:		dbgLED( enm, 8 );	break;
			
				
				case FirmwareUpdate:	dbgLog( "*%s:  DFU \n", enm );			break;
				default:							dbgLog( "%s: ?? \n", enm );	  break;
			}
		osMemoryPoolFree( TBEvent_pool, evt );
	}
}

void 					initControlManager( void ){				// initialize control manager 	
	// init to odd values so changes are visible
	TB_Config.default_volume = 3;
	TB_Config.default_speed = 3;
	TB_Config.powerCheckMS = 10000;				// used by powermanager.c
	TB_Config.shortIdleMS = 3000;
	TB_Config.longIdleMS = 11000;
	TB_Config.systemAudio = "M0:/system/aud/";			// path to system audio files
	TB_Config.minShortPressMS = 30;				// used by inputmanager.c
	TB_Config.minLongPressMS = 900;				// used by inputmanager.c
	
	initTknTable();
	if ( TBDataOK ) {
		readControlDef( );
		readContent( );
		TBook.iSubj = 0;
		TBook.iMsg = 1;
		controlTest();
		runCSM( );	
	} else if ( FileSysOK ) {		// FS but no data, go into USB mode
		logEvt( "NoFS USBmode" );
		USBmode( true );
	} else {  // no FileSystem
		debugTest();
	}
}
// contentmanager.cpp 
