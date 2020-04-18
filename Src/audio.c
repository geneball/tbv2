// audio.c  -- playback & record functions called by mediaplayer

#include "audio.h"
#include "mediaPlyr.h"

const int WaveHdrBytes 			= 44;					// length of .WAV file header
const int SAMPLE_RATE_MIN 	=  8000;
const int SAMPLE_RATE_MAX 	= 96000;
const int BuffLen 					= 4096;				// in bytes
const int BuffWds						= BuffLen/2;  // in shorts
const int nBuffs 						= N_AUDIO_BUFFS;		// number of audio buffers in pool allocated in audio.c
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
	wav->SampleRate = 16000;
	pSt.sqrHfLen = wav->SampleRate / (hz*2);		// samples per half wave
	pSt.sqrWvPh = 0;														// start with beginning of LO
	pSt.sqrHi = 0xAAAA;
	pSt.sqrLo = 0x0001;
	pSt.sqrSamples = nsecs * wav->SampleRate;		// number of samples to send
	
	wav->NbrChannels = 1;
	wav->BitPerSample = 16;
	wav->SubChunk2Size =  pSt.sqrSamples * 2; // bytes of data 
}
void								audInitState( void ){													// set up playback State in pSt
	pSt.LastError = 0;
	pSt.ErrCnt = 0;
	pSt.state = pbOpening;
	pSt.buffNum = -1;
	pSt.nLoaded = 0;
	pSt.bytesPerSample = 0; 
	pSt.samplesPerSec = 0;
	pSt.nSamples = 0;
	pSt.msecLength = 0;
	pSt.tsOpen = tbTimeStamp();		// before fopen
	pSt.tsPlay = 0;
	pSt.tsPause = 0;
	pSt.tsResume = 0;
	pSt.msPlayed = 0;
	pSt.audioEOF = false;
	pSt.wavF = NULL;
  memset( pSt.wavHdr, 0x00, WaveHdrBytes );
	for ( int i=0; i<N_AUDIO_BUFFS; i++ )
		pSt.Buff[i] = NULL;
	
	// don't overwrite SqrWAVE fields
	if ( pSt.stats==NULL ){
		pSt.stats = tbAlloc( sizeof( MsgStats ), "stats" );
		memset( pSt.stats, 0, sizeof(MsgStats ));
	}
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
	
	uint32_t played = pSt.msPlayed + (now - pSt.tsResume);	// elapsed since last Play
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
	if ( pSt.state==pbPaused ){ // resuming
		pSt.tsResume = tbTimeStamp();
		pSt.stats->Resume++;
		ak_SetMute( false );		// unmute
		gSet( gGREEN, 1 );			// Turn ON green LED: audio file playing 
  	Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 2, 0 );	// resume I2S DMA -- 2 => Resume
		pSt.state = pbPlaying;

	} else { // pausing
		ak_SetMute( true );	// (redundant) mute
		pSt.tsPause = tbTimeStamp();
		pSt.msPlayed += (pSt.tsPause - pSt.tsResume);  // (tsResume == tsPlay, if 1st pause)
		gSet( gGREEN, 0 );	// Turn OFF LED green: while paused  
			// subsequent call to audPauseResumeAudio() will call PlayNext() & un-mute
		pSt.stats->Pause++;
  	Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 1, 0 );	// pause I2S DMA -- 1 => Pause
		pSt.state = pbPaused;
	}
}

