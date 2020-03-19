// USBD_User_MSC_LUN    JEB Mar-2020
//   interface between tbook &  Keil USB middleware -- to provide TBook FileSystem as USB MassStorageDevice 
// combines:  
//  TBook interface:   initUSBManager, isMassStorageEnabled(), enableMassStorage(), disableMassStorage()
//  USB_User_MSC callbacks (called from USB_thread) to hook into middleware

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "rl_usb.h"
#include "rl_fs.h"
#include "tbook.h"

// communcation with USBD_MSC_0  USBD_MSC0_SetMediaOwnerUSB
//volatile uint8_t  msc0_media_own;
//volatile uint8_t  usbd_msc0_media_own;  // USB MSCn media ownership

const int maxLUNs = 4;
static struct LogicalDrive {
	char 			drive[6];				// "M0:" or "M1:" or "N0:" or "F0:"
	int32_t 	drv_id;      		// FAT drive id
	bool  		ownedByUSB;			// USB MSCn media ownership
	bool			media_ok;	     	// Media is initialized and ok
	
} LUN[ maxLUNs ];
static int nLUNs = 0;

volatile	bool 								usbProvidingMassStorage = false;	// flag indicating USB has control of FileSys as MSC
volatile	bool 								usbRequestChange			 	= false;
volatile	bool 								usbRequestMSC			 			= false;
static		bool 								usbIsInitialized				= false;		

//static	osEventFlagsId_t 		usbEvent;		
// internal utilities

/*int32_t 	USBD_MSC0_SetMediaOwnerUSB( void ){		// called from non-USB thread to request that USB provide FileSys as MSC0
	// set flag so that next call on USBthread to USBD_MSC0_LUN_CheckMedia will switch FS control
  uint32_t timeout_cnt;
 
  timeout_cnt = 300U;                   // 3 second timeout (300 * 10 ms)
  usbd_msc0_media_own = USBD_MSC0_MEDIA_OWN_CHG | USBD_MSC0_MEDIA_OWN_USB;
  while (usbd_msc0_media_own & USBD_MSC0_MEDIA_OWN_CHG) {
    osDelay(10);
    if ((--timeout_cnt) == 0) { return USBD_MSC0_ERROR; }
  }
   return USBD_MSC0_OK;
}
int32_t USBD_MSC0_SetMediaOwnerFS (void) {			// called from non-USB thread to request that FileSys revert to firmware access
  uint32_t timeout_cnt;
 
  timeout_cnt = 300U;                   // 3 second timeout (300 * 10 ms)
  usbd_msc0_media_own = USBD_MSC0_MEDIA_OWN_CHG;
  while (usbd_msc0_media_own & USBD_MSC0_MEDIA_OWN_CHG) {
    if(!USBD_Configured(USBD_MSC0_DEV)) {
      // USB device not configured, so call CheckMedia to do ownership handling 
      USBD_MSC0_CheckMedia();
    }
    osDelay(10);
    if ((--timeout_cnt) == 0) { return USBD_MSC0_ERROR; }
  }
 
  return USBD_MSC0_OK;
}
*/


void 		initLUN( uint8_t lun ){					// init LUN[ lun ].drive & lock for USB access
  uint32_t 	param;
	if ( lun >= nLUNs ) return;
	LUN[ lun ].media_ok = false;

  int stat = finit( LUN[ lun ].drive );
  if ( stat	!= fsOK ) return;										// failed to init
	
	stat = funmount( LUN[ lun ].drive );					// unmount from TBook

	LUN[lun].ownedByUSB = true;
	int32_t drv_id = fs_ioc_get_id( LUN[ lun ].drive ); 	// Get ID of media drive
	if (drv_id < 0)     return;		 								// invalid => fail

	LUN[lun].drv_id = drv_id;
	
  param = FS_CONTROL_MEDIA_INIT;     // Parameter for fsDevCtrlCodeControlMedia 0: Initialize media
  stat = fs_ioc_device_ctrl( drv_id, fsDevCtrlCodeControlMedia, &param );
	if ( stat != fsOK )  return;          // return if failed
 
  stat = fs_ioc_lock( drv_id );         // Lock media for USB usage
	if ( stat != fsOK )  return;          // return if failed
  
  LUN[ lun ].media_ok = true;           // success -- mark as ok
}
void 		uninitLUN( uint8_t lun ){				// release LUN[ lun ].drive for TBook access
	if ( !LUN[ lun ].media_ok ) return;							// not locked
	LUN[ lun ].media_ok = false;
  int stat = fs_ioc_unlock( LUN[ lun ].drv_id );  // unlock media for USB usage
	
	stat = fmount( LUN[ lun ].drive );			// remount for use by TBook
}
void		addLUN( char *drv ){						// add drive name ( e.g. "M0:") to LUN[]
	if ( drv==NULL ) return;
	int dlen = strlen( drv );
	if ( dlen==0 || dlen > 5 ) return;
	
	strcpy( LUN[ nLUNs ].drive, drv );		// copy drive name into LUN[ nLUNs ]
	nLUNs++;
}
void		initUSBManager( void ){
//	usbProvidingMassStorage = false;
//	usbIsInitialized = false;
//	usbEvent = osEventFlagsNew(NULL);
//	if ( usbEvent == NULL)
//		tbErr( "usbEvent alloc failed" );	
	
	//registerPowerEventHandler( handlePowerEvent );	
}
bool 		usbSignalRequestAndWait( ){		// => true if usbRequestChange handled, false if timeout
		// wait for CheckMedia to handle request
		usbRequestChange = true;
		uint32_t timeout_cnt = 300;           			 // 3 second timeout (300 * 10 ms)
 		while ( usbRequestChange ){ //usbd_msc0_media_own & USBD_MSC0_MEDIA_OWN_CHG) {
			osDelay(10);
			if ((--timeout_cnt) == 0) return false;
		}
		return true;
}

