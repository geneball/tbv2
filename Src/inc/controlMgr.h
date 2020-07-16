// TBookV2		controlmanager.h

#ifndef CONTROL_MGR_H
#define CONTROL_MGR_H
#include "tbook.h"

typedef struct {
	Action	act;
	char *	arg;
} csmAction;


#define		MAX_SUBJ_MSGS			 20		// allocates: (array of ptrs) per subject
#define		MAX_SUBJS					 20		// allocates: (array of ptrs)
#define		MAX_ACTIONS					6		// max # actions per state -- allocs (array of csmAction)
#define		MAX_CSM_STATES		120		// max # csm states -- (allocs array of ptrs)
#define   MAX_PKGS						4		// max # of content packages on a device

// ---------TBook Control State Machine
typedef struct {	// csmState
	TknID 							nmTknID;										// TknID for name of this state
	char *							nm;													// str name of this state
	short 							evtNxtState[ eUNDEF ];			// nxtState for each incoming event (as idx in TBookCSM[])
	short								nActions;
	csmAction 					Actions[ MAX_ACTIONS ];
} csmState;

//  ------------  TBook Content
typedef struct { 	// tbSubject										// info for one subject
	char *							path;												// path to Subj directory
	char *							name;												// identifier
	char *  						audioName;									// file path
	char *  						audioPrompt;								// file path
	short								NMsgs;											// number of messages
	char *  						msgAudio[ MAX_SUBJ_MSGS ];	// file paths for each message
	MsgStats *					stats;
} tbSubject;
//
typedef struct TBPackage {	// TBPackage_t				// path & subject list for a package
	int									idx;
	char * 							path;
	char *							packageName;
	int 								nSubjs;
	tbSubject *					TBookSubj[ MAX_SUBJS ];
} TBPackage_t;

extern int 						nPackages;									// num packages found on device
extern TBPackage_t *	TBPackage[ MAX_PKGS ];			// package info
extern int						iPkg;
extern TBPackage_t 	* TBPkg;											// TBook content in use

// TBook ControlStateMachine
extern int			nCSMstates;																	// #states defined				
extern csmState *	TBookCSM[ MAX_CSM_STATES ];								// TBook state machine definition

extern void						initControlManager( void );						// initialize & run TBook Control State Machine
extern void						buildPath( char *dstpath, const char *dir, const char *nm, const char *ext ); // dstpath[] <= "dir/nm.ext"
extern void						findPackages( void );									// scan for M0:/package*/  directories
TBPackage_t * 				readContent( const char * pkgPath, int pkgIdx );		// parse list_of_subjects.txt & messages.txt for each Subj => Content
extern void 					readControlDef( void );								// parse control.def => Config & TBookCSM[]

#endif
