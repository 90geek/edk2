#include "IpmiLib.h"
#include "BmcPowerRestoreUI.h"

CHAR16 VariableName[] = L"SetPowerRestorePolicy";

EFI_STATUS
BmcGetPowerRestoreCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    BmcIpmiConfigData       *BmcIpmiData;
    UINTN                Size = sizeof(UINTN);
    UINT32                  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |EFI_VARIABLE_BOOTSERVICE_ACCESS;
    UINTN                OldState = UNKNOWN;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: %a Key: %x \n",__FUNCTION__, Key));

    if ( Key != BMC_POWER_RESTORE_POLICY_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_FORM_OPEN ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }
    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&BmcIpmiData );
    if (EFI_ERROR (Status)) {
        return Status;
    }

    ZeroMem((VOID *)BmcIpmiData , SelectionBufferSize);
    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                BmcIpmiData,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: PowerRes = %x\n", BmcIpmiData->PowerRes));

    Status = gRT->GetVariable ((CHAR16*)VariableName, &gIpmiSetPowerRestorePolicyVendorGuid, &Attributes, &Size, &OldState);
    ASSERT_EFI_ERROR(Status);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: OldPowerRes = %x\n", OldState));

    if(BmcIpmiData->PowerRes != OldState) {

      BmcIpmiData->PowerRes = OldState;
      //
      // Set Browser DATA
      //
      Status = HiiLibSetBrowserData (
                  SelectionBufferSize,
                  BmcIpmiData,
                  &gIpmiConfigFormSetGuid,
                  IpmiVarName );
      SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    }
    FreePool (BmcIpmiData);
    return EFI_SUCCESS;
}


EFI_STATUS
BmcSetPowerRestoreCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    EFI_STATUS              Status = EFI_SUCCESS;
    UINTN                   SelectionBufferSize;
    CALLBACK_PARAMETERS     *CallbackParameters;
    BmcIpmiConfigData       *BmcIpmiData;
    UINTN                Size = sizeof(UINTN);
    UINT32                  Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |EFI_VARIABLE_BOOTSERVICE_ACCESS;
    UINT8                RequestData[1];
    UINT8                ResponseDataSize = 1,ResponseData = 0;
    UINTN                NewState = UNKNOWN;
    UINTN                OldState = UNKNOWN;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: %a Key: %x \n",__FUNCTION__, Key));

    if ( Key != BMC_POWER_RESTORE_POLICY_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }
    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&BmcIpmiData );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)BmcIpmiData , SelectionBufferSize);
    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                BmcIpmiData,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: PowerRes = %x\n", BmcIpmiData->PowerRes));

    NewState = BmcIpmiData->PowerRes;
    RequestData[0] = BmcIpmiData->PowerRes;

    Status = gRT->GetVariable ((CHAR16*)VariableName, &gIpmiSetPowerRestorePolicyVendorGuid, &Attributes, &Size, &OldState);
    DbgPrint(DEBUG_INFO,"------get variable (Status:%r) (0x%x)------\n",Status,OldState);

    if(Status == EFI_NOT_FOUND || (Status == EFI_SUCCESS && NewState != OldState)){

        Status = gIpmiTransport->SendIpmiCommand (
                                                  gIpmiTransport,
                                                  EFI_SM_NETFN_CHASSIS,
                                                  BMC_LUN,
                                                  EFI_CHASSIS_SET_POWER_RESTORE_POLICY,
                                                  (UINT8 *) RequestData,
                                                  0x01,// CommandDataSize
                                                  (UINT8 *) &ResponseData,
                                                  (UINT8 *) &ResponseDataSize);
        if(Status != EFI_SUCCESS){
          DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_POWER_FAILD) );
          return Status;
        }

        Status = gRT->SetVariable ((CHAR16*)VariableName, &gIpmiSetPowerRestorePolicyVendorGuid, Attributes, Size, &NewState);
        ASSERT_EFI_ERROR(Status);

        return EFI_SUCCESS;
      }else if(Status == EFI_SUCCESS && NewState == OldState){
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_POWER_REPEAT));
        return EFI_SUCCESS;
      }

    FreePool (BmcIpmiData);
    return EFI_SUCCESS;
}

