/** @file
This is an example of how a driver might export data to the HII protocol to be
later utilized by the Setup Protocol

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "IpmiConfig.h"
#include "IpmiLib.h"

#define DISPLAY_ONLY_MY_ITEM  0x0002

CHAR16 IpmiVarName[] = EFI_BMC_IPMI_VARIABLE_NAME;

EFI_HANDLE                      DriverHandle[2] = {NULL, NULL};
IPMI_CONFIG_PRIVATE_DATA      *mPrivateData = NULL;
EFI_EVENT                       mEvent;

HII_VENDOR_DEVICE_PATH  mHiiVendorDevicePath0 = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    IPMI_CONFIG_FORMSET_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Request                A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param  Results                A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
IpmiConfigExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  IPMI_CONFIG_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       ConfigRequest;
  EFI_STRING                       ConfigRequestHdr;
  UINTN                            Size;
  CHAR16                           *StrPointer;
  BOOLEAN                          AllocatedRequest;

  // DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize the local variables.
  //
  ConfigRequestHdr  = NULL;
  ConfigRequest     = NULL;
  Size              = 0;
  *Progress         = Request;
  AllocatedRequest  = FALSE;

  PrivateData = IPMI_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;

  //
  // Get Buffer Storage data from EFI variable.
  // Try to get the current setting from variable.
  //
  BufferSize = sizeof (BmcIpmiConfigData);
  Status = gRT->GetVariable (
            IpmiVarName,
            &gIpmiConfigFormSetGuid,
            NULL,
            &BufferSize,
            &PrivateData->IpmiConfig
            );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_INFO,"%a-%d\n",__func__,__LINE__));
    return EFI_NOT_FOUND;
  }

  if (Request == NULL) {
    //
    // Request is set to NULL, construct full request string.
    //

    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&gIpmiConfigFormSetGuid, IpmiVarName, PrivateData->DriverHandle[0]);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
    ConfigRequestHdr = NULL;
  } else {
    //
    // Check routing data in <ConfigHdr>.
    // Note: if only one Storage is used, then this checking could be skipped.
    //
    if (!HiiIsConfigHdrMatch (Request, &gIpmiConfigFormSetGuid, NULL)) {
      return EFI_NOT_FOUND;
    }
    //
    // Set Request to the unified request string.
    //
    ConfigRequest = Request;
    //
    // Check whether Request includes Request Element.
    //
    if (StrStr (Request, L"OFFSET") == NULL) {
      //
      // Check Request Element does exist in Reques String
      //
      StrPointer = StrStr (Request, L"PATH");
      if (StrPointer == NULL) {
        return EFI_INVALID_PARAMETER;
      }
      if (StrStr (StrPointer, L"&") == NULL) {
        Size = (StrLen (Request) + 32 + 1) * sizeof (CHAR16);
        ConfigRequest    = AllocateZeroPool (Size);
        ASSERT (ConfigRequest != NULL);
        AllocatedRequest = TRUE;
        UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", Request, (UINT64)BufferSize);
      }
    }
  }

    //
    // Convert buffer data to <ConfigResp> by helper function BlockToConfig()
    //
    Status = HiiConfigRouting->BlockToConfig (
                                  HiiConfigRouting,
                                  ConfigRequest,
                                  (UINT8 *) &PrivateData->IpmiConfig,
                                  BufferSize,
                                  Results,
                                  Progress
                                  );
    // if (!EFI_ERROR (Status)) {
    //   ConfigRequestHdr = HiiConstructConfigHdr (&gIpmiConfigFormSetGuid, IpmiVarName, PrivateData->DriverHandle[0]);
    //   AppendAltCfgString(Results, ConfigRequestHdr);
    // }

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
  }

  if (ConfigRequestHdr != NULL) {
    FreePool (ConfigRequestHdr);
  }
  //
  // Set Progress string to the original request string.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  // DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  return Status;
}


/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Configuration          A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
IpmiConfigRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  EFI_STATUS                       Status;
  UINTN                            BufferSize;
  IPMI_CONFIG_PRIVATE_DATA       *PrivateData;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  // DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  PrivateData = IPMI_CONFIG_PRIVATE_FROM_THIS (This);
  HiiConfigRouting = PrivateData->HiiConfigRouting;
  *Progress = Configuration;

  //
  // Check routing data in <ConfigHdr>.
  // Note: if only one Storage is used, then this checking could be skipped.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &gIpmiConfigFormSetGuid, NULL)) {
    return EFI_NOT_FOUND;
  }

  //
  // Get Buffer Storage data from EFI variable
  //
  BufferSize = sizeof (BmcIpmiConfigData);
  Status = gRT->GetVariable (
            IpmiVarName,
            &gIpmiConfigFormSetGuid,
            NULL,
            &BufferSize,
            &PrivateData->IpmiConfig
            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock()
  //
  BufferSize = sizeof (BmcIpmiConfigData);
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               Configuration,
                               (UINT8 *) &PrivateData->IpmiConfig,
                               &BufferSize,
                               Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Store Buffer Storage back to EFI variable
  //
  Status = gRT->SetVariable(
                  IpmiVarName,
                  &gIpmiConfigFormSetGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  sizeof (BmcIpmiConfigData),
                  &PrivateData->IpmiConfig
                  );

  // DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  return Status;
}


/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
IpmiConfigCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  IPMI_CONFIG_PRIVATE_DATA        *PrivateData;
  EFI_STATUS                      Status;
  EFI_FORM_ID                     FormId;
  UINT32                          ProgressErr;
  UINT64                          BufferValue;
  EFI_HII_POPUP_SELECTION         UserSelection;

  DEBUG((DEBUG_INFO,"%a-%d-Action=0x%x,ques=0x%x,type=0x%x\n",__func__,__LINE__,Action,QuestionId,Type));
  UserSelection = 0xFF;

  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE))||
    (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FormId = 0;
  ProgressErr = 0;
  Status = EFI_SUCCESS;
  BufferValue = 3;
  PrivateData = IPMI_CONFIG_PRIVATE_FROM_THIS (This);

  CALLBACK_PARAMETERS* CbPara = GetCallbackParameters();
  CbPara->Action = Action;
  CbPara->KeyValue = QuestionId;
  CbPara->Type=Type;

  switch (Action) {
  case EFI_BROWSER_ACTION_FORM_OPEN:
    {
      if (QuestionId == BMC_USER_SETTINGS_KEY) {
        CbPara->Action = EFI_BROWSER_ACTION_CHANGING;
        BmcUserSettingsCallbackFunction(
                            mPrivateData->HiiHandle[0],
                            0,
                            0,
                            BMC_USER_SETTINGS_KEY );
      }

      if( BMC_POWER_RESTORE_POLICY_KEY) {
          BmcGetPowerRestoreCallbackFunction(
                        mPrivateData->HiiHandle[0],
                        0,
                        0,
                        QuestionId);
      }
    }
    break;
  case EFI_BROWSER_ACTION_FORM_CLOSE:
         break;
  case EFI_BROWSER_ACTION_RETRIEVE:
    {
    }
    break;
  case EFI_BROWSER_ACTION_DEFAULT_STANDARD:
    {
    }
    break;
  case EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING:
  case EFI_BROWSER_ACTION_CHANGING:
     // if (QuestionId == ADD_BMC_USER_KEY) {
     //    CbPara->Action = EFI_BROWSER_ACTION_CHANGING;
     //    BmcUserSettingsAddUserCallbackFunction(
     //                    mPrivateData->HiiHandle[0],
     //                    0,
     //                    0,
     //                    ADD_BMC_USER_KEY);

     //  }

     //  if (QuestionId == DELETE_BMC_USER_KEY) {
     //    CbPara->Action = EFI_BROWSER_ACTION_CHANGING;
     //    BmcDeleteUserCallbackFunction(
     //                    mPrivateData->HiiHandle[0],
     //                    0,
     //                    0,
     //                    DELETE_BMC_USER_KEY);

     //  }

     //  if (QuestionId == CHANGE_BMC_USER_SETTINGS_KEY) {
     //    CbPara->Action = EFI_BROWSER_ACTION_CHANGING;
     //    ChangeUserSettingsCallbackFunction(
     //                    mPrivateData->HiiHandle[0],
     //                    0,
     //                    0,
     //                    CHANGE_BMC_USER_SETTINGS_KEY);
     //  }
     //  if (QuestionId == BMC_NETWORK_ETH0_KEY) {
     //      BmcSetEthCallbackFunction(
     //                        mPrivateData->HiiHandle[0],
     //                        0,
     //                        0,
     //                        BMC_NETWORK_ETH0_KEY);

     //      BmcEthParamCallBakcFunction(
     //                        mPrivateData->HiiHandle[0],
     //                        0,
     //                        0,
     //                        BMC_NETWORK_ETH0_KEY);
     //  }

     //  if (QuestionId == BMC_NETWORK_ETH1_KEY) {
     //      BmcSetEthCallbackFunction(
     //                        mPrivateData->HiiHandle[0],
     //                        0,
     //                        0,
     //                        BMC_NETWORK_ETH1_KEY);

     //      BmcEthParamCallBakcFunction(
     //                        mPrivateData->HiiHandle[0],
     //                        0,
     //                        0,
     //                        BMC_NETWORK_ETH1_KEY);
     //  }

      break;
  case EFI_BROWSER_ACTION_CHANGED:
    // switch (QuestionId) {
    //   case ADD_BMC_USER_NAME_KEY:
    //     BmcAddUserNameCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     ADD_BMC_USER_NAME_KEY);
    //     break;
    //   case ADD_BMC_USER_PASSWORD_KEY:
    //     BmcAddUserPasswordCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     ADD_BMC_USER_PASSWORD_KEY);
    //     break;
    //   case ADD_BMC_USER_CHANNEL_NO_KEY:
    //     BmcAddUserChannelCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     ADD_BMC_USER_CHANNEL_NO_KEY);
    //     break;
    //   case ADD_BMC_USER_PRIVILEGE_LIMIT_KEY:
    //     BmcAddUserChannelPrivilegeLimitCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     ADD_BMC_USER_PRIVILEGE_LIMIT_KEY);
    //     break;
    //   case DELETE_BMC_USER_NAME_KEY:
    //     BmcDeleteUserNameCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     DELETE_BMC_USER_NAME_KEY);

    //     break;
    //   case DELETE_BMC_USER_PASSWORD_KEY:
    //     BmcDeleteUserPasswordCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     DELETE_BMC_USER_PASSWORD_KEY);
    //     break;
    //   case CHANGE_USER_SETTINGS_BMC_CURRENT_USER_NAME_KEY:
    //     ChangeUserSettingsCurrentBmcUserNameCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_CURRENT_USER_NAME_KEY);
    //     break;
    //   case CHANGE_USER_SETTINGS_BMC_USER_CURRENT_PASSWORD_KEY:
    //     ChangeUserSettingsCurrentBmcUserPasswordCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_USER_CURRENT_PASSWORD_KEY);
    //     break;
    //   case CHANGE_USER_SETTINGS_BMC_USER_PASSWORD_KEY:
    //     ChangeUserSettingsBmcUserPasswordCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_USER_PASSWORD_KEY);
    //     break;
    //   case CHANGE_USER_SETTINGS_BMC_USER_KEY:
    //     ChangeUserSettingsEnableOrDisableBmcUserCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_USER_KEY);
    //     break;

    //   case CHANGE_USER_SETTINGS_BMC_USER_CHANNEL_NO_KEY:
    //     ChangeUserSettingsBmcUserChannelCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_USER_CHANNEL_NO_KEY);
    //     break;

    //   case CHANGE_USER_SETTINGS_BMC_USER_PRIVILEGE_LIMIT_KEY:
    //     ChangeUserSettingsBmcUserChannelPrivilegeLimitCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     CHANGE_USER_SETTINGS_BMC_USER_PRIVILEGE_LIMIT_KEY);
    //     break;
    //   case BMC_NETWORK_ADDR_MODE:
    //   case BMC_NETWORK_STATION_IP_KEY:
    //   case BMC_NETWORK_SUBNET_KEY:
    //   case BMC_NETWORK_ROUTER_IP_KEY:
    //   case BMC_NETWORK_ROUTER_MAC_KEY:
    //     BmcLanConfigCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     QuestionId);
    //     break;
    //   case BMC_NETWORK_ADDR_MODE2:
    //   case BMC_NETWORK_STATION_IP2_KEY:
    //   case BMC_NETWORK_SUBNET2_KEY:
    //   case BMC_NETWORK_ROUTER_IP2_KEY:
    //   case BMC_NETWORK_ROUTER_MAC2_KEY:
    //     QuestionId -=0x5;
    //     BmcLanConfigCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     QuestionId);
    //     break;
    //   case BMC_POWER_RESTORE_POLICY_KEY:
    //     BmcSetPowerRestoreCallbackFunction(
    //                     mPrivateData->HiiHandle[0],
    //                     0,
    //                     0,
    //                     QuestionId);
    //     break;

      // default:
      //   ;
    // }
    break;
  case EFI_BROWSER_ACTION_SUBMITTED:
  default:
    Status = EFI_UNSUPPORTED;
    break;
  }

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  return Status;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
IpmiConfigInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  EFI_STATUS                      Status;
  EFI_HII_HANDLE                  HiiHandle[2];
  EFI_SCREEN_DESCRIPTOR           Screen;
  EFI_HII_DATABASE_PROTOCOL       *HiiDatabase;
  EFI_HII_STRING_PROTOCOL         *HiiString;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  EFI_CONFIG_KEYWORD_HANDLER_PROTOCOL *HiiKeywordHandler;
  EFI_HII_POPUP_PROTOCOL              *PopupHandler;
  CHAR16                          *NewString;
  UINTN                           BufferSize;
  BOOLEAN                         ActionFlag;
  EFI_STRING                      ConfigRequestHdr;
  EFI_STRING                      NameRequestHdr;
  EFI_INPUT_KEY                   HotKey;
  EDKII_FORM_BROWSER_EXTENSION_PROTOCOL *FormBrowserEx;
  BmcIpmiConfigData               *IpmiConfig;

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  //
  // Initialize the local variables.
  //
  ConfigRequestHdr = NULL;
  NewString        = NULL;

  //
  // Initialize screen dimensions for SendForm().
  // Remove 3 characters from top and bottom
  //
  ZeroMem (&Screen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Screen.RightColumn, &Screen.BottomRow);

  Screen.TopRow     = 3;
  Screen.BottomRow  = Screen.BottomRow - 3;

  //
  // Initialize driver private data
  //
  mPrivateData = AllocateZeroPool (sizeof (IPMI_CONFIG_PRIVATE_DATA));
  if (mPrivateData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->Signature = IPMI_CONFIG_PRIVATE_SIGNATURE;

  mPrivateData->ConfigAccess.ExtractConfig = IpmiConfigExtractConfig;
  mPrivateData->ConfigAccess.RouteConfig = IpmiConfigRouteConfig;
  mPrivateData->ConfigAccess.Callback = IpmiConfigCallback;

  //
  // Locate Hii Database protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->HiiDatabase = HiiDatabase;

  //
  // Locate HiiString protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &HiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->HiiString = HiiString;

  //
  // Locate Formbrowser2 protocol
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->FormBrowser2 = FormBrowser2;

  //
  // Locate ConfigRouting protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->HiiConfigRouting = HiiConfigRouting;

  //
  // Locate keyword handler protocol
  //
  Status = gBS->LocateProtocol (&gEfiConfigKeywordHandlerProtocolGuid, NULL, (VOID **) &HiiKeywordHandler);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->HiiKeywordHandler = HiiKeywordHandler;

  //
  // Locate HiiPopup protocol
  //
  Status = gBS->LocateProtocol (&gEfiHiiPopupProtocolGuid, NULL, (VOID **) &PopupHandler);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mPrivateData->HiiPopup = PopupHandler;

  //
  // Locate Ipmi Transport protocol
  //
  Status = gBS->LocateProtocol (&gEfiDxeIpmiTransportProtocolGuid,NULL,(VOID **)&gIpmiTransport );
    if ( EFI_ERROR (Status) ) {
        return Status;
    }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &DriverHandle[0],
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath0,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &mPrivateData->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  mPrivateData->DriverHandle[0] = DriverHandle[0];

  //
  // Publish our HII data
  //
  HiiHandle[0] = HiiAddPackages (
                   &gIpmiConfigFormSetGuid,
                   DriverHandle[0],
                   IpmiConfigStrings,
                   IpmiConfigVfrBin,
                   NULL
                   );
  if (HiiHandle[0] == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mPrivateData->HiiHandle[0] = HiiHandle[0];

  //
  // Update the device path string.
  //
  NewString = ConvertDevicePathToText((EFI_DEVICE_PATH_PROTOCOL*)&mHiiVendorDevicePath0, FALSE, FALSE);
  if (HiiSetString (HiiHandle[0], STRING_TOKEN (STR_DEVICE_PATH), NewString, NULL) == 0) {
    IpmiConfigUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }
  DEBUG((DEBUG_INFO,"Device path=%s\n",NewString));
  if (NewString != NULL) {
    FreePool (NewString);
  }

  //
  // Very simple example of how one would update a string that is already
  // in the HII database
  //
  NewString = L"OK";

  if (HiiSetString (HiiHandle[0], STRING_TOKEN (STR_IPMI_BMC_STATUS_VAL), NewString, NULL) == 0) {
    IpmiConfigUnload (ImageHandle);
    return EFI_OUT_OF_RESOURCES;
  }

  HiiSetString (HiiHandle[0], 0, NewString, NULL);

  //
  // User Settings variable first
  //
  IpmiConfig = &mPrivateData->IpmiConfig;
  ZeroMem (IpmiConfig, sizeof (BmcIpmiConfigData));
  ConfigRequestHdr = HiiConstructConfigHdr (&gIpmiConfigFormSetGuid, IpmiVarName, DriverHandle[0]);
  ASSERT (ConfigRequestHdr != NULL);

  NameRequestHdr = HiiConstructConfigHdr (&gIpmiConfigFormSetGuid, NULL, DriverHandle[0]);
  ASSERT (NameRequestHdr != NULL);

  BufferSize = sizeof (BmcIpmiConfigData);
  Status = gRT->GetVariable (IpmiVarName, &gIpmiConfigFormSetGuid, NULL, &BufferSize, IpmiConfig);
  if (EFI_ERROR (Status)) {
    //
    // Store zero data Buffer Storage to EFI variable
    //
    Status = gRT->SetVariable(
                    IpmiVarName,
                    &gIpmiConfigFormSetGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    sizeof (BmcIpmiConfigData),
                    IpmiConfig
                    );
    if (EFI_ERROR (Status)) {
      IpmiConfigUnload (ImageHandle);
      return Status;
    }
    //
    // EFI variable for NV config doesn't exit, we should build this variable
    // based on default values stored in IFR
    //
    ActionFlag = HiiSetToDefaults (NameRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      IpmiConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }

    ActionFlag = HiiSetToDefaults (ConfigRequestHdr, EFI_HII_DEFAULT_CLASS_STANDARD);
    if (!ActionFlag) {
      IpmiConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }
  } else {
    //
    // EFI variable does exist and Validate Current Setting
    //
    ActionFlag = HiiValidateSettings (NameRequestHdr);
    if (!ActionFlag) {
      IpmiConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }

    ActionFlag = HiiValidateSettings (ConfigRequestHdr);
    if (!ActionFlag) {
      IpmiConfigUnload (ImageHandle);
      return EFI_INVALID_PARAMETER;
    }
  }
  FreePool (ConfigRequestHdr);

    Status = gBS->CreateEventEx (
        EVT_NOTIFY_SIGNAL, 
        TPL_NOTIFY,
        EfiEventEmptyFunction,
        NULL,
        &gEfiIfrRefreshIdOpGuid,
        &mEvent
        );
    ASSERT_EFI_ERROR (Status);

  //
  // Example of how to use BrowserEx protocol to register HotKey.
  // 
  Status = gBS->LocateProtocol (&gEdkiiFormBrowserExProtocolGuid, NULL, (VOID **) &FormBrowserEx);
  if (!EFI_ERROR (Status)) {
    //
    // First unregister the default hot key F9 and F10.
    //
    HotKey.UnicodeChar = CHAR_NULL;
    HotKey.ScanCode    = SCAN_F9;
    FormBrowserEx->RegisterHotKey (&HotKey, 0, 0, NULL);
    HotKey.ScanCode    = SCAN_F10;
    FormBrowserEx->RegisterHotKey (&HotKey, 0, 0, NULL);

    //
    // Register the default HotKey F9 and F10 again.
    //
    HotKey.ScanCode   = SCAN_F10;
    NewString         = HiiGetString (mPrivateData->HiiHandle[0], STRING_TOKEN (FUNCTION_TEN_STRING), NULL);
    ASSERT (NewString != NULL);
    FormBrowserEx->RegisterHotKey (&HotKey, BROWSER_ACTION_SUBMIT, 0, NewString);
    HotKey.ScanCode   = SCAN_F9;
    NewString         = HiiGetString (mPrivateData->HiiHandle[0], STRING_TOKEN (FUNCTION_NINE_STRING), NULL);
    ASSERT (NewString != NULL);
    FormBrowserEx->RegisterHotKey (&HotKey, BROWSER_ACTION_DEFAULT, EFI_HII_DEFAULT_CLASS_STANDARD, NewString);
  }

  //
  // Example of how to display only the item we sent to HII
  // When this driver is not built into Flash device image,
  // it need to call SendForm to show front page by itself.
  //
  if (DISPLAY_ONLY_MY_ITEM <= 1) {
    //
    // Have the browser pull out our copy of the data, and only display our data
    //
    Status = FormBrowser2->SendForm (
                             FormBrowser2,
                             &(HiiHandle[DISPLAY_ONLY_MY_ITEM]),
                             1,
                             NULL,
                             0,
                             NULL,
                             NULL
                             );

    HiiRemovePackages (HiiHandle[0]);

    HiiRemovePackages (HiiHandle[1]);
  }

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
IpmiConfigUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  UINTN Index;

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  ASSERT (mPrivateData != NULL);

  if (DriverHandle[0] != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
            DriverHandle[0],
            &gEfiDevicePathProtocolGuid,
            &mHiiVendorDevicePath0,
            &gEfiHiiConfigAccessProtocolGuid,
            &mPrivateData->ConfigAccess,
            NULL
           );
    DriverHandle[0] = NULL;
  }

  if (mPrivateData->HiiHandle[0] != NULL) {
    HiiRemovePackages (mPrivateData->HiiHandle[0]);
  }

  for (Index = 0; Index < NAME_VALUE_NAME_NUMBER; Index++) {
    if (mPrivateData->NameValueName[Index] != NULL) {
      FreePool (mPrivateData->NameValueName[Index]);
    }
  }
  FreePool (mPrivateData);
  mPrivateData = NULL;

  gBS->CloseEvent (mEvent);

  return EFI_SUCCESS;
}
