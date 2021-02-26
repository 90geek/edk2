/** @file
The functions for Boot Maintainence Main menu.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <Library/DebugLib.h>
#include "AdvancedManager.h"
#include "AdvancedManagerCustomizedUiSupport.h"

#define UI_HII_DRIVER_LIST_SIZE               0x8

typedef struct {
  EFI_STRING_ID   PromptId;
  EFI_STRING_ID   HelpId;
  EFI_STRING_ID   DevicePathId;
  EFI_GUID        FormSetGuid;
  BOOLEAN         EmptyLineAfter;
} UI_HII_DRIVER_INSTANCE;

STATIC UI_HII_DRIVER_INSTANCE       *gHiiDriverList;


/**
  Create empty line menu in the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

**/
VOID
AdvancedCreateEmptyLine (
  IN EFI_HII_HANDLE              HiiHandle,
  IN VOID                        *StartOpCodeHandle
  )
{
  HiiCreateSubTitleOpCode (StartOpCodeHandle, STRING_TOKEN (STR_NULL_STRING), 0, 0, 0);
}

/**
  Extract device path for given HII handle and class guid.

  @param Handle          The HII handle.

  @retval  NULL          Fail to get the device path string.
  @return  PathString    Get the device path string.

**/
static CHAR16 *
ExtractDevicePathFromHandle (
  IN      EFI_HII_HANDLE      Handle
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       DriverHandle;

  ASSERT (Handle != NULL);

  if (Handle == NULL) {
    return NULL;
  }

  Status = gHiiDatabase->GetPackageListHandle (gHiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ConvertDevicePathToText(DevicePathFromHandle (DriverHandle), FALSE, FALSE);
}

/**
  Check whether this driver need to be shown in the front page.

  @param    HiiHandle           The hii handle for the driver.
  @param    Guid                The special guid for the driver which is the target.
  @param    PromptId            Return the prompt string id.
  @param    HelpId              Return the help string id.
  @param    FormsetGuid         Return the formset guid info.

  @retval   EFI_SUCCESS         Search the driver success

**/
static BOOLEAN
IsRequiredDriver (
  IN  EFI_HII_HANDLE              HiiHandle,
  IN  EFI_GUID                    *Guid,
  OUT EFI_STRING_ID               *PromptId,
  OUT EFI_STRING_ID               *HelpId,
  OUT VOID                        *FormsetGuid
  )
{
  EFI_STATUS                  Status;
  UINT8                       ClassGuidNum;
  EFI_GUID                    *ClassGuid;
  EFI_IFR_FORM_SET            *Buffer;
  UINTN                       BufferSize;
  UINT8                       *Ptr;
  UINTN                       TempSize;
  BOOLEAN                     RetVal;

  Status = HiiGetFormSetFromHiiHandle(HiiHandle, &Buffer,&BufferSize);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  RetVal = FALSE;
  TempSize = 0;
  Ptr = (UINT8 *) Buffer;
  while(TempSize < BufferSize)  {
    TempSize += ((EFI_IFR_OP_HEADER *) Ptr)->Length;

    if (((EFI_IFR_OP_HEADER *) Ptr)->Length <= OFFSET_OF (EFI_IFR_FORM_SET, Flags)){
      Ptr += ((EFI_IFR_OP_HEADER *) Ptr)->Length;
      continue;
    }

    ClassGuidNum = (UINT8) (((EFI_IFR_FORM_SET *)Ptr)->Flags & 0x3);
    ClassGuid = (EFI_GUID *) (VOID *)(Ptr + sizeof (EFI_IFR_FORM_SET));
    while (ClassGuidNum-- > 0) {
      DEBUG((DEBUG_INFO,"%a-%d-%g----%g\n",__func__,__LINE__,Guid,ClassGuid));
      if (!CompareGuid (Guid, ClassGuid)){
        ClassGuid ++;
        continue;
      }

      *PromptId = ((EFI_IFR_FORM_SET *)Ptr)->FormSetTitle;
      *HelpId = ((EFI_IFR_FORM_SET *)Ptr)->Help;
      CopyMem (FormsetGuid, &((EFI_IFR_FORM_SET *) Ptr)->Guid, sizeof (EFI_GUID));
      RetVal = TRUE;
    }
  }

  FreePool (Buffer);

  return RetVal;
}

/**
  Search the drivers in the system which need to show in the front page
  and insert the menu to the front page.

  @param    HiiHandle           The hii handle for the Uiapp driver.
  @param    ClassGuid           The class guid for the driver which is the target.
  @param    SpecialHandlerFn    The pointer to the specail handler function, if any.
  @param    StartOpCodeHandle   The opcode handle to save the new opcode.

  @retval   EFI_SUCCESS         Search the driver success

**/
EFI_STATUS
AdvancedListThirdPartyDrivers (
  IN EFI_HII_HANDLE              HiiHandle,
  IN EFI_GUID                    *ClassGuid,
  IN DRIVER_SPECIAL_HANDLER      SpecialHandlerFn,
  IN VOID                        *StartOpCodeHandle
  )
{
  UINTN                       Index;
  EFI_STRING                  String;
  EFI_STRING_ID               Token=0;
  EFI_STRING_ID               TokenHelp=0;
  EFI_HII_HANDLE              *HiiHandles;
  CHAR16                      *DevicePathStr;
  UINTN                       Count;
  UINTN                       CurrentSize;
  UI_HII_DRIVER_INSTANCE      *DriverListPtr;
  EFI_STRING                  NewName;
  BOOLEAN                     EmptyLineAfter;

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  if (gHiiDriverList != NULL) {
    FreePool (gHiiDriverList);
  }

  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  gHiiDriverList = AllocateZeroPool (UI_HII_DRIVER_LIST_SIZE * sizeof (UI_HII_DRIVER_INSTANCE));
  ASSERT (gHiiDriverList != NULL);
  DriverListPtr = gHiiDriverList;
  CurrentSize = UI_HII_DRIVER_LIST_SIZE;

  for (Index = 0, Count = 0; HiiHandles[Index] != NULL; Index++) {
    if (!IsRequiredDriver (HiiHandles[Index], ClassGuid, &Token, &TokenHelp, &gHiiDriverList[Count].FormSetGuid)) {
      continue;
    }

    String = HiiGetString (HiiHandles[Index], Token, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    } else if (SpecialHandlerFn != NULL) {
      //
      // Check whether need to rename the driver name.
      //
      EmptyLineAfter = FALSE;
      if (SpecialHandlerFn (String, &NewName, &EmptyLineAfter)) {
        FreePool (String);
        String = NewName;
        DriverListPtr[Count].EmptyLineAfter = EmptyLineAfter;
      }
    }
    DriverListPtr[Count].PromptId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    String = HiiGetString (HiiHandles[Index], TokenHelp, NULL);
    if (String == NULL) {
      String = HiiGetString (HiiHandle, STRING_TOKEN (STR_MISSING_STRING), NULL);
      ASSERT (String != NULL);
    }
    DriverListPtr[Count].HelpId = HiiSetString (HiiHandle, 0, String, NULL);
    FreePool (String);

    DevicePathStr = ExtractDevicePathFromHandle(HiiHandles[Index]);
    if (DevicePathStr != NULL){
      DriverListPtr[Count].DevicePathId = HiiSetString (HiiHandle, 0, DevicePathStr, NULL);
      FreePool (DevicePathStr);
    } else {
      DriverListPtr[Count].DevicePathId = 0;
    }

    Count++;
    if (Count >= CurrentSize) {
      DriverListPtr = ReallocatePool (
                        CurrentSize * sizeof (UI_HII_DRIVER_INSTANCE),
                        (Count + UI_HII_DRIVER_LIST_SIZE)
                        * sizeof (UI_HII_DRIVER_INSTANCE),
                        gHiiDriverList
                        );
      ASSERT (DriverListPtr != NULL);
      gHiiDriverList = DriverListPtr;
      CurrentSize += UI_HII_DRIVER_LIST_SIZE;
    }
  }

  FreePool (HiiHandles);

  Index = 0;
  while (gHiiDriverList[Index].PromptId != 0) {
    HiiCreateGotoExOpCode (
      StartOpCodeHandle,
      0,
      gHiiDriverList[Index].PromptId,
      gHiiDriverList[Index].HelpId,
      0,
      (EFI_QUESTION_ID) (Index + ADVANCED_PAGE_KEY_DRIVER),
      0,
      &gHiiDriverList[Index].FormSetGuid,
      gHiiDriverList[Index].DevicePathId
    );
    DEBUG((DEBUG_INFO,"%a-%d--0x%x--0x%x--0x%x--%g\n",__func__,__LINE__,
        gHiiDriverList[Index].PromptId,
        gHiiDriverList[Index].HelpId,
        gHiiDriverList[Index].DevicePathId,
        &gHiiDriverList[Index].FormSetGuid));

    if (gHiiDriverList[Index].EmptyLineAfter) {
      AdvancedCreateEmptyLine (HiiHandle, StartOpCodeHandle);
    }

    Index ++;
  }

  DEBUG((DEBUG_INFO,"%a-%d\n",__func__,__LINE__));
  return EFI_SUCCESS;
}
