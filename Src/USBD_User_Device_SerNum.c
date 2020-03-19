/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Device
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    USBD_User_Device_SerNum_0.c
 * Purpose: USB Device User module
 * Rev.:    V1.1.2
 *----------------------------------------------------------------------------*/
/*
 * USBD_User_Device_SerNum_0.c is a code template for the user specific 
 * Device events and Control Endpoint 0 requests handling. It demonstrates how 
 * to specify serial number at runtime instead of fixed one specified in 
 * USBD_Config_0.c file.
 */
 
/**
 * \addtogroup usbd_coreFunctions_api
 *
 */
 
 
//! [code_USBD_User_Device]
 
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "rl_usb.h"
#include <stdio.h>
#include "stm32f412vx.h"
 
static bool    handle_request;
static char  ser_no_string_desc[32];  // String Descriptor: LEN TYP + 4 + 3*8

const int nEvts = 100;
const int mxS = 20;
char Evts[ nEvts ][mxS+1] = {0};
int nxtEvt = 0;

void logUEvt( char *str ){
	int ln = strlen( str );
	if ( ln > mxS ) str[ mxS ] = 0;
	strcpy( &Evts[ nxtEvt ][0], str );
	nxtEvt++;
  if ( nxtEvt == nEvts ) nxtEvt = 0;
	strcpy( &Evts[ nxtEvt ][0], "=====" );
}

// \brief Callback function called during USBD_Initialize to initialize the USB Device
void USBD_Device0_Initialize (void) {
  // Handle Device Initialization
  logUEvt( "Init" );
  handle_request = false;
}
 
// \brief Callback function called during USBD_Uninitialize to de-initialize the USB Device
void USBD_Device0_Uninitialize (void) {
  logUEvt( "Uninit" );
  // Handle Device De-initialization
}
 
// \brief Callback function called when VBUS level changes
// \param[in]     level                current VBUS level
//                                       - true: VBUS level is high
//                                       - false: VBUS level is low
void USBD_Device0_VbusChanged (bool level) {
  logUEvt( "VBusCh" );
}
 
// \brief Callback function called upon USB Bus Reset signaling
void USBD_Device0_Reset (void) {
  logUEvt( "Reset" );
}
 
// \brief Callback function called when USB Bus speed was changed to high-speed
void USBD_Device0_HighSpeedActivated (void) {
  logUEvt( "HSAct" );
}
 
// \brief Callback function called when USB Bus goes into suspend mode (no bus activity for 3 ms)
void USBD_Device0_Suspended (void) {
  logUEvt( "Susp" );
}
 
// \brief Callback function called when USB Bus activity resumes
void USBD_Device0_Resumed (void) {
  logUEvt( "Resum" );
}
 
// \brief Callback function called when Device was successfully enumerated
// \param[in]     val                  current configuration value
//                                       - value 0: not configured
//                                       - value > 0: active configuration
void USBD_Device0_ConfigurationChanged (uint8_t val) {
	char s[20];
	sprintf( s, "CfgCh %d", val );
  logUEvt( s );
}
 
// \brief Callback function called when Set Feature for Remote Wakeup comes over Control Endpoint 0
void USBD_Device0_EnableRemoteWakeup (void) {
  logUEvt( "EnRmtWk" );
}
 
// \brief Callback function called when Clear Feature for Remote Wakeup comes over Control Endpoint 0
void USBD_Device0_DisableRemoteWakeup (void) {
  logUEvt( "DisRmtWk" );
}
 
