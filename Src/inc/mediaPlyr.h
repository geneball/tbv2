#ifndef MEDIAPLYR_H
#define MEDIAPLYR_H

#include "tbook.h"
#include "audio.h"			// interface definitions for audio.c

extern 	osEventFlagsId_t			mMediaEventId;		// event channel for signals to mediaThread

extern void			initMediaPlayer( void );			// init mediaPlayer & spawn thread to handle  playback & record requests
extern void			mediaPowerDown( void );
extern int 			playAudio( const char *fileName, MsgStats *stats );
extern void 		pauseResume( void );
extern void 		stop( void );
extern int			playPosition( void );
extern void 		adjPlayPosition( int sec );
extern void 		adjVolume( int adj );
extern void 		adjVolume( int adj );
extern void 		adjSpeed( int adj );
extern int			recordPosition( void );
extern MediaState	getStatus( void );
extern int			recordAudio( const char *fileName, MsgStats *stats );
extern void			stopRecording( void );
extern void			audLoadBuffs( void );					// pre-load playback data from mediaThread

#endif
