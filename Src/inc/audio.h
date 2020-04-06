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
	uint16_t * data;		// buffer of 16bit samples
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
	WAVE_FormatTypeDef* 	wavHdr;
	uint32_t 							bytesPerSample;	// eg. 4 for 16bit stereo
	uint32_t 							nSamples;				// total samples in file
	uint32_t							msecLength;			// length of file in msec
	uint32_t 							samplesPerSec;	// sample frequency 
	int32_t								buffNum;				// idx in file of buffer currently playing

	uint32_t 							tsOpen;					// timestamp at start of fopen
	uint32_t 							tsPlay;					// timestamp at start of curr playback or resume
	uint32_t 							tsPause;				// timestamp at pause DMA
	uint32_t 							tsResume;				// timestamp at resume I2S
	uint32_t 							msPlayed;				// msec of file played

//	uint32_t 							nPlayed;				// samples played of file so far
	uint32_t 							nLoaded;				// samples loaded from file so far
	uint32_t 							nToPlay;				// samples currently playing
	bool 									monoMode;				// true if data is 1 channel (duplicate on send)
	bool 									audioEOF;				// true if all sample data has been read
	playback_state_t  		state;					// current (overly detailed) playback state
	
	uint32_t 							LastError;			// last audio error code
	uint32_t 							ErrCnt;					// # of audio errors
	
	Buffer_t *						Buff[3];				// pointers to buffers (for double buffering)
  FILE * 								wavF;						// stream for data
	
	bool 									SqrWAVE;				// T => wavHdr pre-filled to generate square wave
	int32_t								sqrSamples;			// samples still to send
	uint32_t 							sqrHfLen;				// samples per half wave
	uint32_t 							sqrWvPh;				// position in wave form: HfLen..0:HI 0..-HfLen: LO
	uint16_t 							sqrHi;					//  = 0x9090;
	uint16_t 							sqrLo;					//  = 0x1010;
	MsgStats *						stats;					// statistics to go to logger
	
} PlaybackFile_t;


// functions called from mediaplayer.c
extern void 				audInitialize( void ); 											// sets up I2S
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
extern void 				audSquareWav( int nsecs, int hz );				  // square: 'nsecs' seconds of 'hz' squareWave

//DEBUG
extern void 				PlayWave( const char *fname ); // start playing from file

#endif /* AUDIO_H */
