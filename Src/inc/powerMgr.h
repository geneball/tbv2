#ifndef POWER_MGR_H
#define POWER_MGR_H

#include 	"tbook.h"

#define		POWER_UP 	1
#define		POWER_DOWN 	0

typedef void(*HPE)( int );		// type for ptr to powerEventHandler() 


extern void		initPowerMgr( void );							// initialize PowerMgr & start thread
extern void 	enableSleep( void );							// low power mode -- CPU stops till interrupt
extern void		powerDownTBook( void );						// shut down TBook
extern void		wakeup( void );
extern void		setPowerCheckTimer( int timerMs );	// set delay in msec between power checks

#endif	/* POWER_MGR_H */

