/** @file

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  NVDataStruc.h

Abstract:

  NVData structure used by the sample driver

Revision History:


**/

#ifndef _NVDATASTRUC_H_
#define _NVDATASTRUC_H_

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/HiiFormMapMethodGuid.h>
#include <Guid/ZeroGuid.h>

#define CONFIGURATION_VARSTORE_ID        0x1234
#define BITS_VARSTORE_ID                 0x2345
#define BMC_USER_SETTINGS_VARSTORE_ID    0x1235

#define IPMI_CONFIG_FORMSET_GUID \
  { \
    0xe3540c0c, 0xa09a, 0x1e0d, {0x09, 0x37, 0xc6, 0x8c, 0x95, 0x2f, 0x97, 0xdb} \
  }

extern EFI_GUID gIpmiConfigFormSetGuid;
extern EFI_GUID gEfiIfrRefreshIdOpGuid;

extern EFI_GUID gEfiIfrAdvancedManagerGuid;

//
// Labels definition
//
#define LABEL_UPDATE1               0x1234
#define LABEL_UPDATE2               0x2234
#define LABEL_UPDATE3               0x3234
#define LABEL_END                   0x2223

#define AUTO_ID(x) x
//
//IPMI formid
//
#define IPMI_CONFIG_FIRST_PAGE_ID        0x1001
#define IPMI_USER_SETTINGS_ID            0x1002
#define IPMI_USER_ADD_ID                 0x1003
#define IPMI_USER_DELETE_ID              0x1004
#define IPMI_USER_CHANGE_ID              0x1005
#define IPMI_USER_CHANGE_PASSWORD_ID     0x1006
#define IPMI_USER_CHANGE_ENA_ID          0x1007
#define IPMI_USER_CHANGE_CHANNEL_ID      0x1008
#define IPMI_USER_CHANGE_LIMIT_ID        0x1009

#define BMC_NETWORK_FORM_ID              0x1010
#define BMC_NETWORK_ETH0_ID              0x1011
#define BMC_NETWORK_ETH1_ID              0x1012

//----------------------------------------------------------------------
//
// Macro definitions
//
#define MAX_BMC_USER_COUNT        64
#define MAX_BMC_USER_NAME_LENGTH  16
#define MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER  17

#define LAN_CHANNEL_TYPE 4
#define MAX_CHANNEL_NUMBER 0x0F

#define MIN_BMC_USER_PASSWORD_LENGTH  6
#define MAX_BMC_USER_PASSWORD_LENGTH  16
#define MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER 17


//key value
#define BMC_USER_SETTINGS_KEY                               0x1100
#define ADD_BMC_USER_KEY                                    0x1101
#define ADD_BMC_USER_NAME_KEY                               0x1102
#define ADD_BMC_USER_PASSWORD_KEY                           0x1103
#define ADD_BMC_USER_CHANNEL_NO_KEY                         0x1104
#define ADD_BMC_USER_PRIVILEGE_LIMIT_KEY                    0x1105
#define DELETE_BMC_USER_KEY                                 0x1106
#define DELETE_BMC_USER_NAME_KEY                            0x1107
#define DELETE_BMC_USER_PASSWORD_KEY                        0x1108
#define CHANGE_BMC_USER_SETTINGS_KEY                        0x1109
#define CHANGE_USER_SETTINGS_BMC_CURRENT_USER_NAME_KEY      0x110a
#define CHANGE_USER_SETTINGS_BMC_USER_PASSWORD_KEY          0x110b
#define CHANGE_USER_SETTINGS_BMC_USER_KEY                   0x110c
#define CHANGE_USER_SETTINGS_BMC_USER_CHANNEL_NO_KEY        0x110d
#define CHANGE_USER_SETTINGS_BMC_USER_PRIVILEGE_LIMIT_KEY   0x110e
#define CHANGE_USER_SETTINGS_BMC_USER_CURRENT_PASSWORD_KEY  0x110f

#define BMC_NETWORK_ADDR_MODE                               0x1121
#define BMC_NETWORK_STATION_IP_KEY                          0x1122
#define BMC_NETWORK_SUBNET_KEY                              0x1123
#define BMC_NETWORK_ROUTER_IP_KEY                           0x1124
#define BMC_NETWORK_ROUTER_MAC_KEY                          0x1125
#define BMC_NETWORK_ADDR_MODE2                              0x1126
#define BMC_NETWORK_STATION_IP2_KEY                         0x1127
#define BMC_NETWORK_SUBNET2_KEY                             0x1128
#define BMC_NETWORK_ROUTER_IP2_KEY                          0x1129
#define BMC_NETWORK_ROUTER_MAC2_KEY                         0x112a
#define BMC_NETWORK_FORM_KEY                                0x112b
#define BMC_NETWORK_FORM                                    0x112c
#define BMC_NETWORK_ETH0_KEY                                0x112d
#define BMC_NETWORK_ETH1_KEY                                0x112e

#define BMC_POWER_RESTORE_POLICY_KEY                        0x1131
#define BMC_POWER_RESTORE_POLICY_GET_KEY                    0x1132

#define SYSTEM_PASSWORD_ADMIN   0
#define SYSTEM_PASSWORD_USER    1


#pragma pack(1)
typedef struct {
  UINT8     UserId;
  UINT8     UserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
}BmcUserList;

//
// Bmc Setup Variables
//
typedef struct {
  UINT8         BmcEthValue;
  UINT8         BmcLanMode;
  CHAR16        StationIp[15];
  CHAR16        Subnet[15];
  CHAR16        RouterIp[15];
  CHAR16        Mac[17];
  CHAR16        RouterMac[17];

} BmcIpSetupParameter;

typedef struct {
  UINT8           BmcSupport;
  UINT8           Access; 

  // User Settings
  CHAR16          AddBmcUserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
  UINT16          AddBmcUserPassword[MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER];
  UINT8           AddBmcChannelNo;
  UINT8           AddBmcUserPrivilegeLimit;

  CHAR16          DeleteBmcUserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
  CHAR16          DeleteBmcUserPassword[MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER];

  CHAR16          ChangeUserSettingsBmcCurrentUserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];
  UINT16          ChangeUserSettingsBmcCurrentUserPassword[MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER];
  UINT8           ChangeUserSettingsBmcUser;
  UINT8           ChangeUserSettingsBmcChannelNo;
  UINT16          ChangeUserSettingsBmcUserPassword[MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER];
  UINT8           ChangeUserSettingsBmcUserPrivilegeLimit;

  UINT8           ChangeUserSettingsValidBmcUser;
  UINT8           ChangeUserSettingsValidBmcUserCredentials;
  UINT8           AddBmcUserMaximumPossibleUserPrivilegeLevel;
  UINT8           ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel;
  UINT8           BmcUserAllowableMax;
  UINT8           BmcUserAllowableChannelNum;
  BmcUserList     BmcUserList[MAX_BMC_USER_COUNT];

  // Lan Config
  UINT8           BmcEth;
  BmcIpSetupParameter BmcIpPara[BMC_LAN_COUNT];

  //Power Restore Policy
  UINT8           PowerRes;
} BmcIpmiConfigData;

#pragma pack()
#endif
