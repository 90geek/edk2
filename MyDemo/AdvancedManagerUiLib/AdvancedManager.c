/** @file
The device manager reference implementation

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "AdvancedManager.h"
#include "AdvancedManagerCustomizedUi.h"

ADVANCED_MANAGER_CALLBACK_DATA  gAdvancedManagerPrivate = {
  ADVANCED_MANAGER_CALLBACK_DATA_SIGNATURE,
  NULL,
  NULL,
  {
    AdvancedManagerExtractConfig,
    AdvancedManagerRouteConfig,
    AdvancedManagerCallback
  }
};


EFI_GUID mAdvancedManagerGuid = ADVANCED_MANAGER_FORMSET_GUID;

HII_VENDOR_DEVICE_PATH  mAdvancedManagerHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    //
    // {2d6a3b9b-629a-5a62-8cd3-7865c6efd37c}
    //
    { 0x2d6a3b9b, 0x629a, 0x5a62, { 0x8c, 0xd3, 0x78, 0x65, 0xc6, 0xef, 0xd3, 0x7c } }
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

ADVANCED_MANAGER_CALLBACK_DATA *mAdvancedCallbackInfo = &gAdvancedManagerPrivate; 
BOOLEAN  mFirstEnterAdvancedForm         = FALSE;

/**

  Update the menus in the Advanced page.

**/
VOID
AdvancedCustomizeMenus (
  VOID
  )
{
  VOID                        *StartOpCodeHandle;
  VOID                        *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL          *StartGuidLabel;
  EFI_IFR_GUID_LABEL          *EndGuidLabel;

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  //
  // Allocate space for creation of UpdateData Buffer
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);
  //
  // Create Hii Extend Label OpCode as the start opcode
  //
  StartGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartGuidLabel->Number       = LABEL_ADVANCED_START;
  //
  // Create Hii Extend Label OpCode as the end opcode
  //
  EndGuidLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndGuidLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndGuidLabel->Number       = LABEL_ADVANCED_END;

  //
  //Updata Front Page form
  //
  UiCustomizeAdvancedPage (
    mAdvancedCallbackInfo->HiiHandle,
    StartOpCodeHandle
    );

  HiiUpdateForm (
    mAdvancedCallbackInfo->HiiHandle,
    &mAdvancedManagerGuid,
    ADVANCED_MANAGER_FORM_ID,
    StartOpCodeHandle,
    EndOpCodeHandle
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
AdvancedManagerExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
AdvancedManagerRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Configuration;

  return EFI_NOT_FOUND;
}

/**
  This function is invoked if user selected a interactive opcode from Advanced Manager's
  Formset. If user set VBIOS, the new value is saved to EFI variable.

  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Action          Specifies the type of action taken by the browser.
  @param QuestionId      A unique value which is sent to the original exporting driver
                         so that it can identify the type of data to expect.
  @param Type            The type of value for the question.
  @param Value           A pointer to the data being sent to the original exporting driver.
  @param ActionRequest   On return, points to the action requested by the callback function.

  @retval  EFI_SUCCESS           The callback successfully handled the action.
  @retval  EFI_INVALID_PARAMETER The setup browser call this function with invalid parameters.

**/
EFI_STATUS
EFIAPI
AdvancedManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{

  DEBUG((DEBUG_INFO,"%a-%d-Action=0x%x,ques=0x%x,type=0x%x\n",__func__,__LINE__,Action,QuestionId,Type));
  if (((Value == NULL) && (Action != EFI_BROWSER_ACTION_FORM_OPEN) && (Action != EFI_BROWSER_ACTION_FORM_CLOSE))||
    (ActionRequest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
 }

/**
  Install Boot Manager Menu driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
AdvancedManagerUiLibConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
)
{
  EFI_STATUS                  Status;

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  gAdvancedManagerPrivate.DriverHandle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gAdvancedManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mAdvancedManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gAdvancedManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish our HII data.
  //
  gAdvancedManagerPrivate.HiiHandle = HiiAddPackages (
                  &mAdvancedManagerGuid,
                  gAdvancedManagerPrivate.DriverHandle,
                  AdvancedManagerVfrBin,
                  AdvancedManagerUiLibStrings,
                  NULL
                  );
  ASSERT (gAdvancedManagerPrivate.HiiHandle != NULL);

  //
  // Update boot manager page
  //
  AdvancedCustomizeMenus ();
  return EFI_SUCCESS;
}

/**
  Unloads the application and its installed protocol.

  @param  ImageHandle     Handle that identifies the image to be unloaded.
  @param  SystemTable     The system table.

  @retval EFI_SUCCESS           The image has been unloaded.
**/
EFI_STATUS
EFIAPI
AdvancedManagerUiLibDestructor(
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
)
{
  EFI_STATUS                  Status;
  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gAdvancedManagerPrivate.DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mAdvancedManagerHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &gAdvancedManagerPrivate.ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  HiiRemovePackages (gAdvancedManagerPrivate.HiiHandle);

  return EFI_SUCCESS;
}
