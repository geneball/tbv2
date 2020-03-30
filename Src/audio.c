// audio.c  -- playback & record functions called by mediaplayer

#include "audio.h"

const int WaveHdrBytes 			= 44;					// length of .WAV file header
const int SAMPLE_RATE_MIN 	=  8000;
const int SAMPLE_RATE_MAX 	= 96000;
const int BuffLen 					= 4096;				// in bytes
const int BuffWds						= BuffLen/2;  // in shorts
const int nBuffs 						= 3;					// number of audio buffers allocated in audio.c
const int MIN_AUDIO_PAD			= 37;					// minimum 0-padding samples at end of file, to clear codec pipeline before powering down

static PlaybackFile_t 			pSt;							// audio player state
static Buffer_t 						audio_buffers[ nBuffs ];
void 												dbgToggleVolume( void );		//DEBUG:

// forward decls for internal functions
void 								initBuffs( void );														// create audio buffers
Buffer_t * 					allocBuff( void );														// get a buffer for use
void 								freeBuff( int idx );													// release buffer[idx] (unless NULL)
void 								printAudSt( void );														// DBG: display audio state
Buffer_t * 					loadBuff( void );															// read next block of audio into a buffer
void 								audPlaybackComplete( void );									// shut down after completed playback
void 								PlayWave( const char *fname ); 								// play a WAV file
pnRes_t 						PlayNextBuff( void );													// start next I2S buffer transferring
void 								saiEvent( uint32_t event );										// called to handle transfer complete

// EXTERNAL functions
void 								audInitialize( void ){ 												// allocs buffers, sets up SAI
	pSt.state = pbIdle;
	initBuffs();
	pSt.wavHdr = (WAVE_FormatTypeDef*) tbAlloc( WaveHdrBytes, "wav file" );  // alloc space for header

	pSt.SqrWAVE = false;			// unless audSquareWav is called
	// Initialize I2S & register callback
  //	I2S_TX_DMA_Handler() overides DMA_STM32F10x.c's definition of DMA1_Channel5_Handler 
	//    calls I2S_TX_DMA_Complete, which calls saiEvent with DMA events
	Driver_SAI0.Initialize( &saiEvent );   
}
void 								audSquareWav( int nsecs, int hz ){						// preload wavHdr & buffers with 1KHz square wave
	pSt.SqrWAVE = true;			// set up to generate 'hz' square wave
	WAVE_FormatTypeDef *wav = pSt.wavHdr;
	wav->SampleRate = 8000;
	pSt.sqrWvLen = wav->SampleRate / hz;				// samples per wave
	pSt.sqrWvPh = 0;														// start with beginning of LO
	pSt.sqrHi = 0x9090;
	pSt.sqrLo = 0x1010;
	
	wav->NbrChannels = 1;
	wav->BitPerSample = 16;
	wav->SubChunk2Size =  nsecs * wav->SampleRate * 2; // bytes of data 
/*
	for ( int i=0; i<nBuffs; i++ ){	// preload audio_buffers with !KHz square wave @ 8KHz == (1,1,1,1,0,0,0,0)...
		Buffer_t *pB = &audio_buffers[i];
		int nwds = BuffLen/4;  				// # of 16bit words in just 1st half of buffer
		for ( int j=0; j<nwds; j++ ){ 
			pB->data[ j ] = 0;
			pB->data[ j + nwds ] = 0xFFFF;		
			if ( !gGet( gPLUS ))
				pB->data[ j ] = (j & 4)? 0x1010 : 0x9090;
		}
	}
	*/
}
void								audInitState( void ){													// set up playback State in pSt
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
	pSt.tsPause = 0;
	pSt.tsResume = 0;
	pSt.msPlayBefore = 0;
	pSt.audioEOF = false;
	// SqrWave fields don't get initialized
}
MediaState 					audGetState( void ){													// => Ready or Playing or Recording
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
int32_t 						audPlayPct( void ){														// => current playback pct of file
	uint32_t now = tbTimeStamp();	
	if ( pSt.state == pbDone ) return 100;	// completed
	
	uint32_t played = pSt.msPlayBefore + (now - pSt.tsResume);	// elapsed since last Play
	uint32_t totmsec = pSt.nSamples * 1000 / pSt.samplesPerSec;
	return played*100 / totmsec;
}
void 								audAdjPlayPos( int32_t adj ){									// shift current playback position +/- by 'adj' seconds
}
void 								audAdjPlaySpeed( int16_t adj ){								// change playback speed by 'adj' * 10%
}
void 								audPlayAudio( const char* audioFileName, MsgStats *stats ){ // start playing from file
	pSt.stats = stats;
	stats->Start++;
	
	if ( strcmp( &audioFileName[ strlen( audioFileName )-4 ], ".wav" ) == 0 ){
		
		PlayWave( audioFileName );
	}
}
void 								audStopAudio( void ){													// about playback 
	pSt.stats->Left++;		//TODO:
	int pct = audPlayPct();
	dbgLog( "aud Stop @ %d \n", pct );
	pSt.stats->LeftSumPct += pct;
	if ( pct < pSt.stats->LeftMinPct ) pSt.stats->LeftMinPct = pct;
	if ( pct > pSt.stats->LeftMaxPct ) pSt.stats->LeftMaxPct = pct;
	audPlaybackComplete();
}
void 								audStartRecording( FILE *outFP, MsgStats *stats ){	// start recording into file -- TBV2_REV1 version
}
void 								audStopRecording( void ){											// signal record loop to stop
}
void 								audPauseResumeAudio( void ){									// signal playback to request Pause or Resume
	if ( pSt.state==pbPaused ){
		pSt.tsResume = tbTimeStamp();
		pSt.stats->Resume++;
		ak_SetMute( false );		// unmute
		gSet( gGREEN, 1 );			// Turn ON green LED: audio file playing 
		//TODO: restart DMA
//		PlayNextBuff();					// continue with next buffer
  	Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 2, 0 );	// resume I2S DMA -- 2 => Resume
		
	} else {
		ak_SetMute( true );	// (redundant) mute
		pSt.tsPause = tbTimeStamp();
		pSt.msPlayBefore += (pSt.tsPause - pSt.tsResume);  // (tsResume == tsPlay, if 1st pause)
		//TODO: stop DMA, & setup for restart
		gSet( gGREEN, 0 );	// Turn OFF LED green: while paused  
			// subsequent call to audPauseResumeAudio() will call PlayNext() & un-mute
		pSt.stats->Pause++;
  	Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 1, 0 );	// pause I2S DMA -- 1 => Pause
	}
}

