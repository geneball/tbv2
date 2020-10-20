// TBookV2  controlmanager.c
//   Gene Ball  May2018

#include "controlMgr.h"

#include "tbook.h"				// enableMassStorage 
#include "mediaPlyr.h"		// audio operations
#include "inputMgr.h"			// osMsg_TBEvents
#include "fileOps.h"			// decodeAudio

const char *  			bgPulse		 			= "_49G";
const char *  			fgPlaying 			= "G!";
const char *  			fgPlayPaused		= "G2_3!";
const char *  			fgRecording			= "R!";
const char *  			fgRecordPaused	= "R2_3!";
const char *				fgSavingRec			= "O!";
const char *				fgSaveRec				= "G3_3G3";
const char *				fgCancelRec			= "R3_3R3";
const char *  			fgUSB_MSC				= "O5o5!";
const char *  			fgTB_Error			= "R8_2R8_2R8_20!";		// repeat: 4.8sec
const char *  			fgNoUSBcable		= "_3R3_3R3_3R3_5!";  // repeat: 2.3sec
const char *				fgUSBconnect		= "G5g5!";
const char *  			fgPowerDown			= "G_3G_3G_9G_3G_9G_3";  // length: 3.5sec
const char *  			fgDFU						= "O3_5O3_3O3_2O3_1O3";  // length: 2.6sec

// TBook Control State Machine
int 	 								nCSMstates = 0;
csmState *						TBookCSM[ MAX_CSM_STATES ];		// TBook state machine definition

// TBook content packages
int										nPackages = 0;								// number of packages found
TBPackage_t *					TBPackage[ MAX_PKGS ];				// package info
int										iPkg = 0;											// package index 
TBPackage_t * 				TBPkg;												// content package in use

// TBook configuration variales
TBConfig_t 						TB_Config;

