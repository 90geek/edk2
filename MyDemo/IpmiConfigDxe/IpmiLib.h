#ifndef __IPMI_LIB_H__
#define __IPMI_LIB_H__

#include "IpmiConfig.h"
#include <Protocol/HiiConfigAccess.h>

#ifndef SERVER_IPMI_DEBUG
#define SERVER_IPMI_DEBUG(Arguments) DEBUG(Arguments)
#endif
#ifndef SERVER_IPMI_TRACE
#define SERVER_IPMI_TRACE(Arguments) DEBUG(Arguments)
#endif
#define IPMI_STALL 1000
#define EFI_BMC_IPMI_VARIABLE_NAME L"BmcIpmiConfigVar"
typedef UINT16  STRING_REF;

extern EFI_IPMI_TRANSPORT  *gIpmiTransport;
extern CHAR16 IpmiVarName[];
typedef struct{
    EFI_HII_CONFIG_ACCESS_PROTOCOL *This;
    EFI_BROWSER_ACTION Action;
    EFI_QUESTION_ID KeyValue;
    UINT8 Type;
    EFI_IFR_TYPE_VALUE *Value;
    EFI_BROWSER_ACTION_REQUEST *ActionRequest;
} CALLBACK_PARAMETERS;

CALLBACK_PARAMETERS* GetCallbackParameters();

EFI_STATUS HiiLibGetBrowserData(
    IN OUT UINTN *BufferSize, OUT VOID *Buffer, 
    IN CONST EFI_GUID *VarStoreGuid OPTIONAL,
    IN CONST CHAR16 *VarStoreName  OPTIONAL
);

EFI_STATUS HiiLibSetBrowserData(
    IN UINTN BufferSize, IN VOID *Buffer, 
    IN CONST EFI_GUID *VarStoreGuid, OPTIONAL 
    IN CONST CHAR16 *VarStoreName  OPTIONAL
);

EFI_STATUS
EFIAPI
IpmiHiiGetString (
  IN       EFI_HII_HANDLE  HiiHandle,
  IN       EFI_STRING_ID   StringId,
  IN       EFI_STRING      String,
  IN       UINTN           *StringSize,
  IN CONST CHAR8           *Language  OPTIONAL
  );

VOID
InitString (
  IN EFI_HII_HANDLE HiiHandle,
  IN STRING_REF     StrRef,
  IN CHAR16         *sFormat,
  IN ... );

VOID
DisplayErrorMessagePopUp(
  IN EFI_HII_HANDLE HiiHandle,
  IN UINT16         Message );

#endif
