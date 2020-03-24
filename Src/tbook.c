// TBookV2    initialization
//	Apr2018 

#include "tbook.h"
#include "controlMgr.h"			// runs on initialization thread
#include "mediaPlyr.h"			// thread to watch audio status

const char * 	TBV2_Version 				= "V2.04 of 20-Mar-2020";

//
// Thread stack sizes
const int 		TBOOK_STACK_SIZE 		= 6000;
const int 		POWER_STACK_SIZE 		= 1024;
const int 		INPUT_STACK_SIZE 		= 1024;
const int 		MEDIA_STACK_SIZE 		= 4048;		// opens in/out files
const int 		CONTROL_STACK_SIZE 	= 1024;		// opens in/out files
const int 		LED_STACK_SIZE 			= 512;

const int pSTATUS 				= 0;
const int	pBOOTCNT 				= 1; 
const int	pCSM_DEF 				= 2;
const int	pLOG_TXT 				= 3;
const int	pSTATS_PATH 		= 4;
const int	pMSGS_PATH 			= 5;
const int	pLIST_OF_SUBJS 	= 6;
const int	pPACKAGE_DIR		= 7;
char * TBP[] = {
		"M0:/system/status.txt",
		"M0:/system/bootcount.txt",
		"M0:/system/control.def",
		"M0:/log/tbLog.txt",
		"M0:/stats/",
		"M0:/messages/",
		"M0:/package/list_of_subjects.txt",
		"M0:/package/"
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


fsStatus fsMount( char *drv ){		// try to finit() & mount()  drv:   finit() code, fmount() code
		fsStatus stat = finit( drv );  		// init file system driver for device
	  if ( stat != fsOK ){
			//dbgLog( "finit( %s ) got %d \n", drv, stat );
			return stat;
		}
		stat = fmount( drv );
		if ( stat==fsOK ) return stat;

		if ( stat == fsNoFileSystem ){
			stat = fformat( drv, "/FAT32" );
			if ( stat == fsOK ) return stat;   // successfully formatted
			
			dbgLog( "formating %s got %d \n", drv, stat );
			return stat;
		}
		return stat;
}
static char * fsDevs[] = { "M0:", "M1:", "N0:", "F0:", "F1:", NULL };
static int    fsNDevs = 0;

void debugLoop( ){
	if ( fsNDevs==0 ) dbgLog( "no storage avail \n" );
	else dbgLog( "no TBook on %s \n", fsDevs[0]  );
	MediaState st = Ready;
	int ledCntr = 0;
	
	while ( true ){
		ledCntr++;
		st = audGetState();
		if ( st==Ready )
			gSet( gRED, ((ledCntr >> 14) & 0x3)== 0 );		// flash RED ON for 1 of 4  
		
		if ( fsNDevs > 0 && gGet( gHOME ) && !isMassStorageEnabled()){  // HOME => if have a filesystem but no data -- try USB MSC
			gSet( gRED, 1 );
			for ( int i=fsNDevs; fsDevs[i]!=NULL; i++ ) fsDevs[i] = NULL;
			enableMassStorage( fsDevs[0], fsDevs[1], fsDevs[2], fsDevs[3] );		// just put 1st device on USB
		}
		
		if ( isMassStorageEnabled() && gGet( gSTAR ) && gGet( gHOME )){	 // STAR HOME => close MSC, continue boot
			disableMassStorage();
			gSet( gRED, 0 );
			return;	
		}
		
		if ( gGet( gRHAND )){			//  RHAND =>  Reboot to Device Firmware Update
			gSet( gGREEN, 1 );
			gSet( gRED, 1 );
			RebootToDFU();
		}

		if ( gGet( gCIRCLE ) && st==Ready ){		// CIRCLE => audio test
			gSet( gRED, 0 );
			audSquareWav( 10 );									// subst file data with preloaded square wave for 10sec
			gSet( gGREEN, 1 );
			PlayWave( "SQR.wav" );							// play 10 seconds of 1KHz square wave
		}
	}
}

//
//  TBook main thread
void talking_book( void *argument ) {
	
	initPowerMgr();			// set up GPIO signals for controlling & monitoring power -- enables MemCard
	
	if ( osKernelGetState() != osKernelRunning )
		debugLoop();	// no OS, so straight to debugLoop, no fileSys check & no mediaManager
	
	usrLog( "%s\n", CPU_ID );
	usrLog( "%s\n", TB_ID );

	initMediaPlayer( );
	
	for (int i=0; fsDevs[i]!=NULL; i++ ){
		fsStatus stat = fsMount( fsDevs[i] );
		if ( stat == fsOK ){	// found a formatted device
			fsDevs[ fsNDevs ] = fsDevs[ i ];
			fsNDevs++;
		} 
		dbgLog( "%s%d ", fsDevs[i], stat );
	}
	dbgLog( " NDv=%d \n", fsNDevs );
	if ( fsNDevs == 0 )
		debugLoop( );
	else {
		if ( strcmp( fsDevs[0], "M0:" )!=0 ){ 	// not M0: !
			flashCode( 10 );		// R G R G : not M0:
			flashLED( "__" );
		}
		for ( int i = pSTATUS; i <= pPACKAGE_DIR; i++ ){
				TBP[ i ][0] = fsDevs[0][0];
				TBP[ i ][1] = fsDevs[0][1];
		}
		FILE *f = fopen( TBOOK_STATUS, "r" );			// try to open "system/status.txt"
		if ( f==NULL )	
			debugLoop( );
		else
			fclose( f );		// load full TBook
	}

	// signal start
	flashLED( "GGGGGRRRRR" );

	initLogger( );
	logEvt( "PowerUp" );

	initLedManager();									//  Setup GPIOs for LEDs
	
	initInputManager();								//  Initialize keypad handler & thread
	
//	const char* startUp = "R3G3_4 R3G3_4 R3G3_4";
//	ledFg( startUp );
	
//	initUSBManager( );

	initControlManager();		// instantiate ControlManager & run on this thread-- doesn't return
}
// end  tbook.c