struct {							// CSM state variables
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
	

static osTimerId_t  	timers[3]; 	// ShortIdle, LongIdle, Timer
	
// ------------  CSM Action execution
static void 					adjSubj( int adj ){								// adjust current Subj # in TBook
	short nS = TBook.iSubj + adj;
	short numS = TBPkg->nSubjs;
	if ( nS < 0 ) 
		nS = numS-1;
	if ( nS >= numS )
		nS = 0;
	logEvtNI( "changeSubj", "iSubj", nS );
	TBook.iSubj = nS;
}
static void 					adjMsg( int adj ){								// adjust current Msg # in TBook
	short nM = TBook.iMsg + adj; 
	short numM = TBPkg->TBookSubj[ TBook.iSubj ]->NMsgs;
	if ( nM < 0 ) 
		nM = numM-1;
	if ( nM >= numM )
		nM = 0;
	logEvtNI( "changeMsg", "iMsg", nM );
	TBook.iMsg = nM;
}
static void 					playSubjAudio( char *arg ){				// play current Subject: arg must be 'nm', 'pr', or 'msg'
	tbSubject * tbS = TBPkg->TBookSubj[ TBook.iSubj ];
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
static void 					playNxtPackage( ){										// play name of next available Package 
	iPkg++;
	if ( iPkg >= nPackages ) iPkg = 0;
	
	TBPackage_t * pkg = TBPackage[ iPkg ];
	logEvtNSNI( "PlayPkg", "Pkg", pkg->packageName, "Idx", iPkg );  
	playAudio( pkg->packageName, NULL );
}
static void 					showPkg( ){										// debug print Package iPkg
	TBPackage_t * pkg = TBPackage[ iPkg]; 
	printf( "iPkg=%d nm=%s nSubjs=%d \n", pkg->idx, pkg->packageName, pkg->nSubjs );
	printf( " path=%s \n", pkg->path );
	for (int i=0; i<pkg->nSubjs; i++){
		tbSubject *sb = pkg->TBookSubj[ i ];
		printf(" S%d nMsgs=%d pth=%s \n", i, sb->NMsgs, sb->path );
		printf("   nm=%s anm=%s pr=%s \n", sb->name, sb->audioName, sb->audioPrompt );
		for (int j=0; j<sb->NMsgs; j++)
			printf("   M%d %s \n", j, sb->msgAudio[j] );
	}
}
static void						changePackage(){											// switch to last played package name
	TBPkg = TBPackage[ iPkg ];
	TBook.iSubj = 0;
	TBook.iMsg = 0;
	logEvtNS( "ChgPkg", "Pkg", TBPkg->packageName );  
	showPkg();
}
static void 					playSysAudio( char *arg ){				// play system file 'arg'
	char path[MAX_PATH];
	resetAudio();
	buildPath( path, TB_Config.systemAudio, arg, ".wav" ); //".ogg" );
	playAudio( path, NULL );
	logEvtNS( "PlaySys", "file", arg );
}
static void						startRecAudio( char *arg ){
	resetAudio();
	tbSubject * tbS = TBPkg->TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	int mCnt = 0;
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".wav", &mCnt ); //".ogg" );		// build file path for next audio msg for S<iS>M<iM>
	logEvtNSNINI( "Record", "Subj", tbS->name, "iM", TBook.iMsg, "cnt", mCnt );
	MsgStats *stats = loadStats( tbS->name, TBook.iSubj, TBook.iMsg );	// load stats for message
	recordAudio( fNm, stats );
}
static void   				playRecAudio(){
	playRecording();
}
static void						saveRecAudio( char *arg ){
	if ( strcasecmp( arg, "sv" )==0 ){
		saveRecording();
	} else if ( strcasecmp( arg, "del" )==0 ){
		cancelRecording();
	} 
}
static void						saveWriteMsg( char *txt ){				// save 'txt' in Msg file
	tbSubject * tbS = TBPkg->TBookSubj[ TBook.iSubj ];
	char path[MAX_PATH];
	int mCnt = 0;
	char * fNm = logMsgName( path, tbS->name, TBook.iSubj, TBook.iMsg, ".txt", &mCnt );		// build file path for next text msg for S<iS>M<iM>
	FILE* outFP = tbOpenWrite( fNm ); //fopen( fNm, "w" );
	int nch = fprintf( outFP, "%s\n", txt );
	tbCloseFile( outFP );  //int err = fclose( outFP ); 
	dbgEvt( TB_wrMsgFile, nch, 0, 0, 0 );
	logEvtNSNININS( "writeMsg", "Subj", tbS->name, "iM", TBook.iMsg, "cnt", mCnt, "msg", txt );
}
static void 					USBmode( bool start ){						// start (or stop) USB storage mode
	if ( start ){
		logEvt( "enterUSB" );
		logPowerDown();				// flush & shut down logs
		enableMassStorage( "M0:", NULL, NULL, NULL );
	} else {	
		disableMassStorage();
		logEvt( "exitUSB" );
		ledFg( "_" );
		logPowerUp( false );
		if ( FirstSysBoot )  // should run in background
			decodeAudio();
	} 
}
static int						stIdx( int iSt ){
	if ( iSt < 0 || iSt >= nCSMstates )
		tbErr("invalid iSt");
	return iSt;
}
static void 					controlTest( void );  //DEBUG
static void 					doAction( Action act, char *arg, int iarg ){	// execute one csmAction
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
		case playRec:
			playRecAudio();
			logEvt( "playRec" );
			break;
		case saveRec:
			saveRecAudio( arg );
			logEvtS( "saveRec", arg );
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
		case resetTimer:
			osTimerStop( timers[2] );
			break;
		case showCharge:
			showBattCharge();
			break;
		case powerDown:		
			ledBg( NULL );								// turn off background heartbeat
			ledFg( TB_Config.fgPowerDown ); 	//  R R R   R R   R
			tbDelay_ms( 15000 );					// wait for LED sequence to finish
			powerDownTBook( false );
			break;
	  case sysTest:
			controlTest();
			break;
	  case playNxtPkg:
			playNxtPackage();
			break;
	  case changePkg:
			changePackage();
			break;
	  case sysBoot:
			
			break;
		default:				break; 
	}
}

