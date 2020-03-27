// audio.c  -- playback & record functions called by mediaplayer

#include "audio.h"

const int WaveHdrBytes 			= 44;			// length of .WAV file header
const int SAMPLE_RATE_MIN 	=  8000;
const int SAMPLE_RATE_MAX 	= 96000;
const int BuffLen 					= 4096;
const int nBuffs 						= 3;			// number of audio buffers allocated in audio.c

static PlaybackFile_t 		pSt;							// audio player state
static Buffer_t audio_buffers[ nBuffs ];
static osEventFlagsId_t audioEventChan;

void 								initBuffs(){										// create audio buffers
	for ( int i=0; i<nBuffs; i++ ){
		Buffer_t *pB = &audio_buffers[i];
		pB->state = bEmpty;
		pB->data = (uint8_t *) tbAlloc( BuffLen, "audio buffer" );
	}
}
Buffer_t * 					allocBuff(){										// get a buffer for use
	for ( int i=0; i<nBuffs; i++ ){
		if ( audio_buffers[i].state == bEmpty ){ 
			Buffer_t * pB = &audio_buffers[i];
			pB->state = bAlloc;
			return pB;
		}
	}
	errLog("out of audio buffers");
	return NULL;
}
void 								freeBuff( Buffer_t *pB ){				// release buffer
	pB->state = bEmpty;
}
void 								audInitialize( osEventFlagsId_t eventId ){ 		// allocs buffers, sets up SAI, gets signalChannel
	audioEventChan = eventId;
	
	pSt.state = pbIdle;
	initBuffs();
	pSt.wavHdr = (WAVE_FormatTypeDef*) tbAlloc( WaveHdrBytes, "wav file" );  // alloc space for header

	// Initialize I2S & register callback
  //	I2S_TX_DMA_Handler() overides DMA_STM32F10x.c's definition of DMA1_Channel5_Handler 
	//    calls I2S_TX_DMA_Complete, which calls saiEvent with DMA events
	Driver_SAI0.Initialize( &saiEvent );   
}
void 								audSquareWav( int nsecs ){												// preload wavHdr & buffers with 1KHz square wave
	pSt.SqrWAVE = true;			// pre-init wav.Hdr & buffers with 1KHz square wave
	WAVE_FormatTypeDef *wav = pSt.wavHdr;
	wav->SampleRate = 8000;
	pSt.SqrBytes = wav->SampleRate * 2 * nsecs;
	wav->NbrChannels = 1;
	wav->BitPerSample = 16;
	wav->SubChunk2Size =  pSt.SqrBytes; // bytes of data 

	for ( int i=0; i<nBuffs; i++ ){	// preload audio_buffers with !KHz square wave @ 8KHz == (1,1,1,1,0,0,0,0)...
		Buffer_t *pB = &audio_buffers[i];
		uint16_t *data = (uint16_t *) &pB->data[0];
		for ( int j=0; j<BuffLen/4; j++ ){  // # of 16bit words in just 1st half of buffer
			data[ j ] = 0;
			if ( !gGet( gPLUS ))
				data[ j ] = (j & 4)? 0x1010 : 0x9090;
		}
	}
}
void								audPowerDown( void ){						// shut down and reset everything
	Driver_SAI0.Uninitialize( );   
}
MediaState 					audGetState( void ){						// => Ready or Playing or Recording
	switch ( pSt.state ) {
		case pbDone:			// I2S of audio done
		case pbIdle:		return Ready; 		// not playing anything
		
		case pbOpening:	// calling fopen 
		case pbLdHdr:		// fread header
		case pbGotHdr:		// fread hdr done
		case pbFilling:	// fread 1st data
		case pbFull: 		// fread data done
		case pbPlaying: 	// I2S started
		case pbPlayFill:	// fread data under I2S
		case pbLastPlay:	// fread data got 0
		case pbFullPlay:	// fread data after I2S done
		case pbPaused:
				return Playing;
	}
	return Ready;
}
int32_t 						audPlayPct( void ){							// => current playback pct of file
	uint32_t now = tbTimeStamp();	
	if ( pSt.state == pbDone ) return 0;	// not playing
	
	uint32_t played = pSt.msPlayBefore + (now - pSt.tsResume);	// elapsed since last Play
	uint32_t totmsec = pSt.nSamples * 1000 / pSt.samplesPerSec;
	return played*100 / totmsec;
}
void 								audAdjPlayPos( int32_t adj ){		// shift current playback position +/- by 'adj' seconds
}
void 								audAdjPlaySpeed( int16_t adj ){	// change playback speed by 'adj' * 10%
}
void 								audPlayAudio( const char* audioFileName, MsgStats *stats ){ // start playing from file
	pSt.stats = stats;
	stats->Start++;
	
	if ( strcmp( &audioFileName[ strlen( audioFileName )-4 ], ".wav" ) == 0 ){
		
		PlayWave( audioFileName );
	}
}
void 								audStopAudio( void ){						// signal playback loop to stop
	if ( pSt.reqStop ) return;		// duplicate request
	pSt.reqStop = true;						// signal a pause, then stop
	pSt.reqPause = true;
}
void 								audStartRecording( FILE *outFP, MsgStats *stats ){	// start recording into file -- TBV2_REV1 version
}
void 								audStopRecording( void ){				// signal record loop to stop
}
void 								audPauseResumeAudio( void ){		// signal playback to request Pause or Resume
	if ( pSt.reqPause ) return;		// duplicate request
	
	if ( pSt.state==pbPaused ){
		pSt.tsResume = tbTimeStamp();
		pSt.stats->Resume++;
		ak_SetMute( false );		// unmute
		gSet( gGREEN, 1 );	// Turn ON green LED: audio file playing 
		PlayNext();		// continue with next buffer
		
	} else {
		pSt.tsPauseReq = tbTimeStamp();
		pSt.stats->Pause++;
		ak_SetMute( true );
		pSt.reqPause = true;	// ask for pause at end of buffer
	}
}

