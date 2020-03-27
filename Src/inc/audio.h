#ifndef AUDIO_H
#define AUDIO_H

#include "main.h"
#include "Driver_SAI.h"
#include "ak4xxx.h"

#include "rl_fs_lib.h"

extern ARM_DRIVER_SAI 			Driver_SAI0;			// from I2S_stm32F4xx.c

// constants defined in mediaplayer.c & referenced in audio.c
extern const int 						CODEC_DATA_TX_DN; 	// signal sent by SAI callback when an buffer completes
extern const int 						MEDIA_PLAY_EVENT;
extern const int 						MEDIA_RECORD_START;

typedef struct{				// WAV file header
  uint32_t   ChunkID;       // 0  
  uint32_t   FileSize;      // 4  
  uint32_t   FileFormat;    // 8  
  uint32_t   SubChunk1ID;   // 12  
  uint32_t   SubChunk1Size; // 16  
  uint16_t   AudioFormat;   // 20  
  uint16_t   NbrChannels;   // 22   
  uint32_t   SampleRate;    // 24 
  
  uint32_t   ByteRate;      // 28  
  uint16_t   BlockAlign;    // 32   
  uint16_t   BitPerSample;  // 34    
  uint32_t   SubChunk2ID;   // 36     
  uint32_t   SubChunk2Size; // 40     
	uint8_t    Data;

} WAVE_FormatTypeDef;

typedef enum {  			// pnRes_t  					-- return codes from PlayNext
	pnDone,
	pnPaused,
	pnPlaying
} pnRes_t;
typedef enum {				// BuffState					-- audio buffer states
	bEmpty, bAlloc, bFull, bDecoding, bPlaying
} BuffState;

typedef struct { 			// Buffer_t						-- audio buffer
	BuffState state;
	int firstSample;
	int cntBytes;
	
	uint8_t * data;
} Buffer_t;
typedef enum {				// playback_state_t		-- audio playback state codes
	pbIdle, 		// not playing anything
	pbOpening,	// calling fopen 
	pbLdHdr,		// fread header
	pbGotHdr,		// fread hdr done
	pbFilling,	// fread 1st data
	pbFull, 		// fread data done
	pbPlaying, 	// I2S transfer started
	pbPlayFill,	// fread data under I2S transfer
	pbLastPlay,	// fread data got 0
	pbFullPlay,	// fread data under I2S transfer done
	pbDone,			// I2S transfer of audio done
	pbPaused
} playback_state_t;
typedef struct { 			// PlaybackFile_t			-- audio state block
	WAVE_FormatTypeDef* wavHdr;
	uint32_t bytesPerSample;	// eg. 4 for 16bit stereo
	uint32_t nSamples;				// total samples in file
	uint32_t msecLength;			// length of file in msec
	uint32_t samplesPerSec;		// sample frequency 
	int32_t  buffNum;					// idx of buffer currently playing

	uint32_t tsOpen;			// timestamp at start of fopen
	uint32_t tsPlay;			// timestamp at start of playback or resume
	uint32_t tsPauseReq;	// timestamp at pause request
	uint32_t tsPause;			// timestamp at pause I2S complete
	uint32_t tsResume;		// timestamp at resume I2S
	uint32_t msPlayBefore;	// msec played before pause

	uint32_t nPlayed;			// samples played
	uint32_t nToPlay;			// samples currently playing
	bool monoMode;
	bool reqPause;				// true if pause requested
	bool reqStop;					// true if stop requested
	playback_state_t  state;
	
	uint32_t LastError;		// last audio error code
	uint32_t ErrCnt;			// # of audio errors
	

	Buffer_t *cBuff;			// currently playing samples start
	Buffer_t *nBuff;			// currently playing samples start
  FILE * wavF;					// stream for data
	bool SqrWAVE;					// T => wavHdr & buffers pre-filled with 1KHz square wave
	int SqrBytes;
	MsgStats *stats;
	
} PlaybackFile_t;


// functions called from mediaplayer.c
extern void 				audInitialize( osEventFlagsId_t eventId ); 	// sets up SAI & gives it channel to signal
extern void					audPowerDown( void );												// shut down and reset everything
extern MediaState 	audGetState( void );												// => Ready or Playing or Recording
extern uint16_t 		audAdjVolume( int16_t adj );								// change volume by 'adj'  0..10
extern int32_t 			audPlayPct( void );													// => current playback pct of file
extern void 				audAdjPlayPos( int32_t adj );								// shift current playback position +/- by 'adj' seconds
extern void 				audAdjPlaySpeed( int16_t adj );							// change playback speed by 'adj' * 10%
extern void 				audPlayAudio( const char* audioFileName, MsgStats *stats ); // start playing from file
extern void 				audStopAudio( void );												// signal playback loop to stop
extern void 				audStartRecording( FILE *outFP, MsgStats *stats );	// start recording into file -- TBV2_REV1 version
extern void 				audStopRecording( void );										// signal record loop to stop
extern void 				audPauseResumeAudio( void );								// signal playback loop to request Pause or Resume
extern void 				audPlaybackDn( void );											// handle end of buffer event
extern void 				audSquareWav( int nsecs );									// preload wavHdr & buffers with 1KHz square wave
// forward decls for internal functions
extern void 				PlayWave( const char *fname ); 			// play a WAV file
extern pnRes_t 			PlayNext( void );									// start next I2S buffer transferring
extern void 				PauseResume( void );									// request pause at next buff complete
extern void 				saiEvent( uint32_t event );					// called to handle transfer complete

#endif /* AUDIO_H */