void								audLoadBuffs(){																// called on mediaThread to preload audio data
		for ( int i=0; i<N_AUDIO_BUFFS; i++)	// pre-load any empty audio buffers
			if ( pSt.Buff[i]==NULL )
				pSt.Buff[i] = loadBuff();	
}
void 								audPlaybackComplete( void ){									// shut down after completed playback
	pSt.tsPause = tbTimeStamp();
	pSt.msPlayed += (pSt.tsPause - pSt.tsResume);  		// (tsResume == tsPlay, if never paused)
  int ms = (pSt.buffNum+1)*BuffWds*500/pSt.samplesPerSec;	// ms for #buffers of stereo @ rate
	int extra = pSt.msPlayed - ms;   // elapsed - calculated
	dbgLog( "%d extra msec\n", extra );
	int pct = audPlayPct();
	dbgEvt( TB_audDone, ms, pSt.msPlayed, pct,0 );

	gSet( gGREEN, 0 );	// Turn OFF LED green: no longer playing  
	sendEvent( AudioDone, pct );				// end of file playback-- generate CSM event 
	pSt.stats->Finish++;
	
	
//	ak_SpeakerEnable( false ); 												// power down codec internals & amplifier
	Driver_SAI0.PowerControl( ARM_POWER_OFF );				// shut off I2S & I2C devices entirely

	for (int i=0; i<N_AUDIO_BUFFS; i++)		// free any allocated audio buffers
		freeBuff( i );

	if ( pSt.ErrCnt > 0 ) 
		dbgLog( "%s audio Errs, Lst=0x%x \n", pSt.ErrCnt, pSt.LastError );
	pSt.state = pbIdle;
}

static uint32_t evtCnt = 0, succCnt=0;  //DEBUG