void 								printAudSt(){										// DBG: display audio state
		uint32_t cTmStamp = tbTimeStamp();
		printf( "  pSt: 0x%08x \n ", (uint32_t) &pSt );
		printf( "  pSt.state: %d \n ", pSt.state );
	  if ( pSt.bytesPerSample != 0 ){
			printf( "  %d BpS @ %d \n ", pSt.bytesPerSample, pSt.wavHdr->SampleRate );
			printf( "  %d of %d \n ", pSt.nPlayed, pSt.nSamples );
			printf( "  %d + %d msec \n ", pSt.msPlayBefore,  cTmStamp - pSt.tsPlay );
		} else {
			printf( "  Open %d msec \n ", cTmStamp - pSt.tsOpen );
		}
}

Buffer_t * 					loadBuff( ){										// read next block of audio into a buffer
	Buffer_t *pB = allocBuff();
	pB->firstSample = pSt.nPlayed;
	int len = pSt.monoMode? BuffLen/2 : BuffLen;		// leave room to duplicate if mono
	if ( pSt.SqrWAVE ){	// data is pre-loaded in all buffers
		pSt.SqrBytes -= len;
		pB->cntBytes = pSt.SqrBytes>0? len : 0;
	} else {
		pB->cntBytes = fread( pB->data, 1, len, pSt.wavF );		// read up to BuffLen bytes
	}
	if ( pSt.monoMode ){
		for ( int i=len-1; i>0; i-- )
			pB->data[i] = pB->data[ i>>1 ];   // len==2048:  d[2047] = d[1023], d[2046]=d[1023], d[2045] = d[1022], ... d[2]=d[1], d[1]=d[0] 
	}
	pB->state = bFull;
	return pB;
}
void 								PlayWave( const char *fname ){ 		// play the WAV file 
	// set up playState
	pSt.LastError = 0;
	pSt.ErrCnt = 0;
	pSt.state = pbOpening;
	pSt.buffNum = -1;
	pSt.nPlayed = 0;
	pSt.bytesPerSample = 0; 
	pSt.samplesPerSec = 0;
	pSt.nSamples = 0;
	pSt.tsOpen = tbTimeStamp();		// before fopen
	pSt.tsPlay = 0;
	pSt.tsPauseReq = 0;
	pSt.tsPause = 0;
	pSt.tsResume = 0;
	pSt.msPlayBefore = 0;
	pSt.reqPause = false;	
	
	//dbgLog( "Wv: %s\n", fname );
	pSt.state = pbLdHdr;
  if ( !pSt.SqrWAVE ){	// if SqrWAVE, pSt.wavHdr is preloaded
		pSt.wavF = fopen( fname, "r" );
		if ( pSt.wavF==NULL || fread( pSt.wavHdr, 1, WaveHdrBytes, pSt.wavF ) != WaveHdrBytes ) 
			errLog( "open wav failed" );
	}
	pSt.state = pbGotHdr;
	int audioFreq = pSt.wavHdr->SampleRate;
  if ((audioFreq < SAMPLE_RATE_MIN) || (audioFreq > SAMPLE_RATE_MAX))
    errLog( "bad audioFreq" );  
	pSt.samplesPerSec = audioFreq;
	
	Driver_SAI0.PowerControl( ARM_POWER_FULL );		// power up audio
	pSt.monoMode = (pSt.wavHdr->NbrChannels == 1);

	uint32_t ctrl = ARM_SAI_CONFIGURE_TX | ARM_SAI_MODE_SLAVE  | ARM_SAI_ASYNCHRONOUS | ARM_SAI_PROTOCOL_I2S | ARM_SAI_DATA_SIZE(16);
//	if ( pSt.monoMode )
//		ctrl |= ARM_SAI_MONO_MODE;		// request setting for mono data
	
	Driver_SAI0.Control( ctrl, 0, audioFreq );	// set sample rate, init codec clock, power up speaker and unmute
	
	pSt.bytesPerSample = pSt.wavHdr->NbrChannels * pSt.wavHdr->BitPerSample/8;  // = 4 bytes per stereo sample -- same as ->BlockAlign
	pSt.nSamples = pSt.wavHdr->SubChunk2Size / pSt.bytesPerSample;
	pSt.msecLength = pSt.nSamples*1000 / pSt.samplesPerSec;
	
	pSt.cBuff = NULL;
	// TestFileRead();
	pSt.state = pbPlayFill;
	pSt.nBuff = loadBuff();		// load first buffer as next to play
	pSt.state = pbFull;
	pSt.tsPlay = tbTimeStamp();		// start of playing
	pSt.tsResume = pSt.tsPlay;		// start of current play also

	dbgLog( "Wv: %d msec\n", pSt.msecLength );
	PlayNext();		// start 1st buffer playing
}
//void 								audPlaybackDn( ){								// process buffer complete events during playback
void 								audBufferDn( ){								// process buffer complete events during playback
	pnRes_t pn = PlayNext();
	if ( pn==pnPlaying ){
			gSet( gGREEN, 1 );	// Turn ON green LED: audio file playing 
	} else if ( pn==pnPaused ){		// paused
			gSet( gGREEN, 0 );	// Turn OFF LED green: while paused  
			ak_SetMute( true );	// (redundant) mute
			// subsequent call to audPauseResumeAudio() will call PlayNext() & un-mute
	} else if ( pn==pnDone ){											// playing complete
			gSet( gGREEN, 0 );	// Turn OFF LED green: no longer playing  
			Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 0, 0 );	// shut down I2S device

	//		Driver_SAI0.PowerControl( ARM_POWER_OFF );				// shut off I2S device entirely -- I2C problem
			Driver_SAI0.Uninitialize( );   

			Driver_SAI0.Initialize( &saiEvent );   
			Driver_SAI0.PowerControl( ARM_POWER_FULL );		// power up audio
	//		I2C_Reinit( 1 );							// try an I2C SWRST before reconnecting
		
			ak_SpeakerEnable( false ); 		// power down codec & amplifier
			sendEvent( AudioDone, audPlayPct() );				// end of file playback-- generate CSM event 
			dbgLog("Wv dn \n");
			if ( pSt.ErrCnt > 0 ) dbgLog( "%s audio Errs, Lst=0x%x \n", pSt.ErrCnt, pSt.LastError );
			pSt.stats->Finish++;
			pSt.state = pbIdle;
			return;
	}
}