// ------------- interpret TBook CSM 
static void						changeCSMstate( short nSt, short lastEvtTyp ){
	dbgEvt( TB_csmChSt, nSt, 0,0,0 );
	if (nSt==TBook.iCurrSt)
		logEvtNSNS( "No-op_evt", "state",TBook.cSt->nm, "evt", eventNm( (Event)lastEvtTyp) ); //DEBUG
	while ( nSt != TBook.iCurrSt ){
		TBook.iPrevSt = stIdx( TBook.iCurrSt );
		TBook.iCurrSt = stIdx( nSt );
		
		csmState *st = TBookCSM[ TBook.iCurrSt ];
		TBook.cSt = st;
		TBook.currStateName = st->nm;	//DEBUG -- update currSt string
		
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
static void						tbTimer( void * eNum ){
	switch ((int)eNum){
		case 0:		sendEvent( ShortIdle,  0 );  	break;
		case 1:		sendEvent( LongIdle,  0 );  	break;
		case 2:		sendEvent( Timer,  0 );  		break;
	}
}
static void 					executeCSM( void ){								// execute TBook control state machine
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
static void 					eventTest(  ){										// report Events until DFU (pot table)
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
//
static uint32_t 			startPlayingTS;
static uint32_t 			PlayLoopBattCheck = 30*60*1000;		// 30min
extern bool FSysPowerAlways;
extern bool SleepWhenIdle;

static void 					controlTest(  ){									// CSM test procedure
	TB_Event *evt;
	osStatus_t status;
	TBook.iCurrSt = stIdx( TB_Config.initState );
	TBook.cSt = TBookCSM[ TBook.iCurrSt ];
	TBook.currStateName = TBook.cSt->nm;	//DEBUG -- update currSt string
  bool playforever = false;
	bool recordingMsg = false;
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
			
		} else if ( recordingMsg ){
			audSt = audGetState();
			switch ( evt->typ ){
				case Table:
				case starTable:
					if ( audSt==Recording )
						stopRecording( );
					break;
				case Star:
					if ( audSt==Recording )
						pauseResume();
					break;
					
				case AudioDone:
				default:
					break;
					
				case Pot:
					if ( audSt==Recording )
						stopRecording( );
					playRecAudio();
					break;
				case Lhand:
					if ( audSt==Recording )
						stopRecording( );
					saveRecAudio( "sv" );
					recordingMsg = false;
					break;
				case Rhand:
					if ( audSt==Recording )
						stopRecording( );
					saveRecAudio( "del" );
					recordingMsg = false;
					break;
				case starHome:
					dbgLog( "going to USB mass-storage mode \n");
					USBmode( true );
					break;
			}
		} else {
			tbSubject * tbS = TBPkg->TBookSubj[ TBook.iSubj ];
			switch (evt->typ){
				case Tree:
					playSysAudio( "welcome" );
					dbgLog( "PlaySys welcome...\n" );
					break; 
				case AudioDone:
				case starPot: 
				case Pot:
					if ( evt->typ==AudioDone && !playforever ) break;
					if ( evt->typ==starPot ){
						playforever = !playforever;
						showBattCharge();
						if ( playforever ){
							startPlayingTS = tbTimeStamp();
							logEvt("PlayLoopStart");
						} else
							logEvt("PlayLoopEnd");
					} 
					if ( tbTimeStamp() - startPlayingTS > PlayLoopBattCheck ){
						showBattCharge();
						startPlayingTS = tbTimeStamp();
					}						
					dbgLog( "Playing msg...\n" );
					TBook.iMsg++;
					if ( TBook.iMsg >= tbS->NMsgs ) TBook.iMsg = 0;
					playSubjAudio( "msg" );
					break; 
				case Circle: 
					TBook.iSubj++;
					if ( TBook.iSubj >= TBPkg->nSubjs  ) TBook.iSubj = 0;
					tbS = TBPkg->TBookSubj[ TBook.iSubj ];
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
					audSt = audGetState();
					if ( audSt==Playing ){
						adjPlayPosition( -2 );
						logEvt( "JumpBack2" );
					} 
					break;
				case Rhand:
					if ( audSt==Playing ){
						adjPlayPosition( 2 );
						logEvt( "JumpFwd2" );
					}
					break;
				
				case Star:
					audSt = audGetState();
					if ( audSt==Playing )
						pauseResume();
					else
						showBattCharge();
					break;
					
				case Table:		// Record
					recordingMsg = true;
					startRecAudio( NULL );
					break;
				
				case starRhand:
					powerDownTBook( true );
					osTimerStart( timers[1], TB_Config.longIdleMS );
					playSysAudio( "faster" );
					break;
				case starLhand:
					FSysPowerAlways = true;
					SleepWhenIdle = false;
					break;
				case LongIdle:
					playSysAudio( "slower" );
					saveWriteMsg( "Clean power down" );
				  break;

				
				case starTree:
					playNxtPackage( );		// bump iPkg & plays next package name 
					showPkg();
					break;
				
				case starTable:		// switch packages to iPkg
					changePackage();
					showPkg();
					dbgLog(" iPkg=%d  iSubj=%d  iMsg=%d \n", iPkg, TBook.iSubj, TBook.iMsg );
					break;
				
				case starPlus:	
					decodeAudio();
					//eventTest();
					break;
				
				case starMinus: 
					executeCSM();
					break;
					
				case starCircle:
					saveWriteMsg( "Clean power down" );
					powerDownTBook( false );
				  break;
				
				case starHome:
					dbgLog( "going to USB mass-storage mode \n");
					USBmode( true );
					break;
				
				case FirmwareUpdate:   // pot table
					dbgLog( "rebooting to system bootloader for DFU... \n" );
					ledFg( TB_Config.fgEnterDFU );
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

void 									initControlManager( void ){				// initialize control manager 	
	// init to odd values so changes are visible
	TB_Config.default_volume = 8; 
	TB_Config.default_speed = 3;
	TB_Config.powerCheckMS = 10000;				// set by setPowerCheckTimer()
	TB_Config.shortIdleMS = 3000;
	TB_Config.longIdleMS = 11000;
	TB_Config.systemAudio = "M0:/system/audio/";			// path to system audio files
	TB_Config.minShortPressMS = 30;				// used by inputmanager.c
	TB_Config.minLongPressMS = 900;				// used by inputmanager.c
	TB_Config.bgPulse 				= (char *) bgPulse;
	TB_Config.fgPlaying 			= (char *) fgPlaying;
	TB_Config.fgPlayPaused		= (char *) fgPlayPaused;
	TB_Config.fgRecording			= (char *) fgRecording;
	TB_Config.fgRecordPaused	= (char *) fgRecordPaused;
	TB_Config.fgSavingRec			= (char *) fgSavingRec;
	TB_Config.fgSaveRec				= (char *) fgSaveRec;
	TB_Config.fgCancelRec			= (char *) fgCancelRec;
	TB_Config.fgUSB_MSC				= (char *) fgUSB_MSC;
	TB_Config.fgUSBconnect		= (char *) fgUSBconnect;
	TB_Config.fgTB_Error			= (char *) fgTB_Error;
	TB_Config.fgNoUSBcable		= (char *) fgNoUSBcable;
	TB_Config.fgPowerDown			= (char *) fgPowerDown;
	TB_Config.fgEnterDFU			= (char *) fgDFU;

	EventRecorderEnable(  evrEA, 	  		TB_no, TBsai_no ); 	// TB, TBaud, TBsai  
	EventRecorderDisable( evrAOD, 			EvtFsCore_No,   EvtFsMcSPI_No );  //FileSys library 
	EventRecorderDisable( evrAOD, 	 		EvtUsbdCore_No, EvtUsbdEnd_No ); 	//USB library 
	
	initTknTable();
	if ( TBDataOK ) {
		readControlDef( );						// reads TB_Config settings
		ledBg( TB_Config.bgPulse );		// reset background pulse according to TB_Config

		findPackages( );		// sets iPkg & TBPackage to shortest name
		
		TBook.iSubj = 0;
		TBook.iMsg = 1;
		
		setPowerCheckTimer( TB_Config.powerCheckMS );		// adjust power timer to input configuration
		setVolume( TB_Config.default_volume );					// set initial volume
		
		for ( int it=0; it<3; it++ ){
			timers[it] = osTimerNew( tbTimer, osTimerOnce, (void *)it, NULL );
			if ( timers[it] == NULL ) 
				tbErr( "timer not alloc'd" );
		}
    
    //dumpAll(4);
		//controlTest();  //DEBUG
		executeCSM();	
		
	} else if ( FileSysOK ) {		// FS but no data, go into USB mode
		logEvt( "NoFS USBmode" );
		USBmode( true );
	} else {  // no FileSystem
		eventTest();
	}
}
// contentmanager.cpp 
