// TBook Rev2  norLog.c  --  nor Flash logging utilities
//   Gene Ball  Oct2020
//     
//   operations on TBook Log written to NorFLASH memory  
//  W25Q64JV  TBookRev2 U5    datasheet: https://www.mouser.com/datasheet/2/949/w25q64jv_revj_03272018_plus-1489671.pdf
//  3V 64M-BIT  SERIAL FLASH MEMORY WITH DUAL, QUAD SPI
//
//	The W25Q64JV device is directly handled by W25Q64JV.c 
//  The driver sends commands to the chip over SPI, this file only access a few driver functions:
//     pNor->Initialize,  pNor->PowerControl, & pNor->GetInfo  -- from initNorLog()
//     pNor->ReadData						  -- from NLogReadPage() and copyNorLog()
//     pNor->ProgramData				  -- from NLogWrite()
//     pNor->EraseSector()  			-- from eraseNorFlash()
//
//  
//  the NOR-FLASH is organized as a collection of sequential text log files, each starting on a NOR page boundary
//  the 1st NOR page is reserved as an array of 64 start addresses -- Nor address for logs 0..63
//  erased NOR locations read as all 1's, so a non-zero log start_address means that log has been initialized
//
//-- initNorLog( bool startNewLog )
//    initializes NLg structure,
//      NLg.pNor points to ARMDRIVER Driver_Flash0 (defined by ARM_Driver_Flash_(DRIVER_FLASH_NUM) in W25Q64JV.c)
//      NLg.pI points to key NORFLASH parameters that are defined in W25Q64JV.h 
//     then calls findCurrNLog()
//       which finds current log index, base address, and next empty byte
//       by scanning the page of log start_addresses (in page0): 
//        if s_a[0] is erased, -- entire flash is erased, so initialize Idx=0, Base=page1, Nxt=Base
//        otherwise, Idx & Base are set from the last used address, then findLogNext()
//            sets Nxt to first still erased byte beyond Base  (calls TbErr if entire NOR flash is full)
//        if startNewLog has been requested, index is incremented,
//            Base & Nxt are set to next empty page, and page0 start_address[idx] is written with Base 
//        if currLog==63, startNewLog forces a full erase of the NORflash and starts Log0
//-- appendNorLog( text ) 
//   writes text to NOR flash, starting at NLg.Nxt 
//      splits into two calls to NLogWrite if data crosses into next page
//-- copyNorLog( path ) -- writes contents of current log to specified file path
//-- eraseNorFlash( saveCurrLog ) -- erases entire NOR flash, a page at a time, verifying that byte0 was reset
//       (page at a time, which is slow, because pNor->eraseChip() didn't work)
//       if saveCurrLog was requested, writes the current log before erasing, 
//          then appends the file to log0 afterward

#include "tbook.h"

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


//
// NOR-flash LOG
const char *					norTmpFile = "M0:/system/svLog.txt";
const char *					norLogPath = "M0:/LOG";
const int 						BUFFSZ = 260;   // 256 plus space for null to terminate string
const int							N_SADDR = 64;		// number of startAddresses in page0

void						NLogShowStatus(){														// write event with NorLog status details
		logEvtNININI( "NorLog", "Idx", NLg.currLogIdx, "Sz", NLg.Nxt-NLg.logBase, "Free%", (NLg.MAX_ADDR-NLg.Nxt)*100/NLg.MAX_ADDR );
}
int							NLogIdx(){  																// => index of log section currently in use
		return NLg.currLogIdx;
}
void	*					NLogReadPage( uint32_t addr ){							// read NOR page at 'addr' into NLg.pg & return ptr to it
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
int 						nxtBlk( int addr, int blksz ){							// => 1st multiple of 'blksz' after 'addr'
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
void 						eraseNorFlash( bool svCurrLog ){						// erase entire chip & re-init with fresh log (or copy of current)
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
void						initNorLog( bool startNewLog ){							// init driver for W25Q64JV NOR flash
	uint32_t stat;
	
	// init constants in NLg struct
	NLg.pNor 			= &Driver_Flash0;
	NLg.pI 				= NLg.pNor->GetInfo();   // get key NORFLASH parameters (defined in W25Q64JV.h)
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
		if ( flen > NLg.PGSZ ) tbErr("append too long");
		
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
		sprintf( fnm, "%s/tbLog_%d.txt", norLogPath, NLg.currLogIdx );  // e.g. LOG/tbLog_x.txt  .,. /NLg61_7829377.txt
	FILE * f = tbOpenWrite( fnm ); //fopen( fnm, "w" );
	if ( f==NULL ) tbErr("cpyNor fopen err");
	
	for ( int p = NLg.logBase; p < NLg.Nxt; p+= NLg.PGSZ ){
		stat = NLg.pNor->ReadData( p, NLg.pg, NLg.PGSZ );
		if ( stat != NLg.PGSZ ) tbErr("cpyNor read => %d", stat );
		
		int cnt = NLg.Nxt-p;
		if ( cnt > NLg.PGSZ ) cnt = NLg.PGSZ;
		stat = fwrite( NLg.pg, 1, cnt, f );
		if ( stat != cnt ){ 
			dbgLog("cpyNor fwrite => %d  totcnt=%d", stat, totcnt );
			tbCloseFile( f );		//fclose( f );
			return;
		}
		totcnt += cnt;
	}
	tbCloseFile( f );		//fclose( f );
	msec = tbTimeStamp() - tsStart;
	dbgLog( "copyNorLog: %s: %d in %d msec \n", fpath, totcnt, msec );
}
void						restoreNorLog( const char * fpath ){								// copy file into current log
	FILE * f = tbOpenRead( fpath ); //fopen( fpath, "r" );
	if ( f==NULL ) tbErr("rstrNor fopen err");
	
	int cnt = fread( NLg.pg, 1, NLg.PGSZ, f );
	while ( cnt > 0 ){
		NLg.pg[cnt] = 0;					// turn into string-- writes one past PGSZ
		appendNorLog( NLg.pg );		// append string to log
		cnt = fread( NLg.pg, 1, NLg.PGSZ, f );
	}
	tbCloseFile( f );		//fclose( f );
}
//end logger.c

