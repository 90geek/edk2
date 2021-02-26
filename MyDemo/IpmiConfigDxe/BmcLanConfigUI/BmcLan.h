#ifndef __BMC_LAN_H__
#define __BMC_LAN_H__

#define BMC_LAN1_CHANNEL_NUMBER	0x1
#define BMC_LAN2_CHANNEL_NUMBER	0x3

//
// Global Variables
//
#pragma pack(1)
typedef struct {
  UINT8         BmcLan;
  UINT8         BmcLanMode;
  UINT8         StationIp[4];
  UINT8         Subnet[4];
  UINT8         RouterIp[4];
  UINT8         Mac[6];
  UINT8         RouterMac[6];
} BMC_IP_BIOS_SETTINGS;

EFI_STATUS
BmcLanConfigCallbackFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key );

VOID
BmcLanParamDisplayInSetup(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key );

EFI_STATUS
BmcSetEthCallbackFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key );

EFI_STATUS
BmcEthParamCallBakcFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key );


#endif
