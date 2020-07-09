// TBook Rev2  logger.c  --  event logging module
//   Gene Ball  Apr2018

#include "tbook.h"
#include "log.h"

static FILE *		logF = NULL;			// file ptr for open log file
static int			totLogCh = 0;
static char			statFileNm[60];		// local storage for computing .stat filepaths
bool 						FirstSysBoot = false;	

const osMutexAttr_t 	logMutex = { "logF_lock", osMutexRecursive, NULL, 0 };
osMutexId_t						logLock;

// const int 			MAX_STATS_CACHED = 10;   // avoid warning:  #3170-D: use of a const variable in a constant expression is nonstandard in C
#define					MAX_STATS_CACHED 10
static int 			nRefs = 0;
static short		lastRef[ MAX_STATS_CACHED ];
static short		touched[ MAX_STATS_CACHED ];
static MsgStats	sStats[ MAX_STATS_CACHED ];
const int				STAT_SIZ = sizeof( MsgStats );
const char *		norEraseFile = "M0:/system/EraseNorLog.txt";			// flag file to force full erase of NOR flash
const char *		logFilePatt = "M0:/LOG/tbLog_%d.txt";			// file name of previous log on first boot

const int 			NOR_BUFFSZ = 260;		// PGSZ + at least one for /0
struct {				// NOR LOG state
	ARM_DRIVER_FLASH *	pNor;
	ARM_FLASH_INFO * 		pI;
	int32_t							currLogIdx;		// index of this log
	int32_t							logBase;			// address of start of log-- 1st page holds up to 64 start addresses-- so flash is wear leveled
	int32_t							Nxt;					// address of 1st erased byte of Nor flash
 	uint32_t 						PGSZ; 				// CONST bytes per page
 	uint32_t 						SECTORSZ; 		// CONST bytes per sector
 	uint8_t  						E_VAL; 				// CONST errased byte value 
 	int32_t  						MAX_ADDR; 		// CONST max address in flash
	char 								pg[ NOR_BUFFSZ ];
} NLg;


