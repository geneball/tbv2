// TBook Rev2  log.h  --  C event logger
//   Gene Ball  Apr2018
#ifndef LOG_H
#define LOG_H
#include "tbook.h"
#include "Driver_Flash.h"
#include "Driver_SPI.h"

extern ARM_DRIVER_FLASH Driver_Flash0;

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
extern char *		logMsgName( char *path, const char * sNm, short iSubj, short iMsg, const char *ext, int *pcnt );	// build & => file path for next msg for Msg_<sNm>_S<iS>_M<iM>.<ext>
extern void			flushStats( void );												// save all cached stats files
extern MsgStats *	loadStats( const char *subjNm, short iSubj, short iMsg );	// load statistics snapshot for Subj/Msg
extern void			logEvt( const char *evtID );									// write log entry: 'EVENT, at:    d.ddd'
extern void			logEvtS( const char *evtID, const char *args );					// write log entry: 'EVENT, at:    d.ddd, ARGS'
extern void     logEvtFmt( const char *EventID, const char *fmt, ...); // write log entry, any arguments
extern void			logEvtNI( const char *evtID, const char *nm, int val );			// write log entry: "EVENT, at:    d.ddd, NM: VAL "
extern void			logEvtNINI( const char *evtID, const char *nm, int val, const char *nm2, int val2 );	// write log entry
extern void 		logEvtNININI( const char *evtID, const char *nm,  int val, const char *nm2, int val2, const char *nm3, int val3 );
extern void			logEvtNS( const char *evtID, const char *nm, const char *val );	// write log entry: "EVENT, at:    d.ddd, NM: 'VAL' "
extern void			logEvtNINS( const char *evtID, const char *nm, int val, const char *nm2, const char * val2 );	// write log entry
extern void			logEvtNSNI( const char *evtID, const char *nm, const char *val, const char *nm2, int val2 );	// write log entry
extern void 		logEvtNSNINI(  const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, int val3 );
extern void 		logEvtNSNININS(  const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, int val3, const char *nm4, const char *val4 );
extern void			logEvtNSNINS( const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, const char *val3 );	// write log entry
extern void			logEvtNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2 );	// write log entry
extern void			logEvtNSNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2, const char *nm3, const char *val3 );	// write log entry

extern void			appendNorLog( const char * s );						// append text to Nor flash
extern void			copyNorLog( const char * fpath );					// copy Nor log into file at path
extern void			restoreNorLog( const char * fpath );			// copy file into current log
extern void			initNorLog( bool startNewLog );						// init driver for W25Q64JV NOR flash
extern void			eraseNorFlash( bool svCurrLog );					// erase entire chip & re-init with fresh log (or copy of current)
extern void			NLogShowStatus( void );
extern int			NLogIdx( void );

#endif  // log.h