void 								audPlaybackComplete( void ){									// shut down after completed playback
	Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 0, 0 );	// shut down I2S device, arg1==0 => Abort
	Driver_SAI0.PowerControl( ARM_POWER_OFF );				// shut off I2S device entirely -- I2C problem
	ak_SpeakerEnable( false ); 		// power down codec & amplifier

	freeBuff( 0 );
	freeBuff( 1 );
	freeBuff( 2 );

	//	Driver_SAI0.Uninitialize( );   
//	Driver_SAI0.Initialize( &saiEvent );   
//	Driver_SAI0.PowerControl( ARM_POWER_FULL );		// power up audio
//		I2C_Reinit( 1 );							// try an I2C SWRST before reconnecting

	int pct = audPlayPct();
	sendEvent( AudioDone, pct );				// end of file playback-- generate CSM event 
	if ( pSt.ErrCnt > 0 ) 
		dbgLog( "%s audio Errs, Lst=0x%x \n", pSt.ErrCnt, pSt.LastError );
	pSt.state = pbIdle;
}

// internal functions
void 								initBuffs(){																	// create audio buffers
	for ( int i=0; i<nBuffs; i++ ){
		Buffer_t *pB = &audio_buffers[i];
		pB->state = bEmpty;
		pB->data = (uint16_t *) tbAlloc( BuffLen, "audio buffer" );
	}
}
Buffer_t * 					allocBuff(){																	// get a buffer for use
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
void 								freeBuff( int idx ){											// release buffer (unless NULL)
	if ( pSt.Buff[ idx ]==NULL ) return;
	pSt.Buff[ idx ]->state = bEmpty;
	pSt.Buff[ idx ] = NULL;
}
void 								printAudSt(){																	// DBG: display audio state
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

Buffer_t * 					loadBuff( ){																	// read next block of audio into a buffer
	if ( pSt.audioEOF ) 
		return NULL;			// all data & padding finished
	
	Buffer_t *pB = allocBuff();
	pB->firstSample = pSt.nPlayed;
	pSt.nToPlay = BuffWds;			// will always play a full buffer -- padded if necessary
	
	int nSamp = 0;
	int len = pSt.monoMode? BuffLen/2 : BuffLen;		// leave room to duplicate if mono
	if ( pSt.SqrWAVE ){	
		pSt.nSamples -= len;		// decrement SqrWv samples to go
		nSamp = pSt.nSamples>0? len : 0;
		int phase = pSt.sqrWvPh;			// start from end of previous
		int val = phase > 0? pSt.sqrHi : pSt.sqrLo;
		for (int i=0; i<len; i++){ 
			pB->data[i] = val;
			phase--;
			if (phase==0)
				val = pSt.sqrLo;
			else if (phase==-pSt.sqrWvLen){
				phase = pSt.sqrWvLen;
				val = pSt.sqrHi;
			}
		}
	} else {
		nSamp = fread( pB->data, 1, len, pSt.wavF )/2;		// read up to len bytes (1/2 buffer if mono)
	}
	if ( pSt.monoMode ){
		for ( int i=(nSamp*2)-1; i>0; i-- )	// nSamp*2 stereo samples
			pB->data[i] = pB->data[ i>>1 ];   // len==2048:  d[2047] = d[1023], d[2046]=d[1023], d[2045] = d[1022], ... d[2]=d[1], d[1]=d[0] 
	}

	if ( nSamp < BuffWds ){ 				// room left in buffer: at EOF, so fill rest with zeros
		if ( nSamp <= BuffWds-MIN_AUDIO_PAD )
			pSt.audioEOF = true;							// enough padding, so stop after this
		
		for( int i=nSamp; i<BuffWds; i++ )		// clear rest of buffer -- can't change count in DoubleBuffer DMA
			pB->data[i] = 0;
	}
	pB->cntBytes = BuffLen;		// always transmit complete buffer
	pB->state = bFull;
	return pB;
}
/*pnRes_t 						PlayNextBuff(){ 															// start next transter or return false if out of data
	// play next buffer as quickly as possible
	freeBuff( pSt.cBuff );		// free buffer that finished	
	Buffer_t *pB = pSt.nBuff;
	if( pSt.reqPause ){  // unless asked to pause
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
	} else if ( pSt.nBuff == NULL){   // or done
		pSt.state = pbDone;
		return pnDone;
	}	else {	
		if ( pB->state != bFull )
			errLog( "audio data not ready: %d", pB->state );
		
		Driver_SAI0.Send( pB->data, pSt.nToPlay );
	dbgToggleVolume();
		
		pSt.nToPlay = pB->cntBytes / pSt.bytesPerSample;
		pSt.cBuff = pSt.nBuff;   							// next buff becomes current
	
		pSt.state = pbPlaying;
		pSt.buffNum++;	
		pB->state = bPlaying;
	
		pSt.nPlayed += pSt.nToPlay;						// update samples played (playing)
		pSt.state = pbPlayFill;
		pSt.nBuff = loadBuff();								// read next chunk of file
		pSt.state = pSt.nBuff==NULL? pbLastPlay : pbFullPlay;
		return pnPlaying;
	}
} */
void 								PlayWave( const char *fname ){ 								// play the WAV file 
	audInitState();
	pSt.state = pbLdHdr;
  if ( !pSt.SqrWAVE ){	// open file, unless SqrWAVE
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
	Driver_SAI0.Control( ctrl, 0, audioFreq );	// set sample rate, init codec clock, power up speaker and unmute
	
	pSt.bytesPerSample = pSt.wavHdr->NbrChannels * pSt.wavHdr->BitPerSample/8;  // = 4 bytes per stereo sample -- same as ->BlockAlign
	pSt.nSamples = pSt.wavHdr->SubChunk2Size / pSt.bytesPerSample;
	pSt.msecLength = pSt.nSamples*1000 / pSt.samplesPerSec;
	
	//DEBUG: TestFileRead();
	pSt.state = pbPlayFill;
	pSt.Buff[0] = loadBuff();				// load first buffer into Buff0
	pSt.Buff[1] = loadBuff();				// & 2nd into Buff1 (waits till done)
	pSt.state = pbFull;
	pSt.tsPlay = tbTimeStamp();		// start of playing
	pSt.tsResume = pSt.tsPlay;		// start of current play also

	dbgLog( "Wv: %d msec\n", pSt.msecLength );
	gSet( gGREEN, 1 );	// Turn ON green LED: audio file playing 
	pSt.state = pbPlaying;
	Driver_SAI0.Send( pSt.Buff[0]->data, pSt.nToPlay );		// start first buffer 
	Driver_SAI0.Send( pSt.Buff[1]->data, pSt.nToPlay );		// & set up next buffer 

	pSt.Buff[2] = loadBuff();
  // buffer complete for cBuff calls saiEvent, which:
  //   1) Send Buff2 
	//   2) shifts buffs 1,2 to 0,1
  //   3) loads new Buff2
	
	//PlayNextBuff();								// start 1st buffer playing
}
void 								saiEvent( uint32_t event ){			// called by ISR on buffer complete or error -- chain next, or report error
	if ( (event & ARM_SAI_EVENT_SEND_COMPLETE) != 0 ){
		if ( pSt.Buff[2] != NULL ){		// have more data to send
			Driver_SAI0.Send( pSt.Buff[2]->data, pSt.nToPlay );		// set up next buffer 
			freeBuff( 0 );		// completed, so free buffer
			pSt.Buff[0] = pSt.Buff[1];
			pSt.Buff[1] = pSt.Buff[2];
			pSt.Buff[2] = loadBuff();
		} else {  // done, close up shop
			pSt.state = pbDone;
			audPlaybackComplete();		// => pbIdle
			gSet( gGREEN, 0 );	// Turn OFF LED green: no longer playing  
			sendEvent( AudioDone, audPlayPct() );				// end of file playback-- generate CSM event 
			pSt.stats->Finish++;
		}
	} else {
		pSt.LastError = event;
		pSt.ErrCnt++;
		//errLog( "audio error: 0x%x", event );
	}
}

// end audio