TBH_arr 				TBH;	//DEBUG -- accessible log history

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
static void			addHist( const char * s1, const char * s2 ){			// DEBUG:  prepend evtMsg to TBHist[] for DEBUG
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

char *					loadLine( char * line, char * fpath, fsTime *tm ){		// => 1st line of 'fpath'
	FILE *stF = fopen( fpath, "rb" );
	if ( stF == NULL ) return NULL;
	
	char * txt = fgets( line, 200, stF );
	fclose( stF );
	
	if ( txt == NULL ) return NULL;
	
	char *pRet = strchr( txt, '\r' );
	if ( pRet!=NULL ) *pRet = 0;
	
	fsFileInfo fAttr;
	fAttr.fileID = 0;
	fsStatus fStat = ffind( fpath, &fAttr );
	if ( fStat != fsOK ){
		dbgLog( "FFind => %d for %s \n", fStat, TBP[ pCSM_VERS ] );
		return txt;
	}
	if (tm!=NULL) 
		*tm = fAttr.time;
	return txt;
}
void 						writeLine( char * line, char * fpath ){
	FILE *stF = fopen( fpath, "wb" );
	if ( stF!=NULL ){
		int nch = fprintf( stF, "%s\n", line );
		int err = fclose( stF );
	  dbgEvt( TB_wrLnFile, nch, err, 0, 0 );
	}
}
bool						openLog( bool forRead ){
	fsFileInfo fAttr;
	fAttr.fileID = 0;
	fsStatus fStat = ffind( TBP[ pLOG_TXT ], &fAttr );
	if ( fStat != fsOK )
		dbgLog( "Log missing => %d \n", fStat );
	else
		dbgLog( "Log sz= %d \n", fAttr.size );

	if ( logF != NULL )
		errLog("openLog logF=0x%x ! \n", logF );
	logF = fopen( TBP[ pLOG_TXT ], forRead? "r" : "a" );		// append -- unless checking
	totLogCh = 0;
	if ( logF==NULL ){
		TBDataOK = false;
		errLog( "Log file failed to open: %s", TBP[ pLOG_TXT ] );
		return false;
	} 
	return true;
}
void						checkLog(){				//DEBUG: verify tbLog.txt is readable
	if ( !openLog( true )) return;  // err msg from openLog
	int linecnt = 0, charcnt = 0;
	char line[200];
	while ( fgets( line, 200, logF )!= NULL ){ 
		linecnt++;
		charcnt += strlen( line );
	}
	int err = fclose( logF );
  logF = NULL;
	dbgEvt( TB_chkLog, linecnt, charcnt, err,0);
	dbgLog( "checkLog: %d lns, %d chs \n", linecnt, charcnt );
}
void						closeLog(){
  if ( logF!=NULL ){
		int err1 = fflush( logF );
		int err2 = fclose( logF );
		dbgLog( "closeLog fflush=%d fclose=%d \n", err1, err2 );
		dbgEvt( TB_wrLogFile, totLogCh, err1,err2,0);
	}
	logF = NULL;
	//checkLog(); //DEBUG
}

void dateStr( char *s, fsTime dttm ){
	sprintf( s, "%d-%d-%d %02d:%02d", dttm.year, dttm.mon, dttm.day, dttm.hr, dttm.min );
}
void						logPowerUp( bool reboot ){											// re-init logger after reboot, USB or sleeping
	char line[200];
	char dt[30];
	fsTime verDt, bootDt;

	logF = NULL;
	//checkLog();	//DEBUG
	//if ( !openLog(false) ) return;
	
	char * boot = loadLine( line, TBP[ pBOOTCNT ], &bootDt ); 
	int bootcnt = 1;		// if file not there-- first boot
	if ( boot!=NULL ) sscanf( boot, " %d", &bootcnt );
	FirstSysBoot = (bootcnt==1);
	
	if ( FirstSysBoot ){  // FirstBoot after install
		bool eraseNor = fexists( norEraseFile );
		if ( eraseNor )  // if M0:/system/EraseNorLog.txt exists-- erase 
			eraseNorFlash( false );			// CLEAR nor & create a new log
		else {     // First Boot starts a new log (unless already done by erase)
			sprintf(line, logFilePatt, NLg.currLogIdx );
			copyNorLog( line );				// save final previous log
			initNorLog( true );				// restart with a new log 
		}
	}
	
	if ( reboot ){
		totLogCh = 0;			// tot chars appended
		if (logF!=NULL) fprintf( logF, "\n" );
		logEvt(   "REBOOT--------" );
		logEvtNS( "TB_V2", "Firmware", TBV2_Version );
		logEvtS(  "CPU",  CPU_ID );
		logEvtS(  "TB_ID",  TB_ID );
		loadTBookName();
		logEvtS(  "TB_NM",  TBookName );
		logEvtNI( "CPU_CLK", "MHz", SystemCoreClock/1000000 );
		logEvtNINI("BUS_CLKS", "APB2", APB2_clock, "APB1", APB1_clock );
	} else
		logEvt(   "RESUME--");
	
	logEvtNI( "BOOT", "cnt", bootcnt );
	dbgEvt( TB_bootCnt, bootcnt, 0,0,0);
	logEvtNININI( "NorLog", "Idx", NLg.currLogIdx, "Sz", NLg.Nxt-NLg.logBase, "FreeK", (NLg.MAX_ADDR-NLg.Nxt)/1000 );
	
	fsFileInfo fAttr;
	fAttr.fileID = 0;
	fsStatus fStat = ffind( TBP[ pCSM_VERS ], &fAttr );
	if ( fStat != fsOK ){
		logEvtNS( "TB_CSM", "missing", TBP[ pCSM_VERS ] );
		return;
	}

	char * status = loadLine( line, TBP[ pCSM_VERS ], &verDt );			// version.txt marker file exists-- when created?
	dateStr( dt, verDt );
	logEvtNSNS( "TB_CSM", "dt", dt, "ver", status );
		
	if ( FirstSysBoot ){  // FirstBoot after install
		logEvtNS( "setRTC", "DtTm", dt );
		setupRTC( verDt );			// init RTC and set to date/time from version.txt
	} else 					// read & report RTC value
		showRTC();
	
	//boot complete!  count it
	bootcnt++;
	sprintf( line, " %d", bootcnt );
	writeLine( line,  TBP[ pBOOTCNT ] );
}
void						logPowerDown( void ){															// save & shut down logger for USB or sleeping
	flushStats();	
	copyNorLog( "" );		// auto log name
	
	if ( logF==NULL ) return;
	logEvtNI( "WrLog", "nCh", totLogCh );
	closeLog();
}

void						loggerThread(){
}
void						initLogger( void ){																// init tbLog file on bootup
	logLock = osMutexNew( &logMutex );
	initNorLog( false );				// init NOR & find current log 
	
	logPowerUp( true );
	
	//registerPowerEventHandler( handlePowerEvent );
	memset( lastRef, 0, (sizeof lastRef));
	memset( touched, 0, (sizeof touched));
	dbgLog( "Logger initialized \n" );
}
static char *		statFNm( const char * nm, short iS, short iM ){		// INTERNAL: fill statFileNm for  subj 'nm', iSubj, iMsg
	sprintf( statFileNm, "%s%s_S%d_M%d.stat", TBP[ pSTATS_PATH ], nm, iS, iM );
	return statFileNm;
}
int							fileMatchNext( const char *fnPatt ){			// return next unused value for file pattern of form "xxxx_*.xxx"
	fsFileInfo fAttr;
	int cnt = 0;
	fAttr.fileID = 0;
	while ( ffind( fnPatt, &fAttr ) == fsOK ){			// scan all files matching pattern
		char * pCnt = strrchr( fAttr.name, '_' )+1;   // ptr to digits after rightmost _
		short v = 0;
		while ( isdigit( *pCnt )){ 
			v = v*10 + (*pCnt-'0'); 
			pCnt++;
		}
		if ( v >= cnt ) cnt = v+1;  // result is 1 more than highest value found
	}
	return cnt;
}
char *					logMsgName( char *path, const char * sNm, short iSubj, short iMsg, const char *ext, int *pcnt ){			// build & => file path for next msg for Msg_<sNm>_S<iS>_M<iM>.<ext>
	char fnPatt[ MAX_PATH ]; 
	sprintf( fnPatt, "%sMsg_%s_S%d_M%d_*%s", TBP[ pMSGS_PATH ], sNm, iSubj, iMsg, ext );	// e.g. "M0:/messages/Msg_Health_S2_M3_*.txt"
	
	int cnt = fileMatchNext( fnPatt );
	sprintf( path, "%sMsg_%s_S%d_M%d_%d%s", TBP[ pMSGS_PATH ], sNm, iSubj, iMsg, cnt, ext );
	*pcnt = cnt;
	return path;
}
void 						saveStats( MsgStats *st ){ 												// save statistics block to file
	char * fnm = statFNm( st->SubjNm, st->iSubj, st->iMsg );
	FILE *stF = fopen( fnm, "wb" );
	int cnt = fwrite( st, STAT_SIZ, 1, stF );
	int err = fclose( stF );
	dbgEvt(TB_wrStatFile, st->iSubj, st->iMsg, cnt,err);
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
	logEvtNI( "SvStats", "cnt", cnt );
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
	// not loaded -- load into LRU block	
	MsgStats * st = &sStats[ oldestIdx ];	
	if ( touched[ oldestIdx ] != 0 ) {
		saveStats( &sStats[ oldestIdx ] );		// save it, if occupied
		touched[ oldestIdx ] = 0; 
	}
	
	short newStatIdx = oldestIdx;
	char * fnm = statFNm( subjNm, iSubj, iMsg );
	FILE *stF = fopen( fnm, "rb" );
	if ( stF == NULL || fread( st, STAT_SIZ, 1, stF ) != 1 ){  // file not defined
		if ( stF!=NULL ){ 
			dbgLog("stats S%d, M%d size wrong \n", iSubj, iMsg ); 
			fclose( stF );
		}
		memset( st, 0, STAT_SIZ );		// initialize new block
		st->iSubj = iSubj;
		st->iMsg = iMsg;
		strncpy( st->SubjNm, subjNm, MAX_SUBJ_NM );
	} 
	else { // success--
		dbgEvt( TB_LdStatFile, iSubj,iMsg, 0,0);
		fclose( stF );		
	}
	lastRef[ newStatIdx ] = nRefs;
	touched[ newStatIdx ] = 1;
	return st;
}
// logEvt formats
void						logEvt( const char *evtID ){											// write log entry: 'EVENT, at:    d.ddd'
	logEvtS( evtID, "" );
}
void						norEvt( const char *s1, const char *s2 ){
	appendNorLog( s1 );
	appendNorLog( ", " );
	appendNorLog( s2 );
	appendNorLog( "\n" );
}
void						logEvtS( const char *evtID, const char *args ){		// write log entry: 'm.ss.s: EVENT, ARGS'
	int 		ts = tbTimeStamp();
	char 		evtBuff[ MAX_EVT_LEN1 ];
	int tsec = ts/100, sec = tsec/10, min = sec/60, hr = min/60;
	if ( hr > 0 )
		sprintf( evtBuff,  "%d.%02d.%02d.%d: %8s", hr, min %60, sec % 60, tsec % 10, evtID );
	else
		sprintf( evtBuff,  "%d.%02d.%d: %8s", min %60, sec % 60, tsec % 10, evtID );
	addHist( evtBuff, args );
	dbgLog( "%s %s\n", evtBuff, args );
	
	if ( osMutexAcquire( logLock, osWaitForever )!=osOK )	tbErr("logLock");
	norEvt( evtBuff, args );	// WRITE to NOR Log
	if ( logF!=NULL ){
		int nch = fprintf( logF, "%s, %s\n", evtBuff, args );
		if (nch < 0) 
			dbgLog("LogErr: %d \n", nch );
		else
			totLogCh += nch;
		int err = fflush( logF );
		
		dbgEvt( TB_flshLog, nch, totLogCh, err,0);
		if ( err<0 ) 
			dbgLog("Log flush err %d \n", err );
	}
	if ( osMutexRelease( logLock )!=osOK )	tbErr("logLock!");
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
void 						logEvtNSNINI(  const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, int val3 ){
	char args[300];
	sprintf( args, "%s: '%s', %s: %d, %s: %d", nm, val, nm2, val2, nm3, val3 );
	logEvtS( evtID, args );
}
void 						logEvtNSNININS( const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, int val3, const char *nm4, const char *val4 ){
	char args[300];
	sprintf( args, "%s: '%s', %s: %d, %s: %d, %s: '%s'", nm, val, nm2, val2, nm3, val3, nm4, val4 );
	logEvtS( evtID, args );
}
void 						logEvtNININI( const char *evtID, const char *nm,  int val, const char *nm2, int val2, const char *nm3, int val3 ){
	char args[300];
	sprintf( args, "%s: %d, %s: %d, %s: %d", nm, val, nm2, val2, nm3, val3 );
	logEvtS( evtID, args );
}
void						logEvtNSNI( const char *evtID, const char *nm, const char *val, const char *nm2, int val2 ){	// write log entry
	char args[300];
	sprintf( args, "%s: '%s', %s: %d", nm, val, nm2, val2 );
	logEvtS( evtID, args );
}
void						logEvtNSNINS( const char *evtID, const char *nm, const char *val, const char *nm2, int val2, const char *nm3, const char *val3 ){	// write log entry
	char args[300];
	sprintf( args, "%s: '%s', %s: %d, %s: '%s'", nm, val, nm2, val2, nm3, val3 );
	logEvtS( evtID, args );
}
void						logEvtNINI( const char *evtID, const char *nm, int val, const char *nm2, int val2 ){	// write log entry
	char args[300];
	sprintf( args, "%s: %d, %s: %d", nm, val, nm2, val2 );
	logEvtS( evtID, args );
}
void						logEvtNI( const char *evtID, const char *nm, int val ){			// write log entry: "EVENT, at:    d.ddd, NM: VAL "
	char args[300];
	sprintf( args, "%s: %d", nm, val );
	logEvtS( evtID, args );
}

//
// NOR-flash LOG
const char *					norTmpFile = "M0:/system/svLog.txt";
const char *					norLogPath = "M0:/LOG";
const int 						BUFFSZ = 260;   // 256 plus space for null to terminate string
const int							N_SADDR = 64;		// number of startAddresses in page0

void	*					NLogReadPage( uint32_t addr ){					// read NOR page at 'addr' into NLg.pg & return ptr to it
	uint32_t stat = NLg.pNor->ReadData( addr, NLg.pg, NLg.PGSZ );			// read page at addr into NLg.pg 
	if ( stat != NLg.PGSZ ) tbErr(" pNor read(%d) => %d", addr, stat );
	return (void *)NLg.pg;
}
void 						NLogWrite( uint32_t addr, const char *data, int len ){	// write len bytes at 'addr' -- MUST be within one page
	int stpg = addr/NLg.PGSZ, endpg = (addr + len-1)/ NLg.PGSZ;
	if ( stpg != endpg ) tbErr( "NLgWr a=%d l=%d", addr, len );
	
	uint32_t stat = NLg.pNor->ProgramData( addr, data, len );		
	if ( stat != ARM_DRIVER_OK ) tbErr(" pNor wr => %d", stat );
}
int 						nxtBlk( int addr, int blksz ){		// => 1st multiple of 'blksz' after 'addr'
	return (addr/blksz + 1) * blksz;
}
void						findLogNext(){															// sets NLg.Nxt to first erased byte in current log
	int nxIdx = -1;
	uint32_t norAddr = NLg.logBase;			// start reading pages of current log
	while ( norAddr < NLg.MAX_ADDR ){
		NLogReadPage( norAddr );
		if ( NLg.pg[ NLg.PGSZ-1 ] == NLg.E_VAL ){  // page isn't full-- find first erased addr
			for ( int i=0; i < NLg.PGSZ; i++ ){
				if ( NLg.pg[i]==NLg.E_VAL ){ // found first still erased location
					nxIdx = norAddr + i;
					dbgLog("flash: Nxt=%d \n", nxIdx );
					NLg.Nxt = nxIdx;
					return;
				}
			}
		}
		// page was full, read next
		norAddr += NLg.PGSZ;
	}
	tbErr("NorFlash Full");
	return;
}

void						findCurrNLog( bool startNewLog ){						// read 1st pg (startAddrs) & init NLg for current entry & create next if startNewLog
	uint32_t *startAddr = NLogReadPage( 0 );		// read array of N_SADDR start addresses

	int lFull; 					// find last non-erased startAddr entry
	for (int i=0; i<N_SADDR; i++)
		if ( startAddr[i]== 0xFFFFFFFF ){
			lFull = i-1;       // i is empty, so i-1 is full
			break;
		}
	if ( lFull < 0 ){ // entire startAddr page is empty (so entire NorFlash has been erased)
		uint32_t pg1 = NLg.PGSZ;		// first log starts at page 1
		NLogWrite( 0, (char *)&pg1, 4 );		// write first entry of startAddr[]
		NLg.logBase = pg1;			// start of log
		NLg.Nxt = pg1;					// & next empty location
		return;
	}
  NLg.currLogIdx = lFull;	
	NLg.logBase = startAddr[ lFull ];
	findLogNext();					// sets NLg.Nxt to first erased byte in current log

	if ( startNewLog ){    // start a new log at the beginning of the first empty page
		lFull++;
		if ( lFull >= N_SADDR ){
			int oldNxt = NLg.Nxt;
			eraseNorFlash( false );				// ERASE flash & start new log at beginning
			logEvtNINI( "ERASE Nor", "logIdx", lFull, "Nxt", oldNxt );
			return;  // since everything was set by erase
		}
		NLg.currLogIdx = lFull;	
		startAddr[ lFull ] =  nxtBlk( NLg.Nxt, NLg.SECTORSZ );
		NLogWrite( lFull*4, (char *)&startAddr[ lFull ], 4 );		// write lFull'th entry of startAddr[]

		NLg.logBase = startAddr[ lFull ];
		NLg.Nxt = NLg.logBase;						// new empty log, Nxt = base
	}
}
void 						eraseNorFlash( bool svCurrLog ){			// erase entire chip & re-init with fresh log (or copy of current)
  if ( NLg.pNor == NULL ) return;				// Nor not initialized

	if ( svCurrLog )
		copyNorLog( norTmpFile );
	
	for ( uint32_t addr = 0; addr < NLg.MAX_ADDR; addr += NLg.SECTORSZ ){
		int stat = NLg.pNor->EraseSector( addr );	
		if ( stat != ARM_DRIVER_OK ) tbErr(" pNor erase => %d", stat );
		
		NLogReadPage( addr );
		if ( NLg.pg[0] != NLg.E_VAL ) 
			dbgLog("ebyte at %x reads %x \n", addr, NLg.pg[0] );
	}
	
	//int stat = NLg.pNor->EraseChip( );	//DOESN'T WORK!
	//if ( stat != ARM_DRIVER_OK ) tbErr(" pNor erase => %d", stat );
	dbgLog(" NLog erased \n");
	
	findCurrNLog( true );				// creates a new empty currentLog
	if ( svCurrLog )
		restoreNorLog( norTmpFile );
}
void						initNorLog( bool startNewLog ){														// init driver for W25Q64JV NOR flash
	uint32_t stat;
	
	// init constants in NLg struct
	NLg.pNor 			= &Driver_Flash0;
	NLg.pI 				= NLg.pNor->GetInfo();
	NLg.PGSZ 			= NLg.pI->page_size;
	NLg.SECTORSZ 	= NLg.pI->sector_size;
	if ( NLg.PGSZ > BUFFSZ ) tbErr("NLog: buff too small");
	NLg.E_VAL	 		= NLg.pI->erased_value;
	NLg.MAX_ADDR 	= ( NLg.pI->sector_count * NLg.pI->sector_size )-1;
	
	stat = NLg.pNor->Initialize( NULL );
	if ( stat != ARM_DRIVER_OK ) tbErr(" pNor->Init => %d", stat );
	
	stat = NLg.pNor->PowerControl( ARM_POWER_FULL );
	if ( stat != ARM_DRIVER_OK ) tbErr(" pNor->Pwr => %d", stat );

	findCurrNLog( startNewLog );			// locates (and creates, if startNewLog ) current log-- sets logBase & Nxt
	
	int logSz = NLg.Nxt-NLg.logBase;
	dbgLog( "NorLog: cLogSz=%d NorFilled=%d%% \n", logSz, NLg.Nxt*100/NLg.MAX_ADDR );
	
	
/*		lg_thread_attr.name = "Log Erase Thread";	
	lg_thread_attr.stack_size = LOG_STACK_SIZE; 
	osThreadId_t lg_thread =  osThreadNew( NLogThreadProc, 0, &lg_thread_attr ); 
	if ( lg_thread == NULL ) 
		tbErr( "logThreadProc not created" );
*/
	
}
void						appendNorLog( const char * s ){									// append text to Nor flash
  if ( NLg.pNor == NULL ) return;				// Nor not initialized

	int len = strlen( s );
	
	if ( NLg.Nxt + len > NLg.MAX_ADDR ){		// NOR flash is full!
		int oldNxt = NLg.Nxt;
		eraseNorFlash( true );				// ERASE flash & reload copy of current log at front
		logEvtNINI( "ERASE Nor", "logIdx", NLg.currLogIdx, "Nxt", oldNxt );
		return;  // since everything was set by erase
	}

	int nxtPg = nxtBlk( NLg.Nxt, NLg.PGSZ ); 
	if ( NLg.Nxt+len > nxtPg ){ // crosses page boundary, split into two writes
		int flen = nxtPg-NLg.Nxt;		// bytes on this page
		NLogWrite( NLg.Nxt, s, flen );						// write rest of this page
		NLogWrite( nxtPg, s+flen, len-flen );			// & part on next page
	
	} else 
		NLogWrite( NLg.Nxt, s, len );
	
	NLg.Nxt += len;
}
void						copyNorLog( const char * fpath ){								// copy curr Nor log into file at path
  if ( NLg.pNor == NULL ) return;				// Nor not initialized

	char fnm[40];
	uint32_t stat, msec, tsStart = tbTimeStamp(), totcnt = 0;
	
	strcpy( fnm, fpath );
	if ( strlen( fnm )==0 ) // generate tmp log name
		sprintf( fnm, "%s/tbLog_%d_%d.txt", norLogPath, NLg.currLogIdx, NLg.Nxt );  // e.g. LOG/NLg0_82173.txt  .,. /NLg61_7829377.txt
	FILE * f = fopen( fnm, "w" );
	if ( f==NULL ) tbErr("cpyNor fopen err");
	
	for ( int p = NLg.logBase; p < NLg.Nxt; p+= NLg.PGSZ ){
		stat = NLg.pNor->ReadData( p, NLg.pg, NLg.PGSZ );
		if ( stat != NLg.PGSZ ) tbErr("cpyNor read => %d", stat );
		
		int cnt = NLg.Nxt-p;
		if ( cnt > NLg.PGSZ ) cnt = NLg.PGSZ;
		stat = fwrite( NLg.pg, 1, cnt, f );
		if ( stat != cnt ){ 
			dbgLog("cpyNor fwrite => %d  totcnt=%d", stat, totcnt );
			fclose( f );
			return;
		}
		totcnt += cnt;
	}
	fclose( f );
	msec = tbTimeStamp() - tsStart;
	dbgLog( "copyNorLog: %s: %d in %d msec \n", fpath, totcnt, msec );
}
void						restoreNorLog( const char * fpath ){								// copy file into current log
	FILE * f = fopen( fpath, "r" );
	if ( f==NULL ) tbErr("rstrNor fopen err");
	
	int cnt = fread( NLg.pg, 1, NLg.PGSZ, f );
	while ( cnt > 0 ){
		NLg.pg[cnt] = 0;					// turn into string-- writes one past PGSZ
		appendNorLog( NLg.pg );		// append string to log
		cnt = fread( NLg.pg, 1, NLg.PGSZ, f );
	}
	fclose( f );
}
//end logger.c

