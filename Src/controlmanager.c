// TBookV2  controlmanager.c
//   Gene Ball  May2018

#include "controlMgr.h"

#include "tbook.h"				// enableMassStorage 
#include "mediaPlyr.h"		// audio operations
#include "inputMgr.h"			// osMsg_TBEvents
const char *  			fgPlaying 			= "G!";
const char *  			fgPlayPaused		= "G2_3!";
const char *  			fgRecording			= "R!";
const char *  			fgRecordPaused	= "R2_3!";
const char *  			fgUSB_MSC				= "O5o5!";
const char *  			fgTB_ERR				= "R8_2 R8_2 R8_20!";

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
//	clearHdr();
	
//	if (TBook.lastEventName != NULL) printHdr( 2, 1, TBook.lastEventName );
//	printHdr( 2, 16, TBook.currStateName );
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
	resetAudio();
	if ( strcasecmp( arg, "nm" )==0 ){
		nm = tbS->audioName;
		logEvtNSNS( "PlayNm", "Subj", tbS->name, "nm", nm ); 
	} else if ( strcasecmp( arg, "pr" )==0 ){
		nm = tbS->audioPrompt;
		logEvtNSNS( "Play", "Subj", tbS->name, "pr", nm ); 
	} else if ( strcasecmp( arg, "msg" )==0 ){
		nm = tbS->msgAudio[ TBook.iMsg ];
		logEvtNSNI( "Play", "Subj", tbS->name, "iM", TBook.iMsg ); //, "aud", nm ); 
		stats = loadStats( tbS->name, TBook.iSubj, TBook.iMsg );	// load stats for message
	}
	buildPath( path, tbS->path, nm, ".wav" ); //".ogg" );
	playAudio( path, stats );
}
static void 	playSysAudio( char *arg ){				// play system file 'arg'
	char path[MAX_PATH];
	resetAudio();
	buildPath( path, TB_Config.systemAudio, arg, ".wav" ); //".ogg" );
	playAudio( path, NULL );
	logEvtNS( "PlaySys", "file", arg );
}
static void		startRecAudio( char *arg ){
	resetAudio();
	tbSubject * tbS = TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	buildPath( path, TB_Config.systemAudio, arg, ".wav" ); //".ogg" );
	int mCnt = 0;
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".wav", &mCnt ); //".ogg" );		// build file path for next audio msg for S<iS>M<iM>
	logEvtNSNINI( "Record", "Subj", tbS->name, "iM", TBook.iMsg, "cnt", mCnt );
	MsgStats *stats = loadStats( tbS->name, TBook.iSubj, TBook.iMsg );	// load stats for message
	recordAudio( fNm, stats );
}
static void		saveWriteMsg( char *txt ){				// save 'txt' in Msg file
	tbSubject * tbS = TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	int mCnt = 0;
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".txt", &mCnt );		// build file path for next text msg for S<iS>M<iM>
	FILE* outFP = fopen( fNm, "w" );
	int nch = fprintf( outFP, "%s\n", txt );
	int err = fclose( outFP ); 
	dbgEvt( TB_wrMsgFile, nch, err, 0, 0 );
	logEvtNSNININS( "writeMsg", "Subj", tbS->name, "iM", TBook.iMsg, "cnt", mCnt, "msg", txt );
}
static void 	USBmode( bool start ){					// start (or stop) USB storage mode
	if ( start ){
		logEvt( "enterUSB" );
		logPowerDown();				// flush & shut down logs
		enableMassStorage( "M0:", NULL, NULL, NULL );
	} else {	
		disableMassStorage();
		logEvt( "exitUSB" );
		ledFg( "_" );
		logPowerUp( false );	
	} 
}
static int		stIdx( int iSt ){
	if ( iSt < 0 || iSt >= nCSMstates )
		tbErr("invalid iSt");
	return iSt;
}
static void 	controlTest( void );  //DEBUG
static void 	doAction( Action act, char *arg, int iarg ){	// execute one csmAction
	dbgEvt( TB_csmDoAct, act, arg[0],arg[1],iarg );
	logEvtNSNS( "Action", "nm", actionNm(act), "arg", arg ); //DEBUG
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
			logEvt( "stopRec" );
			break;
		case writeMsg:
			saveWriteMsg( arg );
			break;
		case stopPlay:
			stopPlayback(); 
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
			if ( !haveUSBpower() )
				ledFg( "_3R3_3R3_3R3_5" ); // no USB cable!
			else
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
			showBattCharge();
			break;
		case powerDown:		
			powerDownTBook();
			break;
	  case sysBoot:
			controlTest();
			break;
		default:				break; 
	}
}

