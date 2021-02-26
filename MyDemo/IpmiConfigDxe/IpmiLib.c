#include "IpmiLib.h"


EFI_IPMI_TRANSPORT  *gIpmiTransport;

/// Global pointer used to store the call back paramters so they can be consumed by the legacy callbacks
CALLBACK_PARAMETERS CallbackParametersPtr ;
/**
 * Function for use by legacy callbacks to retrieve the full callback paramters
 * @return CALLBACK_PARAMETERS Pointer to the CALLBACK_PARAMTER structure for consumption by legacy callbacks
 */
CALLBACK_PARAMETERS* GetCallbackParameters(){
    return &CallbackParametersPtr;
}

//*************************************************************************
// Name: HiiLibGetBrowserData
//
// Description: Retrieves uncommitted state data from the HII browser.
//  Only works in UEFI 2.1 mode.
//
// Input:
//   IN OUT UINTN *BufferSize - A pointer to the size of the buffer associated with Buffer. 
//     On input, the size in bytes of Buffer. On output, the size of data returned in Buffer.
//   OUT VOID *Buffer - A data returned from an IFR browser. 
//   IN CONST EFI_GUID *VarStoreGuid OPTIONAL - An optional field to indicate the target variable GUID name to use.
//   IN CONST CHAR16 *VarStoreName  OPTIONAL  - An optional field to indicate the target variable name.
//
// Output:
//   EFI_STATUS
//          
//<AMI_PHDR_END>
//*************************************************************************
EFI_STATUS HiiLibGetBrowserData(
    IN OUT UINTN *BufferSize, OUT VOID *Buffer, 
    IN CONST EFI_GUID *VarStoreGuid OPTIONAL,
    IN CONST CHAR16 *VarStoreName  OPTIONAL
)
{
  BOOLEAN Status;

  Status = HiiGetBrowserData(VarStoreGuid, VarStoreName, *BufferSize, (UINT8 *)Buffer);
  if(Status)
    return EFI_SUCCESS;
  else
    return EFI_NOT_FOUND;
}

//*************************************************************************
// Name: HiiLibSetBrowserData
//
// Description: Updates uncommitted state data of the HII browser.
//  Only works in UEFI 2.1 mode.
//
// Input:
//   IN UINTN BufferSize - Size of the buffer associated with Buffer. 
//   IN VOID *Buffer - A data to send to an IFR browser. 
//   IN CONST EFI_GUID *VarStoreGuid OPTIONAL - An optional field to indicate the target variable GUID name to use.
//   IN CONST CHAR16 *VarStoreName  OPTIONAL  - An optional field to indicate the target variable name.
//
// Output:
//   EFI_STATUS
//          
//*************************************************************************
EFI_STATUS HiiLibSetBrowserData(
    IN UINTN BufferSize, IN VOID *Buffer, 
    IN CONST EFI_GUID *VarStoreGuid, OPTIONAL 
    IN CONST CHAR16 *VarStoreName  OPTIONAL
)
{
  BOOLEAN Status;

  Status = HiiSetBrowserData(VarStoreGuid, VarStoreName, BufferSize, (UINT8 *) Buffer, NULL);
  if(Status)
    return EFI_SUCCESS;
  else
    return EFI_NOT_FOUND;

}

/**
  Retrieves a string from a string package in a specific language.  

  @param  HiiHandle    HII Handle value.
  @param  StringId     The identifier of the string to retrieved from the string 
                         package associated with HiiHandle.
  @param  String       Pointer to store the retrieved String.
  @param  StringSize   Size of the required string.   
  @param  Language     The language of the string to retrieve.  If this parameter 
                         is NULL, then the current platform language is used.  The 
                         format of Language must follow the language format assumed 
                         the HII Database.

  @retval EFI_SUCCESS  The string specified by StringId is present in the string package.
  @retval Other       The string was not returned.

**/

EFI_STATUS
EFIAPI
IpmiHiiGetString (
  IN       EFI_HII_HANDLE  HiiHandle,
  IN       EFI_STRING_ID   StringId,
  IN       EFI_STRING      String,
  IN       UINTN           *StringSize,
  IN CONST CHAR8           *Language  OPTIONAL
  )
{
  CHAR16 *NewString = NULL;

  NewString = HiiGetString (HiiHandle, StringId, NULL);
  CopyMem (String, NewString,*StringSize); 

  if (NewString != NULL) {
    FreePool (NewString);
    NewString = NULL;
  }
  return EFI_SUCCESS;
}

/**
    This function updates a string defined by the StrRef Parameter in the HII
    database with the string and data passed in.

    @param HiiHandle - handle that indicates which HII Package that
                   is being used
    @param StrRef  - String Token defining which string in the
        database to update
    @param sFormat - string with format descriptors in it
    @param  ...    - extra parameters that define data that correlate
                  to the format descriptors in the String

    @retval VOID

**/

VOID
InitString (
  IN EFI_HII_HANDLE HiiHandle,
  IN STRING_REF     StrRef,
  IN CHAR16         *sFormat,
  IN ... )
{
    CHAR16 s[1024];
    VA_LIST ArgList;
    VA_START(ArgList,sFormat);
    UnicodeVSPrint(s, sizeof(s), sFormat, ArgList);
    VA_END(ArgList);
    HiiSetString(HiiHandle, StrRef, s, NULL);
}

/**
    Displays error message pop up in setup.

    @param HiiHandle - Handle to HII database.
    @param Message - Error message token value.

    @retval VOID
**/

VOID
DisplayErrorMessagePopUp(
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Message )
{
  CHAR16 *NewString = NULL;
  EFI_INPUT_KEY Key;

  NewString = HiiGetString ( HiiHandle, Message, NULL);
    ASSERT (NewString != NULL);
  do {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"",
        NewString,
        L"Press ESC or ENTER to continue ...",
        L"",
        NULL
        );
    } while ((Key.ScanCode != SCAN_ESC) && (Key.UnicodeChar != CHAR_CARRIAGE_RETURN));

    return;
}