// internal functions
static bool firstPlay;
void 								initBuffs(){																	// create audio buffers
	firstPlay = true;
	for ( int i=0; i<nBuffs; i++ ){
		Buffer_t *pB = &audio_buffers[i];
		pB->state = bEmpty;
		pB->cntBytes = 0;
		pB->firstSample = 0;
		pB->data = (uint16_t *) tbAlloc( BuffLen, "audio buffer" );
		for( int j =0; j<BuffWds; j++ ) pB->data[j] = 0x33;
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
void 								freeBuff( int idx ){											// release Buff[idx] (unless NULL)
	Buffer_t * pB = pSt.Buff[ idx ];
	if ( pB==NULL ) return;
	pB->state = bEmpty;
	pB->cntBytes = 0;
	pB->firstSample = 0;
	pSt.Buff[ idx ] = NULL;
}
void 								printAudSt(){																	// DBG: display audio state
		uint32_t cTmStamp = tbTimeStamp();
		printf( "  pSt: 0x%08x \n ", (uint32_t) &pSt );
		printf( "  pSt.state: %d \n ", pSt.state );
	  if ( pSt.bytesPerSample != 0 ){
			printf( "  %d BpS @ %d \n ", pSt.bytesPerSample, pSt.wavHdr->SampleRate );
			printf( "  %d of %d \n ", pSt.nLoaded, pSt.nSamples );
			printf( "  %d + %d msec \n ", pSt.msPlayed,  cTmStamp - pSt.tsPlay );
		} else {
			printf( "  Open %d msec \n ", cTmStamp - pSt.tsOpen );
		}
}

Buffer_t * 					loadBuff( ){																	// read next block of audio into a buffer
	if ( pSt.audioEOF ) 
		return NULL;			// all data & padding finished
	
	Buffer_t *pB = allocBuff();
	pB->firstSample = pSt.nLoaded;
	pSt.nToPlay = BuffWds;			// will always play a full buffer -- padded if necessary
	
	int nSamp = 0;
	int len = pSt.monoMode? BuffWds/2 : BuffWds;		// leave room to duplicate if mono
	if ( pSt.SqrWAVE ){	//DEBUG: gen sqWav
		nSamp = pSt.sqrSamples > len? len : pSt.sqrSamples;
		pSt.sqrSamples -= nSamp;		// decrement SqrWv samples to go

		int phase = pSt.sqrWvPh;			// start from end of previous
		int val = phase > 0? pSt.sqrHi : pSt.sqrLo;
		for (int i=0; i<len; i++){ 
			pB->data[i] = val;
			phase--;
			if (phase==0)
				val = pSt.sqrLo;
			else if (phase==-pSt.sqrHfLen){
				phase = pSt.sqrHfLen;
				val = pSt.sqrHi;
			}
		}
		pSt.sqrWvPh = phase;		// next starts from here
	} 
	
	if ( !pSt.SqrWAVE ){	//NORMAL case
		nSamp = fread( pB->data, 1, len*2, pSt.wavF )/2;		// read up to len samples (1/2 buffer if mono)
		dbgEvt( TB_ldBuff, nSamp, ferror(pSt.wavF), pSt.buffNum, 0 );
	}
	if ( pSt.monoMode ){
		nSamp *= 2;
		for ( int i = nSamp-1; i>0; i-- )	// nSamp*2 stereo samples
			pB->data[i] = pB->data[ i>>1 ];   // len==2048:  d[2047] = d[1023], d[2046]=d[1023], d[2045] = d[1022], ... d[2]=d[1], d[1]=d[0] 
	}

	if ( nSamp < BuffWds ){ 				// room left in buffer: at EOF, so fill rest with zeros
		if ( nSamp <= BuffWds-MIN_AUDIO_PAD ){
			pSt.audioEOF = true;							// enough padding, so stop after this
			fclose( pSt.wavF );
			dbgEvt( TB_audClose, nSamp, 0,0,0);
			pSt.wavF = NULL;
		}
		
		for( int i=nSamp; i<BuffWds; i++ )		// clear rest of buffer -- can't change count in DoubleBuffer DMA
			pB->data[i] = 0;
	}
	pSt.buffNum++;
	pSt.nLoaded += nSamp;			// # loaded
	pB->cntBytes = BuffLen;		// always transmit complete buffer
	pB->state = bFull;
	return pB;
}
void testRead( const char *fname ){ //DEBUG: time reading all samples of file
//	if ( pSt.SqrWAVE ) return;
	
	dbgEvtD( TB_tstRd, fname, strlen(fname) );
	audInitState();
	pSt.state = pbLdHdr;
	uint32_t tsOpen = tbTimeStamp();
	
	pSt.wavF = fopen( fname, "r" );
	if ( pSt.wavF==NULL || fread( pSt.wavHdr, 1, WaveHdrBytes, pSt.wavF ) != WaveHdrBytes ) 
			errLog( "open wav failed" );
	pSt.samplesPerSec = pSt.wavHdr->SampleRate;
	pSt.bytesPerSample = pSt.wavHdr->NbrChannels * pSt.wavHdr->BitPerSample/8;  // = 4 bytes per stereo sample -- same as ->BlockAlign
	pSt.nSamples = pSt.wavHdr->SubChunk2Size / pSt.bytesPerSample;
	pSt.msecLength = pSt.nSamples*1000 / pSt.samplesPerSec;

	while ( !pSt.audioEOF ){
		pSt.Buff[0] = loadBuff();
		freeBuff(0);
		tbDelay_ms(100);
	}
	uint32_t tsClose = tbTimeStamp();
	fclose( pSt.wavF );
	dbgLog("tstRd %d in %d \n", pSt.nLoaded, tsClose-tsOpen );
	evtCnt = 0;
}

void 								PlayWave( const char *fname ){ 								// play the WAV file 
	dbgEvtD( TB_playWv, fname, strlen(fname) );

	audInitState();
//	chkDevState( "PlayWv", true );
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

if (PlayDBG&2) //PlayDBG: TABLE=x1 POT=x2 PLUS=x4 MINUS=x8 STAR=x10 TREE=x20-- if POT, use MASTER Mode
	ctrl = ARM_SAI_CONFIGURE_TX | ARM_SAI_MODE_MASTER  | ARM_SAI_ASYNCHRONOUS | ARM_SAI_PROTOCOL_I2S | ARM_SAI_DATA_SIZE(16);
		
	Driver_SAI0.Control( ctrl, 0, audioFreq );	// set sample rate, init codec clock, power up speaker and unmute
	
	pSt.bytesPerSample = pSt.wavHdr->NbrChannels * pSt.wavHdr->BitPerSample/8;  // = 4 bytes per stereo sample -- same as ->BlockAlign
	pSt.nSamples = pSt.wavHdr->SubChunk2Size / pSt.bytesPerSample;
	pSt.msecLength = pSt.nSamples*1000 / pSt.samplesPerSec;
	

	if ( !pSt.SqrWAVE && (PlayDBG & 1)){	//PlayDBG: TABLE=x1 POT=x2 PLUS=x4 MINUS=x8 STAR=x10 TREE=x20-- if TABLE (PlayDbg & 1), replace file data with sqrWv @440
		pSt.sqrSamples = pSt.nSamples;				// sqr wv for same length as file
		pSt.sqrHfLen = pSt.wavHdr->SampleRate / 880;	// 440Hz @ samplerate from file
		pSt.sqrWvPh = 0;														// start with beginning of LO
		pSt.sqrHi = 0xAAAA;
		pSt.sqrLo = 0x0001;
		pSt.SqrWAVE = true;   // send sqrWv instead of .wav data
	}

	pSt.state = pbPlayFill;
	audLoadBuffs();							// preload N_AUDIO_BUFFS of data
	pSt.state = pbFull;
	pSt.tsPlay = tbTimeStamp();		// start of playing
	pSt.tsResume = pSt.tsPlay;		// start of current play also

	dbgLog( "Wv: %d msec\n", pSt.msecLength );
	gSet( gGREEN, 1 );	// Turn ON green LED: audio file playing 
	pSt.state = pbPlaying;
chkDevState( "stWv", firstPlay );	firstPlay = false;
	
	Driver_SAI0.Send( pSt.Buff[0]->data, pSt.nToPlay );		// start first buffer 
	Driver_SAI0.Send( pSt.Buff[1]->data, pSt.nToPlay );		// & set up next buffer 

  // buffer complete for cBuff calls saiEvent, which:
  //   1) Sends Buff2 
	//   2) shifts buffs 1,2,..N to 0,1,..N-1
  //   3) signals mediaThread to refill empty slots
}
void 								saiEvent( uint32_t event ){			// called by ISR on buffer complete or error -- chain next, or report error
	dbgEvt( TB_saiEvt, event, 0,0,0);

if ( pSt.wavF!=0 && (PlayDBG & 0x1)){  //PlayDBG: TABLE=x1 POT=x2 PLUS=x4 MINUS=x8 STAR=x10 TREE=x20-- if TABLE read 1K from audio.wav
		const int BLEN = 1024;
		char tbuff[BLEN];
		  int cnt = fread( tbuff, 1, BLEN, pSt.wavF );
			if (cnt>0) succCnt++; 
		  dbgEvt( TB_dmaComp, cnt, ferror(pSt.wavF),0, 0 );
		}

	if ( (event & ARM_SAI_EVENT_SEND_COMPLETE) != 0 ){
		if ( pSt.Buff[2] != NULL ){		// have more data to send
			Driver_SAI0.Send( pSt.Buff[2]->data, pSt.nToPlay );		// set up next buffer 
			dbgEvt( TB_audSent, pSt.Buff[2]->firstSample, 0,0,0);
			freeBuff( 0 );		// buff 0 completed, so free it
			for ( int i=0; i<N_AUDIO_BUFFS-1; i++ )	// shift buffer pointers down
			  pSt.Buff[i] = pSt.Buff[i+1];
			pSt.Buff[ N_AUDIO_BUFFS-1 ] = NULL; 
			// signal mediaThread to call audLoadBuffs() to preload empty buffer slots
			dbgEvt( TB_saiTXDN, 0, 0, 0, 0 );
			osEventFlagsSet( mMediaEventId, CODEC_DATA_TX_DN );

		} else if ( pSt.audioEOF ){  // done, close up shop
			pSt.state = pbDone;
			pSt.tsPause = tbTimeStamp();		// timestamp end of playback
			Driver_SAI0.Control( ARM_SAI_ABORT_SEND, 0, 0 );	// shut down I2S device, arg1==0 => Abort

			dbgEvt( TB_saiPLYDN, 0, 0, 0, 0 );
			osEventFlagsSet( mMediaEventId, CODEC_PLAYBACK_DN );	// signal mediaThread for rest
	//		audPlaybackComplete();		// => pbDone
		} else {
			tbErr( "Audio underrun\n" );
		}
	} else {
		pSt.LastError = event;
		pSt.ErrCnt++;
		//errLog( "audio error: 0x%x", event );
	}
}


// end audio