// ------------- interpret TBook CSM 
static void		changeCSMstate( short nSt, short lastEvtTyp ){
	dbgEvt( TB_csmChSt, nSt, 0,0,0 );
	if (nSt==TBook.iCurrSt)
		logEvtNSNS( "No-op_evt", "state",TBook.cSt->nm, "evt", eventNm( (Event)lastEvtTyp) ); //DEBUG
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
static void 	eventTest(  ){					// report Events until DFU (pot table)
	TB_Event *evt;
	osStatus_t status;

	dbgLog( "EvtTst starPlus\n" );
	while (true) {
	  status = osMessageQueueGet( osMsg_TBEvents, &evt, NULL, osWaitForever );  // wait for next TB_Event
		if (status != osOK) 
			tbErr(" EvtQGet error");

	  TBook.lastEventName = eventNm( evt->typ );
		dbgLog( " %s \n", eventNm( evt->typ ));
		dbgEvt( TB_csmEvt, evt->typ, tbTimeStamp(), 0, 0);
		logEvtNS( "evtTest", "nm",eventNm( evt->typ) );

		if ( evt->typ == Tree ) 
			osTimerStart( timers[0], TB_Config.shortIdleMS );
		if ( evt->typ == Pot ) 
			osTimerStart( timers[1], TB_Config.longIdleMS );
		if ( evt->typ == Table ) 
			osTimerStart( timers[2], 20000 );	// 20 sec

		if ( evt->typ == starPlus ) return;
	}
}

void measureSystick( void );
static void 	controlTest(  ){					// CSM test procedure
	TB_Event *evt;
	osStatus_t status;
	TBook.iCurrSt = stIdx( TB_Config.initState );
	TBook.cSt = TBookCSM[ TBook.iCurrSt ];
	TBook.currStateName = TBook.cSt->nm;	//DEBUG -- update currSt string
  bool playforever = false;
	MediaState audSt;
	
	dbgLog( "CTest: \n" );
	while (true) {
	  status = osMessageQueueGet( osMsg_TBEvents, &evt, NULL, osWaitForever );  // wait for next TB_Event
		if (status != osOK) 
			tbErr(" EvtQGet error");
		
	  TBook.lastEventName = eventNm( evt->typ );
		//dbgLog( " %s ", eventNm( evt->typ ));
		dbgEvt( TB_csmEvt, evt->typ, 0, 0, 0);
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
					break; 
				case AudioDone:
				case starPot: 
				case Pot:
					if ( evt->typ==AudioDone && !playforever ) break;
					if ( evt->typ==starPot ) playforever = !playforever;
				
					dbgLog( "Playing msg...\n" );
					TBook.iMsg++;
					if ( TBook.iMsg >= tbS->NMsgs ) TBook.iMsg = 0;
					playSubjAudio( "msg" );
					break; 
				case Circle: 
					TBook.iSubj++;
					if ( TBook.iSubj >= nSubjs  ) TBook.iSubj = 0;
					tbS = TBookSubj[ TBook.iSubj ];
				  TBook.iMsg = 0;
					playSubjAudio( "nm" );
					break;
				case Plus:
					adjVolume( 1 );
					break;
				case Minus:
					adjVolume( -1 );
					break;
				case Lhand:
					adjPlayPosition( -2 );
					logEvt( "JumpBack2" );
					break;
				case starLhand:
					adjPlayPosition( 2 );
					logEvt( "JumpFwd2" );
					break;
				case starRhand:
					osTimerStart( timers[1], TB_Config.longIdleMS );
					playSysAudio( "faster" );
					break;
				case LongIdle:
					playSysAudio( "slower" );
					break;
				
				case Star:
					audSt = audGetState();
					if ( audSt==Recording || audSt==Playing )
						pauseResume();
					break;
					
				case Table:		// Record
					audSt = audGetState();
					if ( audSt==Recording )
						stopRecording( );
					else 
						startRecAudio( NULL );
					break;
				case starTable:
					audSt = audGetState();
					if ( audSt==Recording )
						stopRecording( );
					break;
				
				case starCircle:
					saveWriteMsg( "Clean power down" );
					ledFg("R_3R_3R_9R_3R_9R_3"); //  R R R   R R  R
					tbDelay_ms( 15000 );
					powerDownTBook();
				  break;
				
				case starHome:
					dbgLog( "going to USB mass-storage mode \n");
					USBmode( true );
					break;
					
				case starMinus: 
					//ledFg( "R8_5!");
					//measureSystick();
					ledBg( "_" );
					ledFg( "_" );
					//ak_SetVolume(99);		//Debug test +db volume settings
					executeCSM();
					break;
				
				case starPlus:			
					eventTest(  );
					break;
				
				case FirmwareUpdate:   // pot table
					dbgLog( "rebooting to system bootloader for DFU... \n" );
					ledFg( "R3_5 R3_3 R3_2 R3_1 R3"); 
					tbDelay_ms( 3300 );
					RebootToDFU(  );
					break;
				default:
					dbgLog( "evt: %d %s \n", evt->typ, eventNm( evt->typ ) );
			}
		}
		osMemoryPoolFree( TBEvent_pool, evt );
	}
}


