#ifndef __BMC_POWER_RESTORE_UI_H__
#define __BMC_POWER_RESTORE_UI_H__
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
#include <Include/IpmiNetFnChassisDefinitions.h>
// #include "NVDataStruc.h"

EFI_STATUS
BmcGetPowerRestoreCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );
EFI_STATUS
BmcSetPowerRestoreCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key );

#endif
