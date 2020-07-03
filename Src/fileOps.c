// TBookV2  fileOps.c		audio file decode or encrypt thread
//  Jul2020

#include "fileOps.h"
#include "EncAudio.h"
#include <string.h>

const int 									FILE_ENCRYPT_REQ   =	0x01; 	// signal sent to request audio file encryption
const int 									FILE_DECODE_REQ	 	 =  0x02;		// signal sent to request audio file decoding
const int 									FILEOP_EVENTS 		 =  0x3;		// mask for all events

static 	osThreadAttr_t 			thread_attr;
static  osEventFlagsId_t		mFileOpEventId;								// for signals to fileOpThread

static volatile char				mFileArgPath[ MAX_PATH ];			// communication variable shared with fileOpThread


static void 	fileOpThread( void *arg );						// forward 
	
extern void					initFileOps( void ){								// init fileOps & spawn thread to handle audio encryption & decoding
	mFileOpEventId = osEventFlagsNew(NULL);					// osEvent channel for communication with mediaThread
	if ( mFileOpEventId == NULL)
		tbErr( "mFileOpEventId alloc failed" );	
	
	mFileArgPath[0] = 0;			
	
	thread_attr.name = "FileOp";
	thread_attr.stack_size = FILEOP_STACK_SIZE; 
	void * thread = osThreadNew( fileOpThread, NULL, &thread_attr );
	if ( thread == NULL )
		tbErr( "fileOpThread spawn failed" );	
}
extern void 				decodeAudio(){									// decode all .mp3 files in /system/audio/ & /package/*/  to .wav 
	osEventFlagsSet( mFileOpEventId, FILE_DECODE_REQ );		// transfer request to fileOpThread
}

extern void					copyEncrypted( char *src ){			// make an encrypted copy of src (.wav) as dst
	strcpy( (char *)mFileArgPath, src );
	osEventFlagsSet( mFileOpEventId, FILE_ENCRYPT_REQ );		// transfer request to fileOpThread
}
//
//****************** internal
static void 				encryptCopy( ){
	char basenm[ MAX_PATH ];
	strcpy( basenm, (const char *)mFileArgPath );		// copy *.wav path
	char *pdot = strrchr( basenm, '.' );
	*pdot = 0;
	
 	startEncrypt( basenm );				// write basenm.key & initialize AES encryption
	strcat( basenm, ".dat" );
	
	FILE * fwav = fopen( (const char *)mFileArgPath, "rb" );
	FILE * fdat = fopen( basenm, "wb" );
	if ( fwav==NULL || fdat==NULL ) tbErr("encryptCopy: fwav=%x fdat=%x", fwav, fdat );

	uint32_t flen = 0;
	uint8_t *wavbuff = tbAlloc( BUFF_SIZ, "wavbuff");
	uint8_t *datbuff = tbAlloc( BUFF_SIZ, "datbuff");
	
	while (1){
		uint16_t cnt = fread( wavbuff, 1, BUFF_SIZ, fwav );
		if ( cnt == 0 ) break;
		
		if ( cnt < BUFF_SIZ ) memset( wavbuff+cnt, 0, BUFF_SIZ-cnt );		// extend final block with 0s
		encryptBlock( wavbuff, datbuff, BUFF_SIZ );
		cnt = fwrite( datbuff, 1, BUFF_SIZ, fdat );
    flen += cnt;
		if (cnt != BUFF_SIZ ) tbErr("encryptCopy fwrite => %d", cnt);
 	}
	int res = fclose( fwav );
	if ( res != fsOK ) tbErr("wav fclose => %d", res );
	
	res = fclose( fdat );
	if ( res != fsOK ) tbErr("dat fclose => %d", res );
	logEvtNS("encF", "dat", basenm );

  basenm[ strlen(basenm)-3 ] = 'x';
 	res = frename( (const char *)mFileArgPath, basenm );	// rename .wav => .xav
// 	res = fdelete( (const char *)mFileArgPath, NULL );		// delete wav file
	if ( res != fsOK ) tbErr("dat fclose => %d", res );	
}

static void 				decodeMp3( const char *path, char *fname ){			// decode fname (.mp3) & copy to .wav
	dbgLog("decode %s/%s \n", path, fname );
}
static void 				scanDecodeAudio( ){			// scan audio paths for .mp3 & copy to .wav
	const char *packDirPatt = "M0:/package/*";
	char path[ MAX_PATH ], patt[ MAX_PATH ]; 

	fsFileInfo fInfo;
	fInfo.fileID = 0;
	strcpy( path, "M0:/ststem/audio/" );
	strcpy( patt, path );
	strcat( patt, "*.mp3" );
	while ( ffind( patt, &fInfo )==fsOK ){		// process all .mp3 in /system/audio
		decodeMp3( path, fInfo.name );
	}
	
	fsFileInfo fDir;
	fDir.fileID = 0;
	while ( ffind(packDirPatt, &fDir )==fsOK ){		// process all directories in /package/
		dbgLog("package dir %s \n", fDir.name );
		if ( fDir.name[0]!='.' && fDir.attrib == 0x10 ){  // dir other than . or ..
			strcpy( path, "M0:/package/" );
			strcat( path, fDir.name );
			
			strcpy( patt, path );
			strcat( patt, "/*.mp3" );
			
			fInfo.fileID = 0;
			while ( ffind( patt, &fInfo )==fsOK ){		// process all .mp3 in /package/dir/
				decodeMp3( path, fInfo.name );
			}
		}
	}
}

static void 				fileOpThread( void *arg ){				
	// on wakeup, scan for audio to decode
	scanDecodeAudio();
	
	while (true){		
		uint32_t flags = osEventFlagsWait( mFileOpEventId, FILEOP_EVENTS,  osFlagsWaitAny, osWaitForever );
		
		if ( (flags & FILE_ENCRYPT_REQ) != 0 ){
			encryptCopy();
		}
		
		if ( (flags & FILE_DECODE_REQ) != 0 ){
			scanDecodeAudio();
		}
	}
}