void 					initControlManager( void ){				// initialize control manager 	
	// init to odd values so changes are visible
	TB_Config.default_volume = 8; 
	TB_Config.default_speed = 3;
	TB_Config.powerCheckMS = 10000;				// set by setPowerCheckTimer()
	TB_Config.shortIdleMS = 3000;
	TB_Config.longIdleMS = 11000;
	TB_Config.systemAudio = "M0:/system/aud/";			// path to system audio files
	TB_Config.minShortPressMS = 30;				// used by inputmanager.c
	TB_Config.minLongPressMS = 900;				// used by inputmanager.c
	TB_Config.fgPlaying 			= (char *) fgPlaying;
	TB_Config.fgPlayPaused		= (char *) fgPlayPaused;
	TB_Config.fgRecording			= (char *) fgRecording;
	TB_Config.fgRecordPaused	= (char *) fgRecordPaused;
	TB_Config.fgUSB_MSC				= (char *) fgUSB_MSC;
	TB_Config.fgTB_ERR				= (char *) fgTB_ERR;

	EventRecorderEnable(  evrEA, 	  		TB_no, TBsai_no ); 	// TB, TBaud, TBsai  
	EventRecorderDisable( evrAOD, 			EvtFsCore_No,   EvtFsMcSPI_No );  //FileSys library 
	EventRecorderDisable( evrAOD, 	 		EvtUsbdCore_No, EvtUsbdEnd_No ); 	//USB library 
	
	initTknTable();
	if ( TBDataOK ) {
		readControlDef( );		// reads TB_Config settings
		readContent( );
		TBook.iSubj = 0;
		TBook.iMsg = 1;
		
		setPowerCheckTimer( TB_Config.powerCheckMS );		// adjust power timer to input configuration
		setVolume( TB_Config.default_volume );					// set initial volume
		
		for ( int it=0; it<3; it++ ){
			timers[it] = osTimerNew( tbTimer, osTimerOnce, (void *)it, NULL );
			if ( timers[it] == NULL ) 
				tbErr( "timer not alloc'd" );
		}
		controlTest();  //DEBUG
		//executeCSM();	
		
	} else if ( FileSysOK ) {		// FS but no data, go into USB mode
		logEvt( "NoFS USBmode" );
		USBmode( true );
	} else {  // no FileSystem
		eventTest();
	}
}
// contentmanager.cpp 
