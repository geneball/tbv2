// TBook Rev2  log.h  --  C event logger
//   Gene Ball  Apr2018
#ifndef LOG_H
#define LOG_H
#include "tbook.h"

#define	MAX_SUBJ_NM 	15

typedef struct {	// MsgStats -- stats for iSubj/iMsg
	short		iSubj;
	short 		iMsg;
	char		SubjNm[ MAX_SUBJ_NM ];
	
	short		Start;			// count of StartPlayback's
	short		Finish;
	short		Pause;
	short		Resume;
	short		JumpFwd;
	short		JumpBk;
	short		Left;					// count of abandoned playbacks (slept or went elsewhere)
	short		LeftMinPct;		// min pct played before pause
	short		LeftMaxPct;		// max pct played before pause
	short		LeftSumPct;		// sum of pcts played before pause
	short		RecStart;
	short		RecPause;
	short		RecResume;
	short		RecSave;
	short		RecLeft;		// count of recorded but never saved
} MsgStats;


//
// external functions
extern void			initLogger( void );												// init tbLog file on bootup
extern void			logPowerUp( bool reboot );								// re-init logger after USB or sleeping
extern char *		loadLine( char * line, char * fpath, fsTime *tm );		// => 1st 200 chars of 'fpath' & *tm to lastAccessed
extern void			logPowerDown( void );											// save & shut down logger for USB or sleeping
extern void			saveStats( MsgStats *st ); 										// save statistics block to file
extern char *		logMsgName( char *path, const char * sNm, short iSubj, short iMsg, const char *ext );	// build & => file path for next msg for Msg_<sNm>_S<iS>_M<iM>.<ext>
extern void			flushStats( void );												// save all cached stats files
extern MsgStats *	loadStats( const char *subjNm, short iSubj, short iMsg );	// load statistics snapshot for Subj/Msg
extern void			logEvt( const char *evtID );									// write log entry: 'EVENT, at:    d.ddd'
extern void			logEvtS( const char *evtID, const char *args );					// write log entry: 'EVENT, at:    d.ddd, ARGS'
extern void			logEvtNS( const char *evtID, const char *nm, const char *val );	// write log entry: "EVENT, at:    d.ddd, NM: 'VAL' "
extern void			logEvtNI( const char *evtID, const char *nm, int val );			// write log entry: "EVENT, at:    d.ddd, NM: VAL "
extern void			logEvtNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2 );	// write log entry
extern void			logEvtNSNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2, const char *nm3, const char *val3 );	// write log entry
extern void			logEvtNINI( const char *evtID, const char *nm, int val, const char *nm2, int val2 );	// write log entry
extern void			logEvtNSNI( const char *evtID, const char *nm, const char *val, const char *nm2, int val2 );	// write log entry

#endif  // log.h
