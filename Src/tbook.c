// TBookV2    initialization
//	Apr2018 

#include "tbook.h"
#include "controlmanager.h"			// runs on initialization thread
#include "mediaPlyr.h"			// thread to watch audio status
#include "fs_evr.h"					// FileSys components
#include "fileOps.h"				// decode & encrypt audio files

const char * 	TBV2_Version 				= "V3.02 of 3-Mar-2021";

//
// Thread stack sizes
const int 		TBOOK_STACK_SIZE 		= 6144;		// init, becomes control manager
const int 		POWER_STACK_SIZE 		= 2048;
const int 		INPUT_STACK_SIZE 		= 1024;
const int 		MEDIA_STACK_SIZE 		= 4096;		// opens in/out files
const int 		FILEOP_STACK_SIZE 	= 6144;		// opens in/out files, mp3 decode
const int 		LED_STACK_SIZE 			= 512;

//const int pSTATUS 				= 0;
const int pCSM_VERS				= 0;
const int	pBOOTCNT 				= 1; 
const int	pCSM_DEF 				= 2;
const int	pLOG_TXT 				= 3;
const int	pSTATS_PATH 		= 4;
const int	pMSGS_PATH 			= 5;
const int	pLIST_OF_SUBJS 	= 6;
const int	pPACKAGE_DIR		= 7;
const int pPKG_VERS  		  = 8;
const int pAUDIO = 9;		// DEBUG
const int pLAST = 9;
char * TBP[] = {
		"M0:/system/version.txt",
		"M0:/system/bootcount.txt",
		"M0:/system/control.def",
		"M0:/log/tbLog.txt",
		"M0:/stats/",
		"M0:/messages/",
		"M0:/package/list_of_subjects.txt",
		"M0:/package/",
		"M0:/package/version.txt",
		"M0:/audio.wav"
};
	
const char *	TBOOK_STATUS 				= "M0:/system/status.txt"; 
const char *	TBOOK_BOOTCNT 			= "M0:/system/bootcount.txt"; 
const char *	TBOOK_CSM_DEF 			= "M0:/system/control.def";
const char *	TBOOK_LOG_TXT 			= "M0:/log/tbLog.txt";
const char *	TBOOK_STATS_PATH 		= "M0:/stats/";
const char *	TBOOK_MSGS_PATH 		= "M0:/messages/";
const char *	TBOOK_LIST_OF_SUBJS = "M0:/package/list_of_subjects.txt";
const char *	TBOOK_PACKAGE_DIR		= "M0:/package/";


//TBOOK error codes
const int 		TB_SUCCESS 						=	0;
const int 		GPIO_ERR_ALREADYREG 	=	1;
const int 		GPIO_ERR_INVALIDPIN 	=	2;

const int 		MEDIA_ALREADY_IN_USE	=	1;

const int 		MALLOC_HEAP_SIZE 			=	20000;
char 					MallocHeap[ 20000 ];    // MALLOC_HEAP_SIZE ];
bool					FileSysOK 						= false;
bool					TBDataOK 							= true;			// false if no TB config found


void setDev( char *fname, const char *dev ){  // replace front of fname with dev
	for (int i=0; i< strlen(dev); i++ )
	  fname[i] = dev[i];
}

static char * fsDevs[] = { "M0:", "M1:", "N0:", "F0:", "F1:", NULL };
static int    fsNDevs = 0;

void copyFile( const char *src, const char * dst ){
	FILE * fsrc = tbOpenRead( src ); //fopen( src, "r" );
	FILE * fdst = tbOpenWrite( dst ); //fopen( dst, "w" );
	if ( fsrc==NULL || fdst==NULL ) return;
	char buf[512];
	while (true){
		int cnt = fread( buf, 1, 512, fsrc );
		if ( cnt==0 )	break;

		fwrite( buf, 1, 512, fdst );
	}
	tbCloseFile( fsrc );		// fclose(fsrc);
	tbCloseFile( fdst );		// fclose(fdst);
}
int PlayDBG = 0;
void 								saiEvent( uint32_t event );

