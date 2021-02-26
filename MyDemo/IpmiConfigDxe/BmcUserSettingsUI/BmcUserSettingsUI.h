//**********************************************************************
//**********************************************************************
//**                                                                  **
//**        (C)Copyright 1985-2014, American Megatrends, Inc.         **
//**                                                                  **
//**                       All Rights Reserved.                       **
//**                                                                  **
//**      5555 Oakbrook Parkway, Suite 200, Norcross, GA 30093        **
//**                                                                  **
//**                       Phone: (770)-246-8600                      **
//**                                                                  **
//**********************************************************************
//**********************************************************************

/** @file BmcUserSettings.h
    Includes for BMC User Settings.

**/

#ifndef _BMC_USER_SETTINGS_H_
#define _BMC_USER_SETTINGS_H_

//----------------------------------------------------------------------

#include <Protocol/IPMITransportProtocol.h>
#include <Include/IpmiNetFnAppDefinitions.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Include/ServerMgmtSetupVariable.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/HiiLib.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include "NVDataStruc.h"
//----------------------------------------------------------------------


#define STRING_BUFFER_LENGTH    4000

#pragma pack(1)
typedef struct _BMC_USER_DETAILS_LIST {
  UINT8     UserId;
  CHAR8     UserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
}BMC_USER_DETAILS_LIST;

typedef struct _BMC_USER_DETAILS {
  UINT8       UserId;
  CHAR8       UserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
  CHAR8       UserPassword[MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER];
}BMC_USER_DETAILS;

#pragma pack()


EFI_STATUS
BmcUserSettingsCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcUserSettingsAddUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcAddUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcAddUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcAddUserChannelCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcAddUserChannelPrivilegeLimitCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcDeleteUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcDeleteUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
BmcDeleteUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsCurrentBmcUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsCurrentBmcUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsBmcUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsEnableOrDisableBmcUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsBmcUserChannelCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

EFI_STATUS
ChangeUserSettingsBmcUserChannelPrivilegeLimitCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );




#endif