// exported to TBook
bool		isMassStorageEnabled( void ){	// => true if usb is providing FileSys as MSC
	return usbProvidingMassStorage;
}	
bool		enableMassStorage( char *drv0, char *drv1, char *drv2, char *drv3 ){
	if ( usbProvidingMassStorage ) return true;
	
	// initialize LUN[] array for specified drives
	addLUN( drv0==NULL? "M0:" : drv0 ); 
	addLUN( drv1 ); 
	addLUN( drv2 ); 
	addLUN( drv3 ); 
	
//		bool isConfigured = USBD_Configured( 0 );		// is USB device ready?
	usbStatus stat = USBD_Initialize(0);						// initialize -- calls USB_MSC0_Initialize which unmounts LUNs & locks for USB
	usbIsInitialized = (stat == usbOK); 
	if ( !usbIsInitialized ) 			return false;			// USB failed to initialize
	
	stat = USBD_Connect(0);													// signal connection to Host, so Host will enumerate & discover configured drives
	
	// wait until USB driver calls checkMedia
//	if ( !usbSignalRequestAndWait() ) return false;		  // timeout => failed
//		if ( USBD_MSC0_SetMediaOwnerUSB() == USBD_MSC0_OK ){		
//			//TODO? USBD_SetSerialNumber( 0, "TBook SN#" );
//			ledFg( "O2o2!" );	
//			usbConnected = true;			
//		}
	return usbProvidingMassStorage;
}
bool		disableMassStorage( void ){
	if ( usbProvidingMassStorage ){	
		usbProvidingMassStorage = false;
		for ( int i=0; i<nLUNs; i++ )
				uninitLUN( i );				// unlock & remount for TBook

		USBD_Disconnect( 0 );
		USBD_Uninitialize( 0 );
		usbIsInitialized = false;
		usbProvidingMassStorage = false;
	}
	ledFg( "_" );	// ledOn(LED_ORANGE);		
	return usbProvidingMassStorage;
}
bool 		getCacheInfoLUN( uint8_t lun, uint32_t *buffer, uint32_t *size  ){
  if ( !LUN[ lun ].media_ok ) return false;

	fsIOC_Cache 	cache_info;
  int stat = fs_ioc_get_cache( LUN[ lun ].drv_id, &cache_info);  // Get cache settings of File System
  if ( stat	!= fsOK )     return false; 
 
  // Use File Systems cache for MSC
  *buffer = (uint32_t)cache_info.buffer;// Cache buffer from File System
  *size   = cache_info.size;            // Cache size
	return true;
}