void debugLoop( bool autoUSB ){			// called if boot-PLUS, no file system,  autoUSB => usbMode
	if ( fsNDevs==0 ) dbgLog( "no storage avail \n" );
	else dbgLog( "no TBook on %s \n", fsDevs[0]  );

	MediaState st = Ready;
	int ledCntr = 0, LEDpauseCnt = 0;
	bool curPl,prvPl, curMi,prvMi, curTr,prvTr, curLH,prvLH, curRH,prvRH = false;
	bool inTestSequence = false;
	
	initLogger();			// init Event log
	logEvtNI( "DebugLoop", "NFSys", fsNDevs );
	
	while ( true ){		// loop processing debug commands
		
		// LED control-- don't change LEDs unless LEDpauseCnt == 0, <0 while USB or Playing, >0 long flash
		st = audGetState();
		if ( LEDpauseCnt != 0 ){		// =-1 
			if ( LEDpauseCnt > 0 )
				LEDpauseCnt--;
		} else if (st == Ready) {  // audio Ready-- flash RED ON for 1 of 4  
			ledCntr++;
			gSet( gGREEN, 0 );
			gSet( gRED, ((ledCntr >> 14) & 0x3)== 0 );		// flash RED ON for 1 of 4  
	  }
		
		// AUDIO commands -- CIR, PL_CIR, MI_CIR, LH_CIR -- play different tones
		if ( st==Playing ){ // controls while playing  PLUS(vol+), MINUS(vol-), TREE(pause/resume), LH adjPos-2), RH adjPos(2)
			curPl = gGet( gPLUS );
			if ( curPl & !prvPl )
				adjVolume( 5 ); 
			prvPl = curPl;
			
			curMi = gGet( gMINUS );
			if ( curMi & !prvMi )
				adjVolume( -5 ); 
			prvMi = curMi;
		
			curTr = gGet(gTREE);
			if (curTr && !prvTr) 
				pauseResume();
			prvTr = curTr;
			
			curLH = gGet(gLHAND);
			if (curLH && !prvLH) 
				adjPlayPosition( -2 );
			prvLH = curLH;
			
			curRH = gGet(gRHAND);
			if (curRH && !prvRH) 
				adjPlayPosition( 2 );
			prvRH = curRH;
		} else if ( gGet( gCIRCLE ) && st==Ready ){		// CIRCLE => 1KHz audio, (PLUS CIRCLE)440Hz=A, MINUS CIR:C, LH CIR:E
			gSet( gRED, 0 );
			gSet( gGREEN, 1 );
			if ( fexists( TBP[pAUDIO] )){
				//PlayDBG: TABLE=x1 POT=x2 PLUS=x4 MINUS=x8 STAR=x10 TREE=x20
				PlayDBG = (gGet( gTABLE )? 1:0) + (gGet( gPOT )? 2:0) + (gGet( gPLUS )? 4:0) + (gGet( gMINUS )? 8:0) + (gGet( gSTAR )? 0x10:0) + (gGet( gTREE )? 0x20:0); 
				dbgEvt( TB_dbgPlay, PlayDBG, 0,0,0 );
				dbgLog( " PlayDBG=0x%x \n", PlayDBG );
				playWave( TBP[pAUDIO] );
			} else {
				int hz = gGet( gPLUS )? 440 : gGet( gMINUS )? 523 : gGet( gLHAND )? 659 : 1000;
				audSquareWav( 5, hz );							// subst file data with square wave for 5sec
				playWave( "SQR.wav" );							// play x seconds of hz square wave
			}
			LEDpauseCnt = -1;
		}
		//
		// USB commands -- have FS and autoUSB or HOME -- enable
		if ( isMassStorageEnabled() ){
			if (gGet( gSTAR ) || gGet( gHOME )){	 // STAR HOME => close MSC, continue boot
				disableMassStorage();
				gSet( gRED, 0 );
				return;		// exit DebugLoop after disabling USB
			}
		} else if ( fsNDevs > 0 && ( autoUSB || gGet( gHOME ))){  // HOME => if have a filesystem but no data -- try USB MSC
			gSet( gRED, 1 );
			LEDpauseCnt = -1;		// solid RED while in USB
			for ( int i=fsNDevs; fsDevs[i]!=NULL; i++ ) fsDevs[i] = NULL;
			enableMassStorage( fsDevs[0], fsDevs[1], fsDevs[2], fsDevs[3] );		// just put 1st device on USB
		}			

		//
		// TestSequence -- RHAND to enter (RED), STAR to proceed (GREEN)
		if ( inTestSequence ){
			if (gGet( gSTAR )){
				gSet( gGREEN, 1 );
				Driver_SAI0.Initialize( &saiEvent );   
				Driver_SAI0.PowerControl( ARM_POWER_FULL );
				uint32_t ctrl = ARM_SAI_CONFIGURE_RX | ARM_SAI_MODE_SLAVE  | ARM_SAI_ASYNCHRONOUS | ARM_SAI_PROTOCOL_I2S | ARM_SAI_DATA_SIZE(16);
				Driver_SAI0.Control( ctrl, 0, 8000 );	// set sample rate, init codec clock
				
				LEDpauseCnt = 100000;  // long GREEN => running clocks
			}
		} else if ( gGet( gRHAND )){			//  RHAND =>  Reboot to Device Firmware Update
			inTestSequence = true;
			gSet( gRED, 1 );
		//	testEncrypt();
		//	RebootToDFU();
			cdc_PowerUp( );			// enable all codec power
			LEDpauseCnt = 100000;	// long RED => power enabled
		}
	}
}

