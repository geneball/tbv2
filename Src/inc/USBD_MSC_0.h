/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB:Host:MSC
 * Copyright (c) 2004-2015 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    USBD_MSC_0.h
 * Purpose: Functions for media ownership control between USB and File System
 * Rev.:    V6.3
 *----------------------------------------------------------------------------*/

#ifndef USBD_MSC0_H
#define USBD_MSC0_H

#include "stdint.h"                     // data type definitions

#define USBD_MSC0_MEDIA_OWN_USB  (1U   ) ///< Media owned by USB (bit mask)
#define USBD_MSC0_MEDIA_OWN_CHG  (1U<<1) ///< Media ownership change requested (bit mask)

#define USBD_MSC0_OK               0     ///< Function succeeded
#define USBD_MSC0_ERROR           -1     ///< Unspecified error


/// \brief Set USB as media owner
/// \return     execution status
///               - USBD_MSC0_OK           = Media ownership changed successfully
///               - USBD_MSC0_ERROR        = Media ownership change has failed (due timeout)
extern int32_t USBD_MSC0_SetMediaOwnerUSB (void);


/// \brief Set File System as media owner
/// \return     execution status
///               - USBD_MSC0_OK           = Media ownership changed successfully
///               - USBD_MSC0_ERROR        = Media ownership change has failed (due timeout)
extern int32_t USBD_MSC0_SetMediaOwnerFS (void);

#endif