// define callbacks ( override weak definitions ) from USBD driver
//
void	 	USBD_MSC0_Initialize( void ){																	// callback from USBD_Initialize to initialize all Logical Units of the USB MSC class instance.
  for ( int i=0; i<nLUNs; i++ )
		initLUN( i );
	usbProvidingMassStorage = true;
}
void 		USBD_MSC0_Uninitialize( void ){																// callback from USBD_Uninitialize to de-initialize all Logical Units of the USB MSC class instance.
  // Add code for de-initialization
  uninitLUN( 0 );
  uninitLUN( 1 );
}
 
 
bool 		USBD_MSC0_GetCacheInfo( uint32_t *buffer, uint32_t *size ) {  	// Get cache info: sets cache buffer address & size  => true if success
	return getCacheInfoLUN( 0, buffer, size );		// use LUN0 for both ?!
}
uint8_t USBD_MSC0_GetMaxLUN( void ) {	// return maximum Index of logical units
  return nLUNs-1;                     // max index of configured file drives 
}
bool 		USBD_MSC0_LUN_GetMediaCapacity( uint8_t lun, uint32_t *block_count, uint32_t *block_size ){	// set #blocks & blocksize for logical unit => true if success
  fsMediaInfo media_info;
  if ( !LUN[ lun ].media_ok ) return false;
  int stat = fs_ioc_read_info( LUN[ lun ].drv_id, &media_info );  // Read media information of actual media
  if ( stat	!= fsOK) return false;      
 
  *block_count = media_info.block_cnt;  	// Total number of blocks on media
  *block_size  = media_info.read_blen;  	// Block size of blocks on media 
  return true;
}
bool 		USBD_MSC0_LUN_Read( uint8_t lun, uint32_t lba, uint32_t cnt, uint8_t *buf ){		// read 'cnt' blocks starting at 'lba' into 'buf'  => true if success
  if ( !LUN[ lun ].media_ok ) return false;
	
  int stat = fs_ioc_read_sector( LUN[ lun ].drv_id, lba, buf, cnt );   // Read data directly from media
  return ( stat == fsOK );
}
bool 		USBD_MSC0_LUN_Write (uint8_t lun, uint32_t lba, uint32_t cnt, const uint8_t *buf) {	// write 'cnt' blocks starting at 'lba' from 'buf'  => true if success
  if ( !LUN[ lun ].media_ok ) return false;
	
  int stat = fs_ioc_write_sector( LUN[ lun ].drv_id, lba, buf, cnt );   // Write data directly to media
  return ( stat == fsOK );
}
// callback from USBD driver -- something might have changed
uint32_t USBD_MSC0_LUN_CheckMedia( uint8_t lun ){		// return status for logical unit:  bit 0: 1 if media present, bit 1: 1 if write-protected
	if ( lun >= nLUNs || !LUN[ lun ].media_ok ) 
		return 0; 		// drive is not ready

	uint32_t fsstatus;
	int stat = 	fs_ioc_device_ctrl( LUN[ lun ].drv_id, fsDevCtrlCodeCheckMedia, &fsstatus );
	
	if ( stat == fsOK && (fsstatus & FS_MEDIA_PROTECTED) != 0 ) 
     return USBD_MSC_MEDIA_PROTECTED | USBD_MSC_MEDIA_READY;
 
	return USBD_MSC_MEDIA_READY;
/*	
       uint32_t fsstatus;
static uint8_t  media_ready_ex = 0U;    // Previous media ready state
       uint8_t  own;
 
  uint8_t  media_state = USBD_MSC_MEDIA_READY;
	
	
	int stat = 	fs_ioc_device_ctrl( LUN[ lun ].drv_id, fsDevCtrlCodeCheckMedia, &fsstatus );
	
	if ( stat == fsOK && (fsstatus & FS_MEDIA_PROTECTED) != 0 ) {
     media_state |=  USBD_MSC_MEDIA_PROTECTED;
  }
 
  // Store current owner so no new request can interfere
  own = usbd_msc0_media_own;
 
  // De-initialize media according to previous owner
  if (own & MEDIA_OWN_CHG) {                    // If owner change requested
    if (own & MEDIA_OWN_USB) {                  // If new requested owner is USB (previous owner was File System)
      funmount (MEDIA_DRIVE);                   // De-initialize media and dismount Drive
    } else {                                    // If new requested owner is File System (previous owner was USB)
      fs_ioc_unlock (drv_id);                   // Un-lock media
    }
  }
 
  // Initialize media according to current owner
  if ((own & MEDIA_OWN_CHG)        ||           // If owner change requested or
      (media_state ^ media_ready_ex)) {         // if media ready state has changed (disconnect(SD remove)/connect(SD insert))
    if (media_state & USBD_MSC_MEDIA_READY) {   // If media is ready
      if (own & MEDIA_OWN_USB){                 // If current owner is USB
        media_ok     = false;                   // Invalidate current media status (not initialized = not ok)
        param_status = 0U;                      // Parameter for function call is 0
        if (fs_ioc_device_ctrl (drv_id, fsDevCtrlCodeControlMedia, &param_status) == fsOK) {
                                                // Initialization of media has succeeded
          if (fs_ioc_lock (drv_id) == 0) {      // If lock media for USB usage has succeeded
            media_ok = true;                    // Media was initialized and is ok
          }
        }
      } else {                                  // If current owner is File System
        if (fmount (MEDIA_DRIVE) == fsOK) {     // Initialize media and Mount Drive for File System usage
          media_ok = true;                      // Media was initialized and is ok
        }
      }
    }
    if (own & MEDIA_OWN_CHG) {
      usbd_msc0_media_own &= ~MEDIA_OWN_CHG;    // Clear request to change media owner if it was handled
    }
    media_ready_ex = media_state & USBD_MSC_MEDIA_READY;
  }
 
  // If media is not ok or owned by File System return that it is not ready for USB
  if ((!media_ok) || (!(usbd_msc0_media_own & MEDIA_OWN_USB))) {
    return 0U;
  }
 
  return media_state;
*/
}
// end USBD_User_MSC_LUN