pnRes_t 						PlayNext(){ 										// start next transter or return false if out of data
	if ( pSt.cBuff != NULL )
		freeBuff( pSt.cBuff );		// free buffer that finished		
	
	pSt.cBuff = pSt.nBuff;   		// next buff becomes current
	Buffer_t *pB = pSt.cBuff;
	if ( pB->state != bFull )
		errLog( "audio data not ready: %d", pB->state );
		
	if ( pB->cntBytes == 0 ){ 	// at EOF?
		freeBuff( pB );		// free empty buffer	
		pSt.state = pbDone;
		return pnDone;
	}
	
	pSt.nToPlay = pB->cntBytes / pSt.bytesPerSample;
	if ( pSt.reqPause ){
		pSt.reqPause = false;
		pSt.tsPause = tbTimeStamp();	// time we actually paused
		pSt.msPlayBefore += (pSt.tsPause - pSt.tsResume);	// elapsed since last Play
		if ( pSt.reqStop ){
			pSt.reqStop = false;
			freeBuff( pB );			// free buffer, since not coming back
			pSt.state = pbDone;
			pSt.stats->Left++;	// stopped in middle
			int pct = audPlayPct();
			if ( pct < pSt.stats->LeftMinPct ) pSt.stats->LeftMinPct = pct;
			if ( pct > pSt.stats->LeftMaxPct ) pSt.stats->LeftMaxPct = pct;
			pSt.stats->LeftSumPct += pct;
			return pnDone;
			
		} else {
			pSt.state = pbPaused;
			pSt.cBuff = NULL;   // looks like initial start, cBuff null, nBuff full
			return pnPaused;
		}
	}
	pSt.state = pbPlaying;
	pSt.buffNum++;	
	pB->state = bPlaying;
	Driver_SAI0.Send( pB->data, pSt.nToPlay );
	
	pSt.nPlayed += pSt.nToPlay;						// update samples played (playing)
	pSt.state = pbPlayFill;
	pSt.nBuff = loadBuff();			// read next chunk of file
	pSt.state = pSt.nBuff->cntBytes==0? pbLastPlay : pbFullPlay;
	return pnPlaying;
}
void 								saiEvent( uint32_t event ){			// called on transfer complete or error -- send message to mediaPlayer thread
	if ( (event & ARM_SAI_EVENT_SEND_COMPLETE) != 0 )
		audBufferDn();
		//osEventFlagsSet( audioEventChan, CODEC_DATA_TX_DN );
	else {
		pSt.LastError = event;
		pSt.ErrCnt++;
		//errLog( "audio error: 0x%x", event );
	}
}

// end audio
