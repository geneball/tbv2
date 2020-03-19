// TBook Rev2  logger.c  --  event logging module
//   Gene Ball  Apr2018

#include "tbook.h"
#include "log.h"

static FILE *		logF = NULL;			// file ptr for open log file
static char			statFileNm[60];		// local storage for computing .stat filepaths

// const int 			MAX_STATS_CACHED = 10;   // avoid warning:  #3170-D: use of a const variable in a constant expression is nonstandard in C
#define					MAX_STATS_CACHED 10
static int 			nRefs = 0;
static short		lastRef[ MAX_STATS_CACHED ];
static short		touched[ MAX_STATS_CACHED ];
static MsgStats	sStats[ MAX_STATS_CACHED ];
const int				STAT_SIZ = sizeof( MsgStats );

//#define 				MAX_TB_HIST 50
//typedef char *	TBH_arr[ MAX_TB_HIST*2 ];
TBH_arr 					TBH;

const int				MAX_EVT_LEN1 = 32, MAX_EVT_LEN2 = 64;
static void			initTBHistory(){
	for ( int i=0; i < MAX_TB_HIST; i++ ){
		TBH[i*2] = tbAlloc( MAX_EVT_LEN1, "log hist" );
		strcpy( TBH[i*2], "---" );
		TBH[i*2+1] = tbAlloc( MAX_EVT_LEN2, "log hist2" );
		TBH[i*2+1][0] = 0;
	}
	Dbg.TBookLog = &TBH;
}
static void			addHist( const char * s1, const char * s2 ){			// INTERNAL:  prepend evtMsg to TBHist[] for DEBUG
	if ( TBH[0]==NULL ) 
		initTBHistory();
	
	char * old1 = TBH[ (MAX_TB_HIST-1)*2 ];
	char * old2 = TBH[ (MAX_TB_HIST-1)*2+1 ];
	for ( int i=(MAX_TB_HIST-1); i>0; i-- ){
		TBH[i*2+1] = TBH[(i-1)*2+1];
		TBH[i*2] = TBH[(i-1)*2];
	}
	TBH[0] = old1;
	TBH[1] = old2;
	strcpy( old1, s1 );
	strcpy( old2, s2 );
}
void						logPowerUp( void ){																// re-init logger after USB or sleeping
	char line[200];
	logF = fopen( TBP[ pLOG_TXT ], "a" );		// open file for appending
	if ( logF==NULL ){
		TBDataOK = false;
		errLog( "Log file failed to open: %s", TBP[ pLOG_TXT ] );
	} else {
		logEvt( "\n\n------------" );
		logEvtNS( "TB_V2", "firmware", TBV2_Version );
		fsFileInfo fAttr;
		fAttr.fileID = 0;
		fsStatus fStat = ffind( TBP[ pSTATUS ], &fAttr );
		if ( fStat==fsOK ){		// status.txt marker file exists-- when created?
			char dt[30];
			fsTime tm = fAttr.time;
			sprintf( dt, "%d-%d-%d %d:%d", tm.year, tm.mon, tm.day, tm.hr, tm.min );
			logEvtNS( "TB_Install", "date", dt );
			FILE *stF = fopen( TBP[ pSTATUS ], "rb" );
			char * status = fgets( line, 200, stF );
			if ( status!=NULL ){ 
				char *pRet = strchr( status, '\r' );
				if ( pRet!=NULL ) *pRet = 0;
				logEvtNS( "TB_Status", "txt", status );
			}
			fclose( stF );
			
			stF = fopen( TBP[ pBOOTCNT ], "rb" );		// read or init bootcnt.txt
			int bootcnt;
			if ( stF==NULL || fscanf( stF, " %d", &bootcnt )!=1 )
				bootcnt = 1;
		  else {
				bootcnt++;
				fclose( stF );
			}
			stF = fopen( TBP[ pBOOTCNT ], "wb" );		// write updated bootcnt.txt
			if ( stF!=NULL ){
				fprintf( stF, " %d", bootcnt );
				fclose( stF );
		  }
			logEvtNI( "BootCnt", "#", bootcnt );
			
		} else {
			logEvtNS( "FileError", "missing", TBP[ pSTATUS ] );
		}
	}
}
void						logPowerDown( void ){															// save & shut down logger for USB or sleeping
	flushStats();	
	if ( logF==NULL ) return;
	logEvt( "PwrDown" );
#ifdef USE_FILESYS
	fflush( logF );
	fclose( logF );
#endif
	logF = NULL;
}
void						initLogger( void ){																// init tbLog file on bootup
	logPowerUp();
	
	//registerPowerEventHandler( handlePowerEvent );
	memset( lastRef, 0, (sizeof lastRef));
	memset( touched, 0, (sizeof touched));
	dbgLog( "Logger initialized \n" );
}
static char *		statFNm( const char * nm, short iS, short iM ){		// INTERNAL: fill statFileNm for  subj 'nm', iSubj, iMsg
	sprintf( statFileNm, "%s%s_S%d_M%d.stat", TBP[ pSTATS_PATH ], nm, iS, iM );
	return statFileNm;
}
char *					logMsgName( char *path, const char * sNm, short iSubj, short iMsg, const char *ext ){			// build & => file path for next msg for Msg_<sNm>_S<iS>_M<iM>.<ext>
	fsFileInfo fAttr;
	char fnPatt[ MAX_PATH ]; 
	sprintf( fnPatt, "%sMsg_%s_S%d_M%d_*.%s", TBP[ pMSGS_PATH ], sNm, iSubj, iMsg, ext );	// e.g. "M0:/messages/Msg_Health_S2_M3_*.txt"
	short cnt = 0;
	fAttr.fileID = 0;
	while ( ffind( fnPatt, &fAttr ) == fsOK ){
		char * pCnt = strrchr( fAttr.name, '_' )+1;
		short v = 0;
		while ( isdigit( *pCnt )){ 
			v = v*10 + (*pCnt-'0'); 
			pCnt++;
		}
		if ( v >= cnt ) cnt = v+1;
	}
	sprintf( path, "%sMsg_%s_S%d_M%d_%d.%s", TBP[ pMSGS_PATH ], sNm, iSubj, iMsg, cnt, ext );
	return path;
}
void 						saveStats( MsgStats *st ){ 												// save statistics block to file
	char * fnm = statFNm( st->SubjNm, st->iSubj, st->iMsg );
	FILE *stF = fopen( fnm, "wb" );
	fwrite( st, STAT_SIZ, 1, stF );
	fclose( stF );
}
void						flushStats(){																			// save all cached stats files
	short cnt = 0;
	for ( short i=0; i <  MAX_STATS_CACHED; i++ ){
		if ( touched[i] != 0 ){
			saveStats( &sStats[i] );
			touched[ i ] = 0;
			cnt++;
		}
	}
	logEvtNI( "StatsSaved", "cnt", cnt );
}
MsgStats *			loadStats( const char *subjNm, short iSubj, short iMsg ){		// load statistics snapshot for Subj/Msg
	int oldestIdx = 0;  // becomes idx of block with lowest lastRef
	nRefs++;
	for ( short i=0; i < MAX_STATS_CACHED; i++ ){
		if ( lastRef[i] > 0 && sStats[i].iSubj==iSubj && sStats[i].iMsg==iMsg ){
			lastRef[ i ] = nRefs;	// Most Recently Used
			touched[ i ] = 1;
			return &sStats[ i ];
		} else if ( lastRef[ i ] < lastRef[ oldestIdx ] )			
			oldestIdx = i;
	}	
	MsgStats * st = &sStats[ oldestIdx ];		// not loaded -- load into LRU block	
	if ( touched[ oldestIdx ] != 0 ) {
		saveStats( &sStats[ oldestIdx ] );		// save it, if occupied
		touched[ oldestIdx ] = 0; 
	}
	short newStatIdx = oldestIdx;
	
	char * fnm = statFNm( subjNm, iSubj, iMsg );
	FILE *stF = fopen( fnm, "rb" );
	if ( stF==NULL || fread( st, STAT_SIZ, 1, stF ) != STAT_SIZ ){  // file not defined
		memset( st, 0, STAT_SIZ );		// initialize new block
		st->iSubj = iSubj;
		st->iMsg = iMsg;
		strncpy( st->SubjNm, subjNm, MAX_SUBJ_NM );
	} 
	else 
		fclose( stF );
	lastRef[ newStatIdx ] = nRefs;
	touched[ newStatIdx ] = 1;
	return st;
}
void						logEvt( const char *evtID ){											// write log entry: 'EVENT, at:    d.ddd'
	logEvtS( evtID, "" );
}
void						logEvtS( const char *evtID, const char *args ){		// write log entry: 'EVENT, at:    d.ddd, ARGS'
	int 		ts = tbTimeStamp();
	char 		evtBuff[ MAX_EVT_LEN1 ];
	
	sprintf( evtBuff,  "%8s, at:%5d.%03d", evtID, ts/1000, ts%1000 );
	addHist( evtBuff, args );
	if ( logF==NULL ) return;
	dbgLog( "%s %s\n", evtBuff, args );
#ifdef USE_FILESYS
	fprintf( logF, "%s %s\n", evtBuff, args );
	fflush( logF );
#endif
}
void						logEvtNS( const char *evtID, const char *nm, const char *val ){	// write log entry: "EVENT, at:    d.ddd, NM: 'VAL' "
	char args[300];
	sprintf( args, "%s: '%s'", nm, val );
	logEvtS( evtID, args );
}
void						logEvtNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2 ){	// write log entry
	char args[300];
	sprintf( args, "%s: '%s', %s: '%s'", nm, val, nm2, val2 );
	logEvtS( evtID, args );
}
void						logEvtNSNSNS( const char *evtID, const char *nm, const char *val, const char *nm2, const char *val2, const char *nm3, const char *val3 ){	// write log entry
	char args[300];
	sprintf( args, "%s: '%s', %s: '%s', %s: '%s'", nm, val, nm2, val2, nm3, val3 );
	logEvtS( evtID, args );
}
void						logEvtNSNI( const char *evtID, const char *nm, const char *val, const char *nm2, short val2 ){	// write log entry
	char args[300];
	sprintf( args, "%s: '%s', %s: %d", nm, val, nm2, val2 );
	logEvtS( evtID, args );
}
void						logEvtNI( const char *evtID, const char *nm, int val ){			// write log entry: "EVENT, at:    d.ddd, NM: VAL "
	char args[300];
	sprintf( args, "%s: %d", nm, val );
	logEvtS( evtID, args );
}
//end logger.c