//
//  TBook main thread
void talking_book( void *arg ) {
	dbgLog( "4 tbThr: 0x%x 0x%x \n", &arg, &arg + TBOOK_STACK_SIZE );
	
	EventRecorderInitialize( EventRecordNone, 1 );  // start EventRecorder
	EventRecorderEnable( evrE, 			EvtFsCore_No, EvtFsMcSPI_No );  	//FileSys library 
	EventRecorderEnable( evrE, 	 		EvtUsbdCore_No, EvtUsbdEnd_No ); 	//USB library 

	EventRecorderEnable( evrEA,  		TB_no, TB_no );  									//TB:  	Faults, Alloc, DebugLoop
	EventRecorderEnable( evrE, 			TBAud_no, TBAud_no );   					//Aud: 	audio play/record
	EventRecorderEnable( evrE, 			TBsai_no, TBsai_no ); 	 					//SAI: 	codec & I2S drivers
	EventRecorderEnable( evrEA, 		TBCSM_no, TBCSM_no );   					//CSM: 	control state machine
	EventRecorderEnable( evrE, 			TBUSB_no, TBUSB_no );   					//TBUSB: 	usb user callbacks
	EventRecorderEnable( evrE, 			TBUSBDrv_no, TBUSBDrv_no );   		//USBDriver: 	usb driver
	EventRecorderEnable( evrE, 			TBkey_no, TBkey_no );   					//Key: 	keypad input
	
	initPowerMgr();			// set up GPIO signals for controlling & monitoring power -- enables MemCard
	
	// eMMC &/or SDCard are powered up-- check to see if we have usable devices
	EventRecorderEnable( evrEAOD, 			EvtFsCore_No, EvtFsMcSPI_No );  	//FileSys library 
	for (int i=0; fsDevs[i]!=NULL; i++ ){
		fsStatus stat = fsMount( fsDevs[i] );
		if ( stat == fsOK ){	// found a formatted device
			fsDevs[ fsNDevs ] = fsDevs[ i ];
			fsNDevs++;
		} 
		dbgLog( "3 %s%d ", fsDevs[i], stat );
	}
	EventRecorderDisable( evrAOD, 			EvtFsCore_No, EvtFsMcSPI_No );  	//FileSys library 
	dbgLog( "3 NDv=%d \n", fsNDevs );

	if ( osKernelGetState() != osKernelRunning ) // no OS, so straight to debugLoop (no threads)
		debugLoop( fsNDevs > 0 );	
	
	//usrLog( "%s\n", CPU_ID );
	//usrLog( "%s\n", TB_ID );

	initMediaPlayer( );
	
	if ( fsNDevs == 0 )
		debugLoop( false );
	else {
		if ( strcmp( fsDevs[0], "M0:" )!=0 ){ 	// not M0: !
			flashCode( 10 );		// R G R G : not M0:
			flashLED( "__" );
		}
		for ( int i = pCSM_VERS; i <= pLAST; i++ ){		// change paths to fsDevs[0]
			setDev( TBP[ i ], fsDevs[0] );
		}
		if ( fsNDevs > 1 ){
			if ( fexists( TBP[pAUDIO] )){	// audio.wav exists on device fsDevs[0] --- copy it to fsDevs[1]
				char dst[40];
				strcpy( dst, TBP[pAUDIO] );
				setDev( dst, fsDevs[1] );
				copyFile( TBP[pAUDIO], dst );
			}
		}

		if ( !fexists( TBP[pCSM_VERS] ) && fexists( "M0:/system/status.txt" ))
			copyFile( TBP[pCSM_VERS], "M0:/system/status.txt" );  // backward compatibility
		
		if ( gGet( gMINUS ) || !fexists( TBP[pCSM_VERS] ))	
			debugLoop( !fexists( TBP[pCSM_VERS] ) );	// if no version.txt -- go straight to USB mode
	}


	initLogger( );

	initLedManager();									//  Setup GPIOs for LEDs
	
	initInputManager();								//  Initialize keypad handler & thread

	initFileOps();										//  decode mp3's 
	
	const char* startUp = "R3_3G3";
	ledFg( startUp );
	
	initControlManager();		// instantiate ControlManager & run on this thread-- doesn't return
}
// end  tbook.c
