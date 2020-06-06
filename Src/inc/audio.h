#ifndef AUDIO_H
#define AUDIO_H

#include "main.h"
#include "Driver_SAI.h"
#include "ak4xxx.h"

#include "rl_fs_lib.h"

extern ARM_DRIVER_SAI 			Driver_SAI0;			// from I2S_stm32F4xx.c

// constants defined in mediaplayer.c & referenced in audio.c
extern const int 						CODEC_DATA_TX_DN; 	// signal sent by SAI callback when buffer TX done
extern const int 						CODEC_PLAYBACK_DN; 	// signal sent by SAI callback when playback complete
extern const int 						CODEC_DATA_RX_DN; 	// signal sent by SAI callback when a buffer has been filled
extern const int 						CODEC_RECORD_DN; 	  // signal sent by SAI callback when recording stops
extern const int 						MEDIA_PLAY_EVENT;
extern const int 						MEDIA_RECORD_START;

typedef struct{				// WAV file header
  uint32_t   ChunkID;       // 0  == 'RIFF'		RIFF_ID = 0x46464952;
  uint32_t   FileSize;      // 4  
  uint32_t   FileFormat;    // 8  == 'WAVE'		WAVE_ID = 0x45564157;
  uint32_t   SubChunk1ID;   // 12 == 'fmt ' 	FMT_ID  = 0x20746D66;
  uint32_t   SubChunk1Size; // 16 == 16
  uint16_t   AudioFormat;   // 20 == 16?
  uint16_t   NbrChannels;   // 22 == 1  
  uint32_t   SampleRate;    // 24 
  
  uint32_t   ByteRate;      // 28  
  uint16_t   BlockAlign;    // 32   
  uint16_t   BitPerSample;  // 34    
  uint32_t   SubChunk2ID;   // 36 == 'data'  DATA_ID = 0x61746164;  
  uint32_t   SubChunk2Size; // 40     
	uint8_t    Data;					// @44

} WAVE_FormatTypeDef;

typedef enum {			// audType
  audUNDEF,
	audWave,
	audMP3,
	audOGG
} audType_t;

typedef enum {  			// pnRes_t  					-- return codes from PlayNext
	pnDone,
	pnPaused,
	pnPlaying
} pnRes_t;
typedef enum {				// BuffState					-- audio buffer states
	bFree, bAlloc, bEmpty, bFull, bDecoding, bPlaying, bRecording, bRecorded
} BuffState;

typedef struct { 			// Buffer_t						-- audio buffer
	BuffState state;
	int firstSample;
	int cntBytes;
	uint16_t * data;		// buffer of 16bit samples
} Buffer_t;

#define N_AUDIO_BUFFS			8
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
	pbPaused,
	
	pbWroteHdr,	// recording, wrote header
	pbRecording,	// started recording
	pbRecStop,		// record stop requested
	pbRecPaused		// recording paused -- NYI
	
} playback_state_t;

typedef struct { 			// PlaybackFile_t			-- audio state block
	audType_t 						audType;				// file type to playback
	WAVE_FormatTypeDef* 	wavHdr;					// WAVE specific info
	
  FILE * 								audF;						// stream for data
	
	uint32_t 							samplesPerSec;	// sample frequency 
	uint32_t 							bytesPerSample;	// eg. 4 for 16bit stereo
	uint32_t 							nSamples;				// total samples in file
	uint32_t							msecLength;			// length of file in msec

	uint32_t 							tsOpen;					// timestamp at start of fopen
	uint32_t 							tsPlay;					// timestamp at start of curr playback or resume
	uint32_t 							tsPause;				// timestamp at pause
	uint32_t 							tsResume;				// timestamp at resume (or start)
	uint32_t							tsRecord;				// timestamp at recording start
	
	playback_state_t  		state;					// current (overly detailed) playback state
	bool 									monoMode;				// true if data is 1 channel (duplicate on send)
	uint32_t 							nPerBuff;				// samples per buffer (always stereo)
	uint32_t 							LastError;			// last audio error code
	uint32_t 							ErrCnt;					// # of audio errors
	MsgStats *						stats;					// statistics to go to logger

	Buffer_t *						Buff[ N_AUDIO_BUFFS ];			// pointers to playback/record buffers (for double buffering)
	Buffer_t *						SvBuff[ N_AUDIO_BUFFS ];		// pointers to save buffers (waiting to write to file)
	
	// playback 
	int32_t								buffNum;				// idx in file of buffer currently playing
	uint32_t 							nLoaded;				// samples loaded from file so far
	uint32_t 							nPlayed;				// # samples in completed buffers
	uint32_t 							msPlayed;				// elapsed msec playing file
	bool 									audioEOF;				// true if all sample data has been read
//	uint32_t 							msPos;					// msec position in file
	
	// recording
	uint32_t 							msRecorded;			// elapsed msec recording file
	uint32_t 							nSaved;					// recorded samples sent to file so far

	// simulated square wave data
	bool 									SqrWAVE;				// T => wavHdr pre-filled to generate square wave
	int32_t								sqrSamples;			// samples still to send
	uint32_t 							sqrHfLen;				// samples per half wave
	int32_t 							sqrWvPh;				// position in wave form: HfLen..0:HI 0..-HfLen: LO
	uint16_t 							sqrHi;					//  = 0x9090;
	uint16_t 							sqrLo;					//  = 0x1010;
	
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
extern void 				audRequestRecStop( void );										// signal record loop to stop
extern void 				audPauseResumeAudio( void );								// signal playback loop to request Pause or Resume
extern void 				audPlaybackDn( void );											// handle end of buffer event
extern void 				audSquareWav( int nsecs, int hz );				  // square: 'nsecs' seconds of 'hz' squareWave
extern void					audLoadBuffs( void );												// pre-load playback data (from mediaThread)
extern void 				audSaveBuffs( void );												// save recorded buffers (called from mediaThread)
extern void 				audPlaybackComplete( void );								// playback complete (from mediaThread)
extern void 				audRecordComplete( void );									// last buff recorded, finish save

//DEBUG
extern void 				playWave( const char *fname ); // start playing from file

#endif /* AUDIO_H */
