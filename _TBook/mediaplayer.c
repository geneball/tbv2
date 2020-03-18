// TBookV2  mediaplayer.c		play/record audio
//  Apr2018

#include "mediaPlyr.h"

const int 									CODEC_DATA_TX_DN   =	0x01; 			// signal sent by SAI callback when an buffer completes
const int 									MEDIA_PLAY_EVENT	 =	0x10;
const int 									MEDIA_RECORD_START =	0x20;

static 	osThreadAttr_t 				thread_attr;
static 	osThreadId_t					mMediaThreadId;
static 	osEventFlagsId_t			mMediaEventId;

static int audioVolume				= DEFAULT_VOLUME;

// communication variables shared with mediaThread
static volatile char					mPlaybackFilePath[ MAX_PATH ];
static volatile MsgStats *		mPlaybackStats;
static volatile char 					mRecordFilePath[ MAX_PATH ];
static volatile MsgStats *		mRecordStats;

static void 	mediaThread( void *arg );						// forward 
	
void					initMediaPlayer( void ){						// init mediaPlayer & spawn thread to handle  playback & record requests
	mMediaEventId = osEventFlagsNew(NULL);					// osEvent channel for communication with mediaThread
	if ( mMediaEventId == NULL)
		tbErr( "mMediaEventId alloc failed" );	

	mPlaybackFilePath[0] = 0;			
	mRecordFilePath[0] = 0;			
	
	thread_attr.name = "Media";
	thread_attr.stack_size = MEDIA_STACK_SIZE; 
	mMediaThreadId = osThreadNew( mediaThread, NULL, &thread_attr );
	if ( mMediaThreadId == NULL )
		tbErr( "mediaThread spawn failed" );	
	
	//registerPowerEventHandler( handlePowerEvent );
	audInitialize( mMediaEventId );
}
void					mediaPowerDown( void ){
	audPowerDown();
}

// external methods for  controlManager executeActions
int 					playAudio( const char * fileName, MsgStats *stats ){ // start playback from fileName
	while ( audGetState()!=Ready ){ 
		audStopAudio();
		osDelay( 5 );
	}
	strcpy( (char *)mPlaybackFilePath, fileName );
	mPlaybackStats = stats;
	osEventFlagsSet( mMediaEventId, MEDIA_PLAY_EVENT );
	return TB_SUCCESS;
}
void 					pauseResume( void ){								// pause (or resume) playback or recording
	audPauseResumeAudio( );
}
void 					stop( void ){												// stop playback
	audStopAudio( );
}

int						playPosition( void ){								// => current playback position in sec
	return audPlayPct();
}
void 					adjPlayPosition( int sec ){					// skip playback forward/back 'sec' seconds
	audAdjPlayPos( sec );
}
void 					adjVolume( int adj ){								// adjust playback volume by 'adj'
	audioVolume += adj;
	if ( audioVolume > MAX_VOLUME ) audioVolume = MAX_VOLUME;
	if ( audioVolume < MIN_VOLUME ) audioVolume = MIN_VOLUME;
	ak_SetVolume( audioVolume );
	dbgLog( "SetVol:%d  [%d]", audioVolume, audPlayPct() );
}
void 					adjSpeed( int adj ){								// adjust playback speed by 'adj'
	audAdjPlaySpeed( adj );
}
MediaState		getStatus( void ){									// => Ready / Playing / Recording
	return audGetState();
}
int						recordAudio( const char * fileName, MsgStats *stats ){	// start recording into fileName
	while ( audGetState()!=Ready ){ 
		audStopAudio();
		osDelay( 5 );
	}
	strcpy( (char *)mRecordFilePath, fileName );
	mRecordStats = stats;
	osEventFlagsSet( mMediaEventId, MEDIA_RECORD_START );
	return TB_SUCCESS;
}
void					stopRecording( void ){  						// stop recording
	audStopRecording( ); 
}

/* **************  mediaThread -- start audio play & record operations, handle completion signal
// 		MEDIA_PLAY_EVENT 	=> play mPlaybackFilename
//		MEDIA_RECORD_START 	=> record into mRecordFilePath
//		CODEC_PLAY_DONE 	=> send AudioDone CSM event
***************/
static void 	mediaThread( void *arg ){						// communicates with audio codec for playback & recording		
	const int MEDIA_EVENTS = MEDIA_PLAY_EVENT | MEDIA_RECORD_START | CODEC_DATA_TX_DN;
	while (true){		
		uint32_t flags = osEventFlagsWait( mMediaEventId, MEDIA_EVENTS,  osFlagsWaitAny, osWaitForever );
		
		if ( (flags & CODEC_DATA_TX_DN) != 0 ){								// buffer transmission complete from SAI_event
			audPlaybackDn( );				// plays next buffer, or generates  AudioDn event
		} else if ( (flags & MEDIA_PLAY_EVENT) != 0 ){				// request to start playback
			if ( mPlaybackFilePath[0] == 0 ) continue;
			audPlayAudio( (const char *)mPlaybackFilePath, (MsgStats *) mPlaybackStats );
			
		} else if ( (flags & MEDIA_RECORD_START) != 0 ){			// request to start recording
			FILE* outFP = fopen( (const char *)mRecordFilePath, "wb" );
			if ( outFP != NULL ){
				audStartRecording( outFP, (MsgStats *) mRecordStats );
			} else {
				printf ("Cannot open record file to write\n\r");
			}
		}
	}
}
//end  mediaplayer.c

