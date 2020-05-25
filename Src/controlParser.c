// TBookV2  controlParser.c
//   Gene Ball  May2018

#include "tbook.h"
#include "controlMgr.h"


// ---------------   parse  /system/control.def  &  /package/list_of_subjects.txt  & /package/SUBJ*/messages.txt 
static void		pathCat( char *path, const char * src ){  // strcat & convert '\' to '/'
	short plen = strlen( path );
	short slen = strlen( src );
	for( short i=0; i<slen; i++ )
		path[ plen+i ] = src[i]=='\\'? '/' : src[i];
	path[ plen+slen ] = 0;
}
static void		appendIf( char * path, const char *suffix ){	// append 'suffix' if not there
	short len = strlen( path );
	short slen = strlen( suffix );
	if ( len<slen || strcasecmp( &path[len-slen], suffix )!=0 )
		pathCat( path, suffix );
}
void					buildPath( char *path, const char *dir, const char *nm, const char *ext ){
	short dirlen = strlen( dir );
	short nmlen = strlen( nm );
	short extlen = strlen( ext );
	path[0] = 0;
	if ( dirlen<2 || dir[2]!=':' ) strcat( path, "M0:/" );
	pathCat( path, dir );
	appendIf( path, "/" );
	pathCat( path, nm );
	appendIf( path, ext );
	if ( strlen( path ) >= MAX_PATH ) 
		tbErr( "path too long" ); 
}
static char *	allocStr( const char * s ){
	char * pStr = tbAlloc( strlen( s )+1, "parser paths" );
	strcpy( pStr, s );	
	return pStr;
}
static char *	loadPath( TknID tknid ){	// build file path prefix from tkn in path[] => &path
	char path[ MAX_PATH ];
	buildPath( path, tknStr(tknid), "", "" );
	return allocStr( path );
}
void 					readContent( void ){		// parse list_of_subjects.txt & messages.txt for each Subj => Content	
	FILE *inFile = fopen( TBP[ pLIST_OF_SUBJS ], "rb" );
	if ( inFile==NULL )
		tbErr( "list_of_subjects file not found" );
	
	char 		line[200], dt[30];			// up to 200 characters per line
	fsTime tm;
	char *	contentVersion = loadLine( line, TBP[ pPKG_VERS ], &tm );
	sprintf( dt, "%d-%d-%d %d:%d", tm.year, tm.mon, tm.day, tm.hr, tm.min );
	logEvtNSNS( "Package", "dt", dt, "ver", contentVersion );
	
	TknID 	lineTkns[20];
	short 	nLnTkns = 0;		// num tokens in current line
	char	msg_txt_fname[MAX_PATH+15];
	while (true){
		char *s = fgets( line, 200, inFile );
		if (s==NULL) break;
		nLnTkns = tokenize( lineTkns, 20, line );		// get tokens from line
		if ( nLnTkns>0 && asPunct( lineTkns[0] )!=Semi ){	// if not comment, tkn is file path for subj/messages.txt
			tbSubject * sb = tbAlloc( sizeof( tbSubject ), "tbSubjects" ); 
			TBookSubj[ nSubjs ] = sb;
			nSubjs++;

			buildPath( msg_txt_fname, TBP[ pPACKAGE_DIR ], tknStr( lineTkns[0] ), "");
			sb->path = allocStr( msg_txt_fname );
			
			buildPath( msg_txt_fname, sb->path, "messages.txt", ".txt" );
			TknID msgs = parseFile( msg_txt_fname );	// parse JSONish messages.txt
			sb->name = tknStr( getField( msgs, "Subject" ));
			sb->audioName = tknStr( getField( msgs, "audioName" ));
			sb->audioPrompt = tknStr( getField( msgs, "audioPrompt" ));
			TknID msgLst = getField( msgs, "Messages" );
			sb->NMsgs = lstCnt( msgLst );
			if ( sb->NMsgs == 0 || sb->NMsgs > MAX_SUBJ_MSGS ) 
				tbErr( "Invalid msg list" );
			for ( int i=0; i<sb->NMsgs; i++ )
				sb->msgAudio[ i ] = tknStr( getField( msgLst, (char *)i ));			// file paths for each message
		}
	}
	fclose( inFile );	// done with list_of_subjects.txt
}
static short	asStIdx( TknID stNm ){			// => idx of csmState with this 'stNm', or alloc
	for( short i=0; i<nCSMstates; i++ ){
		if ( TBookCSM[i]->nmTknID.tknID == stNm.tknID ) 
			return i;
	}
	short stIdx = nCSMstates;
	if ( nCSMstates > MAX_CSM_STATES ) 
		tbErr( "too many CSM states" );
	csmState * cst = tbAlloc( sizeof( csmState ), "csmStates"); 
	cst->nmTknID = stNm;
	cst->nm = tknStr( stNm );
	cst->nActions = 0;
	for ( int e = (int)eNull; e < (int)eUNDEF; e++ ){ 
		cst->evtNxtState[ (Event)e ] = nCSMstates;		// default: all events stay in this state
	}
	nCSMstates++;
	TBookCSM[ stIdx ] = cst;
	return stIdx;
}
static void 	setAnyKey( void *vcst, TknID nxtstate ){ // set cst->evtNxtState[ all keys ] = nxtstate
	csmState * cst = (csmState *) vcst;
	for ( Event e = Home; e<= starTable;  e = (Event)( (short) e+1) )
		cst->evtNxtState[ e ] = asStIdx( nxtstate );
}
static void 	setCGrp( void *vcst, TknID cgrpLst ){	// set cst->evtNxtState[ x ] for x:nxt in cgrpLst
	csmState * cst = (csmState *) vcst;
	for ( short iE = 0; iE < lstCnt(cgrpLst); iE++ )
		cst->evtNxtState[ asEvent( getLstNm(cgrpLst, iE)) ] = asStIdx( getLstVal(cgrpLst, iE));
}
void 					readControlDef( void ){				// parse control.def => Config & TBookCSM[]
	TknID cdef = parseFile( TBP[ pCSM_DEF ] );
	TknID cfg = getField( cdef, "config" );
	
	TB_Config.systemAudio = loadPath( getField( cfg, "systemAudio" ));
	TB_Config.default_volume = getIntFld( cfg, "default_volume", 5 );
	TB_Config.default_speed = getIntFld( cfg, "default_speed", 5 );
	TB_Config.powerCheckMS = getIntFld( cfg, "powerCheckMS", 60000 );	// once/min
	TB_Config.shortIdleMS = getIntFld( cfg, "shortIdleMS", 2000 );		// 2 sec
	TB_Config.longIdleMS = getIntFld( cfg, "longIdleMS", 30000 );		// 30 sec

	TB_Config.minShortPressMS = getIntFld( cfg, "minShortPressMS", 50 );	// extern from inputmanager.c
	TB_Config.minLongPressMS = getIntFld( cfg, "minLongPressMS", 600 );	// extern from inputmanager.c

	TB_Config.initState = asStIdx( getField( cfg, "initState" ));
	
	TknID cgrps = getField( cdef, "CGroups" );
	short nCGrps = lstCnt( cgrps );
	char Nm[MAX_PATH] = { 0 };	//DEBUG
	char Val[MAX_DBG_LEN] = {0}; char* Val2, *Val3;	//DEBUG
	
	TknID cstates =  getField( cdef, "CStates" );
	for ( short iSt=0; iSt < lstCnt( cstates ); iSt++ ){

		TknID csNm = getLstNm( cstates, iSt );
		TknID csDef = getLstVal( cstates, iSt );  	// defn of cstate[iSt]
		toStr( Nm, csNm );  toVStr( Val,0, &Val2, &Val3, csDef ); //DEBUG
		if ( !isLst( csDef )) 
			tbErr( "controlState defn not an object" );
		short stIdx = asStIdx( csNm );
		csmState * cst = TBookCSM[ stIdx ];
		// now fill in cst-> from defn
		TknID actV, argV;
		for ( short stF=0; stF < lstCnt( csDef ); stF++ ){
			TknID nm = getLstNm( csDef, stF );		toStr( Nm, nm );	//DEBUG
			TknID val = getLstVal( csDef, stF );	toVStr( Val,0, &Val2, &Val3, val ); //DEBUG
			if ( nm.tknID==toTkn( "Actions" ).tknID ){
				for ( short iA=0; iA < MAX_ACTIONS; iA++ ) 
					cst->Actions[iA].act = aNull;
				TknID actLst = val;
				cst->nActions = lstCnt( actLst );
				for ( short iA=0; iA < cst->nActions; iA++ ){		// list of tkns, or [ tkn arg ]
					TknID actdefn = getLstVal( actLst, iA );	toStr( Val, actdefn );	//DEBUG
					cst->Actions[ iA ].arg = NULL; // by default
					if ( isLst( actdefn )){ // [ tkn, arg ]
						actV = getLstVal( actdefn, 0 );		toStr( Val, actV );	//DEBUG
						argV = getLstVal( actdefn, 1 );		toStr( Nm, argV );	//DEBUG
						cst->Actions[ iA ].act = asAction( actV );	// Action
						cst->Actions[ iA ].arg = tknStr( argV );	// arg
					} else {
						actV = actdefn;		toStr( Val, actV );	//DEBUG
						cst->Actions[ iA ].act = asAction( actV );	// Action
					}
				}
			} else if ( nm.tknID==toTkn( "CGroups" ).tknID ){
				TknID grpLst = val;			// [ CG1, CG2... ]
				for ( short iG=0; iG < lstCnt( grpLst ); iG++ ){
					TknID cGrp = getLstVal( grpLst, iG );	// nm of CGrp
					TknID gDef = getField( cgrps, tknStr( cGrp ));	// lookup cgrp definition
					setCGrp( cst, gDef );
				}
			} else {	// Evt:St pair
				Event e = asEvent( nm );
				if ( e == anyKey ) 
					setAnyKey( cst, val );
				else
					cst->evtNxtState[ e ] = asStIdx( val );		// nxtState for incoming event 'e'
			}
		}
		//TODO: verify cst->evtNxtState[] are all valid 
	}
	dbgLog( "CSM.def has %d CGroups & %d states \n", nCGrps, nCSMstates );
}
// controlParser.cpp 