// \brief Callback function called when Device 0 received SETUP PACKET on Control Endpoint 0
// \param[in]     setup_packet             pointer to received setup packet.
// \param[out]    buf                      pointer to data buffer used for data stage requested by setup packet.
// \param[out]    len                      pointer to number of data bytes in data stage requested by setup packet.
// \return        usbdRequestStatus        enumerator value indicating the function execution status
// \return        usbdRequestNotProcessed: request was not processed; processing will be done by USB library
// \return        usbdRequestOK:           request was processed successfully (send Zero-Length Packet if no data stage)
// \return        usbdRequestStall:        request was processed but is not supported (stall Endpoint 0)
static uint8_t **lastSUBuf;
static uint32_t *lastSUlen;
usbdRequestStatus USBD_Device0_Endpoint0_SetupPacketReceived (const USB_SETUP_PACKET *setup_packet, uint8_t **buf, uint32_t *len) {
	char s[20];
	sprintf( s, "SU(T%02x R%d %04x %04x)", ((char *)setup_packet)[0], setup_packet->bRequest, setup_packet->wValue, setup_packet->wIndex  );
  logUEvt( s );
  lastSUBuf = buf;
	lastSUlen = len;
  switch (setup_packet->bmRequestType.Type) {
    case USB_REQUEST_STANDARD:
      // Catch Get String Descriptor request for serial number string and return desired string:
      if ((setup_packet->bmRequestType.Dir       == USB_REQUEST_DEVICE_TO_HOST) &&      // Request to get
          (setup_packet->bmRequestType.Recipient == USB_REQUEST_TO_DEVICE     ) &&      // from device
          (setup_packet->bRequest                == USB_REQUEST_GET_DESCRIPTOR) &&      // the descriptor
         ((setup_packet->wValue >> 8)            == USB_STRING_DESCRIPTOR_TYPE) &&      // String Descriptor Type
         ((setup_packet->wValue & 0xFFU)         == 0x03U                     ) &&      // Index of String = 3
          (setup_packet->wIndex                  == 0x0409U                   )) {      // Language ID = 0x0409 = English (United States)
        ser_no_string_desc[0] = 26U;    // Total size of String Descriptor
        ser_no_string_desc[1] = USB_STRING_DESCRIPTOR_TYPE;   // String Descriptor Type
				uint32_t *pUID = (uint32_t *) UID_BASE;
				sprintf( &ser_no_string_desc[2], "TB2.%08x%08x%08x", pUID[0], pUID[1], pUID[2] );
        //memcpy(&ser_no_string_desc[2], L"0002B0000001", 24);  // Serial Number String value
        *buf = (uint8_t *) &ser_no_string_desc[0];      // Return pointer to prepared String Descriptor
				logUEvt( &ser_no_string_desc[2] );
        if (setup_packet->wLength >= 26) {
          *len = 26U;                   // Number of bytes of whole String Descriptor
        } else {
          *len = setup_packet->wLength; // Requested number of bytes of String Descriptor
        }
        handle_request = true;          // This request is handled
        return usbdRequestOK;           // Return status that custom handling for this request is used
      }
      break;
    case USB_REQUEST_CLASS:
      break;
    case USB_REQUEST_VENDOR:
      break;
    case USB_REQUEST_RESERVED:
      break;
  }
  return usbdRequestNotProcessed;
}
 
// \brief Callback function called when SETUP PACKET was processed by USB library
// \param[in]     setup_packet            pointer to processed setup packet.
void USBD_Device0_Endpoint0_SetupPacketProcessed (const USB_SETUP_PACKET *setup_packet) {
	char s[20];
	if ( *lastSUlen==0 ) 
		logUEvt( "SU =>0" );
	else {
		sprintf( s, "SU=>%d[%02x %02x %02x %02x]", *lastSUlen, *lastSUBuf[0], *lastSUBuf[1], *lastSUBuf[2], *lastSUBuf[3] );
		logUEvt( s );
	}
 
  switch (setup_packet->bmRequestType.Type) {
    case USB_REQUEST_STANDARD:
      break;
    case USB_REQUEST_CLASS:
      break;
    case USB_REQUEST_VENDOR:
      break;
    case USB_REQUEST_RESERVED:
      break;
  }
}
 
// \brief Callback function called when Device 0 received OUT DATA on Control Endpoint 0
// \param[in]     len                      number of received data bytes.
// \return        usbdRequestStatus        enumerator value indicating the function execution status
// \return        usbdRequestNotProcessed: request was not processed; processing will be done by USB library
// \return        usbdRequestOK:           request was processed successfully (send Zero-Length Packet)
// \return        usbdRequestStall:        request was processed but is not supported (stall Endpoint 0)
// \return        usbdRequestNAK:          request was processed but the device is busy (return NAK)
usbdRequestStatus USBD_Device0_Endpoint0_OutDataReceived (uint32_t len) {
	char s[20];
	sprintf( s, "OD L%d", len );
  logUEvt( s );
 
  return usbdRequestNotProcessed;
}
 
// \brief Callback function called when Device 0 sent IN DATA on Control Endpoint 0
// \param[in]     len                      number of sent data bytes.
// \return        usbdRequestStatus        enumerator value indicating the function execution status
// \return        usbdRequestNotProcessed: request was not processed; processing will be done by USB library
// \return        usbdRequestOK:           request was processed successfully (return ACK)
// \return        usbdRequestStall:        request was processed but is not supported (stall Endpoint 0)
// \return        usbdRequestNAK:          request was processed but the device is busy (return NAK)
usbdRequestStatus USBD_Device0_Endpoint0_InDataSent (uint32_t len) {
	char s[20];
	sprintf( s, "IN L%d", len );
  logUEvt( s );

  handle_request = false;
  if (handle_request == true) {
    handle_request = false;
    return usbdRequestOK;               // Acknowledge custom handled request
  }
 
  return usbdRequestNotProcessed;
}
 
//! [code_USBD_User_Device]
