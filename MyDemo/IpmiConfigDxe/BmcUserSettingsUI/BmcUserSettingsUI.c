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

/** @file BmcUserSettings.c
    Provides functionality to Add/Delete/Change BMC user
    settings.

**/

//----------------------------------------------------------------------

#include "BmcUserSettingsUI.h"
#include "IpmiLib.h"

//----------------------------------------------------------------------

//
// Global variables
//
BMC_USER_DETAILS_LIST       gBmcUserDetailsList[MAX_BMC_USER_COUNT];
UINT8                       gAllowableUser = 0;
UINT8                       gAllowableUserCh = 0;

BMC_USER_DETAILS            BmcAddUserDetails;
BMC_USER_DETAILS            BmcDeleteUserDetails;
BMC_USER_DETAILS            BmcChangeUserDetails;

// /// Global pointer used to store the call back paramters so they can be consumed by the legacy callbacks
// CALLBACK_PARAMETERS CallbackParametersPtr ;
// /**
//  * Function for use by legacy callbacks to retrieve the full callback paramters
//  * @return CALLBACK_PARAMETERS Pointer to the CALLBACK_PARAMTER structure for consumption by legacy callbacks
//  */
// CALLBACK_PARAMETERS* GetCallbackParameters(){
//     return &CallbackParametersPtr;
// }

/**
    Converts Unicode to ASCII Characters.

    @param UnicodeStr The Unicode string to be written to. The buffer must
                      be large enough.
    @param AsciiStr The ASCII string to be converted.

    @retval VOID
 
**/

VOID
IpmiUnicode2Ascii (
  IN      CHAR16         *UnicodeStr,
  IN OUT  CHAR8          *AsciiStr
   )
{

    while (*UnicodeStr != '\0') {
      *(AsciiStr++) = (CHAR8) *(UnicodeStr++);
    }

    *AsciiStr = '\0';

    return;
}

/**
    This function collects number of users allowed BMC.

    @param VOID

    @retval VOID
**/

VOID
GetMaxUserAllowable(
 VOID )
{

    EFI_STATUS                    Status;
    UINT8                         ChannelNumber;
    EFI_CHANNEL_INFO              GetChannelInfoResponse = {0};
    EFI_GET_USER_ACCESS_COMMAND   GetUserAccessCommand = {0};
    EFI_GET_USER_ACCESS_RESPONSE  GetUserAccessResponse = {0};
    UINT8                         GetChannelInfoResponseSize;
    UINT8                         GetUserAccessResponseSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside GetMaxUserAllowable Function \n"));
    for (ChannelNumber = 0; ChannelNumber <= MAX_CHANNEL_NUMBER; ChannelNumber++) {

        GetChannelInfoResponseSize = sizeof (GetChannelInfoResponse);
        Status = gIpmiTransport->SendIpmiCommand(
                          gIpmiTransport,
                          EFI_SM_NETFN_APP,
                          BMC_LUN,
                          EFI_APP_GET_CHANNEL_INFO,
                          (UINT8*)&ChannelNumber,
                          sizeof (ChannelNumber),
                          (UINT8*)&GetChannelInfoResponse,
                          &GetChannelInfoResponseSize );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of Channel %02x Get Channel Info Command: %r \n", ChannelNumber, Status));
        if (!EFI_ERROR(Status)) {
            if (GetChannelInfoResponse.ChannelMediumType == LAN_CHANNEL_TYPE) {
                GetUserAccessCommand.ChannelNo = ChannelNumber;
                GetUserAccessCommand.UserId = 1;
                GetUserAccessResponseSize = sizeof (GetUserAccessResponse);
                Status = gIpmiTransport->SendIpmiCommand(
                                                 gIpmiTransport,
                                                 EFI_SM_NETFN_APP,
                                                 BMC_LUN,
                                                 EFI_APP_GET_USER_ACCESS,
                                                 (UINT8 *)&GetUserAccessCommand,
                                                 sizeof (GetUserAccessCommand),
                                                 (UINT8*)&GetUserAccessResponse,
                                                 &GetUserAccessResponseSize );
                SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of Get User Access Command: %r \n", Status));
                if (!EFI_ERROR(Status)) {
                    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Maximum User Id allowable: %04x \n", GetUserAccessResponse.MaxUserId));
                    gAllowableUser = GetUserAccessResponse.MaxUserId;
                    gAllowableUserCh = ChannelNumber;
                    return;
                }
            }
        }
    }
}
/**
    This function collects all user names from BMC.

    @param VOID

    @retval VOID
**/

VOID
CollectAllBmcUserDetails (
  VOID )
{
    UINT8                       i;
    EFI_STATUS                  Status;
    EFI_GET_USER_NAME_COMMAND   GetUserNameCommandData = {};
    EFI_GET_USER_NAME_RESPONSE  GetUserNameResponseData = {};
    UINT8                       GetUserNameResponseDataSize;
    CHAR8                       *InvalidUserName = "Invalid";
    CHAR8                       *Reserved = "Reserved";

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside CollectAllBmcUserDetails Function \n"));

    //
    // Send Get User Name Command to BMC for all possible User IDs
    //
    GetUserNameResponseDataSize = sizeof (EFI_GET_USER_NAME_RESPONSE);

    for(i = 0; i <= gAllowableUser; i++) {
        gBmcUserDetailsList[i].UserId = i;
        if ((i == 0) || (i == 1)) {// User Id 0 & 1 are reserved as per IPMI specification
            CopyMem (gBmcUserDetailsList[i].UserName, Reserved, AsciiStrLen (Reserved));
            continue;
        }

        CopyMem ( gBmcUserDetailsList[i].UserName, InvalidUserName, AsciiStrLen (InvalidUserName));

        GetUserNameCommandData.UserId = i;
        GetUserNameResponseDataSize = sizeof (GetUserNameResponseData);
        ZeroMem (GetUserNameResponseData.UserName, MAX_BMC_USER_NAME_LENGTH );

        Status = gIpmiTransport->SendIpmiCommand(
                                       gIpmiTransport,
                                       EFI_SM_NETFN_APP,
                                       BMC_LUN,
                                       EFI_APP_GET_USER_NAME,
                                       (UINT8 *)&GetUserNameCommandData,
                                       sizeof (EFI_GET_USER_NAME_COMMAND),
                                       GetUserNameResponseData.UserName,
                                       &GetUserNameResponseDataSize );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of Get User Name Command: %r \n", Status));
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));

        if (!EFI_ERROR (Status)) {
            //
            // Copy the user name in User Details Global List
            //
            if (GetUserNameResponseData.UserName[0] != 0x00) {
                CopyMem ( gBmcUserDetailsList[i].UserName, GetUserNameResponseData.UserName, sizeof(GetUserNameResponseData));
            } 
        }
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Id: %x \n", gBmcUserDetailsList[i].UserId));
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Name: %a \n", gBmcUserDetailsList[i].UserName));
    }

    return;
}

/**
    Checks the given user name already available in global user list

    @param UserDetails Contains user name

    @retval TRUE User already available.
    @retval FALSE User not available.
**/

BOOLEAN
CheckUserNameAvailability (
  IN OUT BMC_USER_DETAILS   *UserDetails )
{
    UINT8       i;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside CheckUserNameAvailability Function...\n"));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: UserName: %a\n", UserDetails->UserName));

    for (i = 0; i <= gAllowableUser; i++) {

        if (AsciiStrCmp (gBmcUserDetailsList[i].UserName, UserDetails->UserName) == 0x00) {
            UserDetails->UserId = gBmcUserDetailsList[i].UserId;
            SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Available UserId: %x\n", UserDetails->UserId));
            SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning TRUE from CheckUserNameAvailability Function...\n"));
            return TRUE;
        }
    }

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning FALSE from CheckUserNameAvailability Function...\n"));
    return FALSE;
}

/**
    Checks add new user is possible or not.

    @param VOID

    @retval TRUE Add new user is possible.
    @retval FALSE Add new user is not possible.
**/

BOOLEAN
CheckAddNewUserPossibility (
  VOID )
{
    UINT8           i;
    CHAR8           *InvalidUserName = "Invalid";

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside CheckAddNewUserPossibility Function...\n"));
    for (i = 0; i <= gAllowableUser; i++){
        if (AsciiStrCmp (gBmcUserDetailsList[i].UserName, InvalidUserName) == 0x00) {
            SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning TRUE from CheckAddNewUserPossibility Function...\n"));
            return TRUE;
        }
    }
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning FALSE from CheckAddNewUserPossibility Function...\n"));
    return FALSE;
}

/**
    Send Set User Name command to BMC.

    @param UserId User Id.
    @param UserName User Name.

    @return EFI_STATUS
**/

EFI_STATUS
SendSetUserNameCommandToBmc(
  IN UINT8      UserId,
  IN CHAR8      *UserName)
{
    EFI_STATUS                      Status;
    EFI_SET_USER_NAME_COMMAND       SetUserName = {0};
    UINT8                           ResponseDataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside SendSetUserNameCommandToBmc function\n"));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Id: %x\n", UserId));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Name: %a\n", UserName));

    SetUserName.UserId = UserId;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AsciiStrLen(UserName): %x\n", AsciiStrLen(UserName)));
    CopyMem (&SetUserName.UserName, UserName, AsciiStrLen (UserName));
    ResponseDataSize = 0;

    Status = gIpmiTransport->SendIpmiCommand(
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_SET_USER_NAME,
                (UINT8 *)&SetUserName,
                sizeof (SetUserName),
                NULL,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "Status of Set User Name Command: %r \n", Status));

    return Status; 
}

/**
    Delete user details from global list.

    @param UserName User Name.

    @retval EFI_SUCCESS User details successfully deleted from list.
    @retval EFI_NOT_FOUND User is not available in list.
**/

EFI_STATUS
DeleteUserEntryFromList(
  IN CHAR8      *UserName )
{
    UINT8           i;
    CHAR8           *InvalidUserName = "Invalid";

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside DeleteUserEntryFromList function\n"));
    for (i = 0; i <= gAllowableUser; i++) {

        if (AsciiStrCmp (gBmcUserDetailsList[i].UserName, UserName) == 0x00) {
            AsciiStrCpy (gBmcUserDetailsList[i].UserName, InvalidUserName);
            SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: gBmcUserDetailsList[%x].UserName: %a\n", i, gBmcUserDetailsList[i].UserName));
            return EFI_SUCCESS;
        }

    }
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from DeleteUserEntryFromList function\n"));
    return EFI_NOT_FOUND;
}

/**
    Add user details to global list.

    @param UserDetails User details.

    @retval VOID
**/

VOID
AddUserEntryToList(
  IN BMC_USER_DETAILS      *UserDetails )
{
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside AddUserEntryToList Function...\n"));

    gBmcUserDetailsList[UserDetails->UserId].UserId = UserDetails->UserId;
    CopyMem ( gBmcUserDetailsList[UserDetails->UserId].UserName, UserDetails->UserName, sizeof(UserDetails->UserName));

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: gBmcUserDetailsList[UserDetails->UserId].UserId:%x\n", gBmcUserDetailsList[UserDetails->UserId].UserId));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: gBmcUserDetailsList[UserDetails->UserId].UserName:%a\n", gBmcUserDetailsList[UserDetails->UserId].UserName));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from AddUserEntryToList Function...\n"));

}

/**
    Parse the global list and get the available used id for new user.

    @param UserId User Id.

    @retval EFI_SUCCESS ID is available.
    @retval EFI_NOT_FOUND No more id is available.
**/

EFI_STATUS
GetAvailableUserId(
  IN OUT UINT8      *UserId)
{
    UINT8       i;
    CHAR8       *InvalidUserName = "Invalid";

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside GetAvailableUserId Function..."));

    for (i = 0; i <= gAllowableUser; i++){
        if (AsciiStrCmp (gBmcUserDetailsList[i].UserName, InvalidUserName) == 0x00) {
            *UserId = gBmcUserDetailsList[i].UserId;
            SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: User Id: %x", *UserId));
            SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Returning EFI_SUCCESS from GetAvailableUserId Function..."));
            return EFI_SUCCESS;
        }
    }
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning EFI_NOT_FOUND from GetAvailableUserId Function..."));
    return  EFI_NOT_FOUND;
}

/**
    Send Set user password command as per input data.

    @param UserId User Id.
    @param PasswordSize Password size(16 or 20 byte).
    @param Operation Tells what operation to perform.
    @param UserPassword Password.

    @return EFI_STATUS
**/

EFI_STATUS
PerformGivenUserPasswordAction(
  IN UINT8      UserId,
  IN UINT8      PasswordSize,
  IN UINT8      Operation,
  IN CHAR8      *UserPassword )
{
    EFI_SET_USER_PASSWORD_COMMAND_OF_SIZE_TWENTY        SetUserPassword = {0};
    EFI_STATUS                                          Status;
    UINT8                                               DataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside PerformGivenUserPasswordAction Function...\n"));

    ZeroMem (SetUserPassword.PasswordData, sizeof (SetUserPassword.PasswordData));
    CopyMem (SetUserPassword.PasswordData, UserPassword, AsciiStrLen(UserPassword));

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: UserId: %x\n", UserId));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: IsPasswordSize: %x\n", PasswordSize));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Operation: %x\n", Operation));

    SetUserPassword.UserId = UserId;
    SetUserPassword.PasswordSize = PasswordSize;
    SetUserPassword.Operation = Operation;
    DataSize = 0;

    Status = gIpmiTransport->SendIpmiCommand(
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_SET_USER_PASSWORD,
                (UINT8 *)&SetUserPassword,
                sizeof (SetUserPassword),
                NULL,
                &DataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Set User Password Command Status: %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));

    if( (!EFI_ERROR(Status)) && (gIpmiTransport->CommandCompletionCode == 0x80)) {
        return EFI_INVALID_PARAMETER;
    }
    return Status;
}

/**
    Deletes user name from BMC.

    @param UserId User Id.

    @return EFI_STATUS
**/

EFI_STATUS
DeleteUserNameFromBmc(
  IN UINT8      UserId )
{
    EFI_STATUS                  Status;
    CHAR8                       DeleteUserName[MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER];

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: DeleteUserNameFromBmc Function...\n"));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Id: %x\n", UserId));

    ZeroMem (DeleteUserName, MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER);
    Status = SendSetUserNameCommandToBmc ( UserId, DeleteUserName);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Set User Name Command Status: %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from DeleteUserNameFromBmc Function...\n"));
    return Status;

}

/**
    Displays text box in setup to get user password.

    @param HiiHandle HII Handle.
    @param TitleToken Display title token.
    @param UserPassword User password.

    @retval VOID
**/

VOID
DisplayTextBox(
  IN  VOID        *HiiHandle,
  IN  UINT16      TitleToken,
  IN  CHAR16      **UserPassword )
{
    // EFI_STATUS              Status;
    // TEXT_INPUT_TEMPLATE     BmcUserPasswordPrompt = {0};

    // SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Inside DisplayTextBox Function...\n"));

    //
    // Fill the Text input template
    //
    // BmcUserPasswordPrompt.ItemToken = STR_BMC_USER_PASSWORD;
    // BmcUserPasswordPrompt.MaxSize = MAX_BMC_USER_PASSWORD_LENGTH;
    // BmcUserPasswordPrompt.FilterKey = TSE_FILTER_KEY_NULL;
    // BmcUserPasswordPrompt.Hidden = TRUE;
    // BmcUserPasswordPrompt.Value = *UserPassword;

    //
    // Call DisplayTextBox() function to display the text box in setup and
    // get the Secret string.
    //
    // Status = gAmiPostMgr->DisplayTextBox(
    //             HiiHandle,
    //             TitleToken,
    //             &BmcUserPasswordPrompt,
    //             1,
    //             NULL );

    // SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of DisplayTextBox: %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from DisplayTextBox Function...\n"));

    return;

}

/**
    This function collects all BMC user details from BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcUserSettingsCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    EFI_STATUS                  Status = EFI_SUCCESS;
    CALLBACK_PARAMETERS         *CallbackParameters;
    STATIC BOOLEAN              IsFirstTimeCallBack = TRUE;
    BmcIpmiConfigData           *ServerMgmtConfiguration = NULL;
    UINTN                       SelectionBufferSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Key Value in BmcUserSettingsCallbackFunction function: %x\n", Key));

    //
    // Continue only for BMC_USER SETTING_KEY
    //
    if ( Key != BMC_USER_SETTINGS_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action: %x\n", CallbackParameters->Action));
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: IsFirstTimeCallBack: %x\n", IsFirstTimeCallBack));
#if COLLECT_BMC_USER_DETAILS_FOR_EVERY_BMC_USER_SETTING_CALLBACK == 0
    if (IsFirstTimeCallBack == FALSE) {
        return EFI_SUCCESS;
    }
#endif

    //
    // Locate IpmiTransport protocol
    //
    // Status = gBS->LocateProtocol (
    //             &gEfiDxeIpmiTransportProtocolGuid,
    //             NULL,
    //             (VOID **)&gIpmiTransport );
    // SERVER_IPMI_DEBUG ((EFI_D_LOAD, " IPMI: gEfiDxeIpmiTransportProtocolGuid Status: %r \n", Status));

    //
    // If locate protocol fails, display error message and return
    //
    // if(EFI_ERROR (Status)) {
    //     DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_IPMI_PROTOCOL_ERROR) );
    //     return Status;
    // }

    //
    // Collect maximum user allowable 
    //
    GetMaxUserAllowable ();

    if(IsFirstTimeCallBack) {
        CollectAllBmcUserDetails ();
    } else {
#if COLLECT_BMC_USER_DETAILS_FOR_EVERY_BMC_USER_SETTING_CALLBACK
        CollectAllBmcUserDetails ();
#endif
    }
    IsFirstTimeCallBack = FALSE;

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                   EfiBootServicesData,
                   SelectionBufferSize,
                   (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                   &SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);
    ZeroMem (ServerMgmtConfiguration->BmcUserList, sizeof(BMC_USER_DETAILS_LIST)*MAX_BMC_USER_COUNT);
    CopyMem (ServerMgmtConfiguration->BmcUserList,gBmcUserDetailsList, sizeof(BMC_USER_DETAILS_LIST)*gAllowableUser);
    ServerMgmtConfiguration->BmcUserAllowableMax = gAllowableUser;
    ServerMgmtConfiguration->BmcUserAllowableChannelNum = gAllowableUserCh;

    CHAR8 temp[1024] ;
    UINT8 i=0;
    UINT8 num = sizeof(BMC_USER_DETAILS_LIST)+3;
    CHAR8       *InvalidUserName = "Invalid";
    AsciiSPrint(temp, sizeof(temp), "%a-%a\n","UserId","UserName");
    for (i = 0; i <= gAllowableUser; i++){
        if (AsciiStrCmp (gBmcUserDetailsList[i].UserName, InvalidUserName) != 0x00) {
            AsciiSPrint(temp+num*i, sizeof(temp)-num*i, "%d-%a\n",gBmcUserDetailsList[i].UserId,gBmcUserDetailsList[i].UserName);
            SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: %a",temp+num*i));
        }
    }
    SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: %a",temp));
    CHAR16 temp16[1024];
    UnicodeSPrintAsciiFormat(temp16,sizeof(temp16),"%s",temp);
    InitString (HiiHandle,STRING_TOKEN(STR_BMC_USER_LIST_VAL),L"%s",temp16);
    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                   SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);


    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from BmcUserSettingsCallbackFunction function...\n"));
    return  EFI_SUCCESS;
}

/**
    This function validates whether add new user is possible or not.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcUserSettingsAddUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    EFI_STATUS                  Status = EFI_SUCCESS;
    CALLBACK_PARAMETERS         *CallbackParameters;
    BOOLEAN                     IsAddNewUserPossible;
    BmcIpmiConfigData           *ServerMgmtConfiguration = NULL;
    UINTN                       SelectionBufferSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Key Value in BmcUserSettingsAddUserCallbackFunction: %x\n", Key));

    //
    // Continue only for BMC_ADD USER KEY
    //
    if ( Key != ADD_BMC_USER_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action: %x\n", CallbackParameters->Action));
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    IsAddNewUserPossible = CheckAddNewUserPossibility ();

    if(IsAddNewUserPossible == FALSE) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Add New User Not Possible!!!\n"));
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_NEW_USER_ADD_ERROR) );
        *(CallbackParameters->ActionRequest) = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
        return  EFI_SUCCESS;
    }
    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                   EfiBootServicesData,
                   SelectionBufferSize,
                   (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                   &SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    ZeroMem (ServerMgmtConfiguration->AddBmcUserName, MAX_BMC_USER_NAME_LENGTH);
    ZeroMem (ServerMgmtConfiguration->AddBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
    ServerMgmtConfiguration->AddBmcChannelNo = 0;
    ServerMgmtConfiguration->AddBmcUserPrivilegeLimit  = 0;

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                   SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    FreePool (ServerMgmtConfiguration);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from BmcUserSettingsAddUserCallbackFunction...\n"));
    return  EFI_SUCCESS;
}
/**
    This function send set user name to BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcAddUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    BOOLEAN                             UserNameAlreadyAvailable;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcAddUserNameCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if (Key != ADD_BMC_USER_NAME_KEY) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    //
    // Convert User name format from UNICODE to ASCII
    //
    ZeroMem (BmcAddUserDetails.UserName, MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER);
    IpmiUnicode2Ascii (ServerMgmtConfiguration->AddBmcUserName, BmcAddUserDetails.UserName);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddUserName: %a\n", BmcAddUserDetails.UserName));

    //
    // Check the user name is already available in User Details list
    //
    UserNameAlreadyAvailable = CheckUserNameAvailability (&BmcAddUserDetails);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: UserNameAlreadyAvailable:%x\n", UserNameAlreadyAvailable));

    if(UserNameAlreadyAvailable) {
        //
        // Display User Name already available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_NAME_EXISTS));
        ZeroMem (ServerMgmtConfiguration->AddBmcUserName, MAX_BMC_USER_NAME_LENGTH);
        ZeroMem (&BmcAddUserDetails, sizeof (BmcAddUserDetails.UserName));
        ServerMgmtConfiguration->AddBmcChannelNo = 0; 
        ServerMgmtConfiguration->AddBmcUserPrivilegeLimit  = 0; 
    } else {
        Status = GetAvailableUserId (&(BmcAddUserDetails.UserId));
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of GetAvailableUserId Function = %r\n", Status));
        if (EFI_ERROR (Status)) {
            DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_ADD_NOT_POSSIBLE));
        } else {
            //
            // Send Set User Name Command
            //
            Status = SendSetUserNameCommandToBmc (BmcAddUserDetails.UserId, BmcAddUserDetails.UserName);
            SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Set User Name Command Status: %r\n", Status));
            if (EFI_ERROR (Status)){
                DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_NAME_CMD_FAIL) );
            }
        }
        if (EFI_ERROR (Status)) {
            ZeroMem (ServerMgmtConfiguration->AddBmcUserName, MAX_BMC_USER_NAME_LENGTH);
            ZeroMem (&BmcAddUserDetails, sizeof (BMC_USER_DETAILS));
            ServerMgmtConfiguration->AddBmcChannelNo = 0;
            ServerMgmtConfiguration->AddBmcUserPrivilegeLimit = 0;
        }
    }

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function adds user password to BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS

**/
EFI_STATUS
BmcAddUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcAddUserPasswordCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != ADD_BMC_USER_PASSWORD_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    //
    // Convert User Password format from UNICODE to ASCII
    //
    ZeroMem (BmcAddUserDetails.UserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
    IpmiUnicode2Ascii (ServerMgmtConfiguration->AddBmcUserPassword, BmcAddUserDetails.UserPassword);

    Status = PerformGivenUserPasswordAction (BmcAddUserDetails.UserId, TwentyByte, SetPassword, (CHAR8 *)BmcAddUserDetails.UserPassword);
    if( EFI_ERROR (Status) ) {

        //
        // Display User Name already available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_PASSWORD_FAIL) );
        ZeroMem (ServerMgmtConfiguration->AddBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH);
        ZeroMem (&BmcAddUserDetails.UserPassword, sizeof (BmcAddUserDetails.UserPassword));
    } else {
        AddUserEntryToList (&BmcAddUserDetails);
        ZeroMem (ServerMgmtConfiguration->AddBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH); //Clearing the password from browser data
    }

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    FreePool (ServerMgmtConfiguration);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from BmcAddUserPasswordCallbackFunction\n"));
    return EFI_SUCCESS;
}

/**
    This function sends get channel access command to BMC for given input.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcAddUserChannelCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    EFI_GET_CHANNEL_ACCESS_COMMAND      GetChannelAccessCommand = {0};
    EFI_GET_CHANNEL_ACCESS_RESPONSE     GetChannelAccessResponse = {0};
    UINT8                               GetChannelAccessResponseDataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcAddUserChannelCallbackFunction Key: %x \n", Key));
    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != ADD_BMC_USER_CHANNEL_NO_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if (CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User entered channel no = %x\n", ServerMgmtConfiguration->AddBmcChannelNo));

    GetChannelAccessCommand.ChannelNo = ServerMgmtConfiguration->AddBmcChannelNo;
    GetChannelAccessCommand.MemoryType = NonVolatile;// Non Volatile channel Access
    GetChannelAccessResponseDataSize = sizeof (EFI_GET_CHANNEL_ACCESS_RESPONSE);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_GET_CHANNEL_ACCESS,
                (UINT8 *)&GetChannelAccessCommand,
                sizeof (EFI_GET_CHANNEL_ACCESS_COMMAND),
                (UINT8 *)&GetChannelAccessResponse,
                &GetChannelAccessResponseDataSize);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of EFI_APP_GET_CHANNEL_ACCESS Command: %r \n", Status));

    if((EFI_ERROR (Status)) || (gIpmiTransport->CommandCompletionCode == 0x82)) {
        DisplayErrorMessagePopUp (HiiHandle, STRING_TOKEN(STR_GET_CHANNEL_ACCESS_CMD_FAIL));
        ServerMgmtConfiguration->AddBmcChannelNo = 0;
    } else {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: GetChannelAccessResponse.ChannelPriviledgeLimit: %x \n", GetChannelAccessResponse.ChannelPriviledgeLimit));
        ServerMgmtConfiguration->AddBmcUserMaximumPossibleUserPrivilegeLevel = GetChannelAccessResponse.ChannelPriviledgeLimit;
    }

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddBmcUserMaximumPossibleUserPrivilegeLevel(Before Set Browser data) = %x\n", ServerMgmtConfiguration->AddBmcUserMaximumPossibleUserPrivilegeLevel));

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddBmcUserMaximumPossibleUserPrivilegeLevel(After Set Browser data) = %x\n", ServerMgmtConfiguration->AddBmcUserMaximumPossibleUserPrivilegeLevel));

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function sends set user access command to BMC for given input.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcAddUserChannelPrivilegeLimitCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    EFI_SET_USER_ACCESS_COMMAND         SetUserAccess = {0};
    UINT8                               DataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcAddUserChannelPrivilegeLimitCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != ADD_BMC_USER_PRIVILEGE_LIMIT_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddBmcUserPrivilegeLimit = %x\n", ServerMgmtConfiguration->AddBmcUserPrivilegeLimit));

    //
    // Byte1
    //
    SetUserAccess.ChannelNo = ServerMgmtConfiguration->AddBmcChannelNo;
    SetUserAccess.EnableIpmiMessaging = 1;
    SetUserAccess.EnableUserLinkAuthetication = 1;
    SetUserAccess.UserRestrictedToCallback = 0;
    SetUserAccess.EnableChangeBitsInByte1 = 1;

    //
    // Byte2
    //
    SetUserAccess.UserID = BmcAddUserDetails.UserId;
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: SetUserAccess.UserID = %x\n", SetUserAccess.UserID));

    //
    // Byte3
    //
    SetUserAccess.UserPrivilegeLimit = ServerMgmtConfiguration->AddBmcUserPrivilegeLimit;

    //
    // Byte4
    //
    SetUserAccess.UserSimultaneousSessionLimit = 0;
    DataSize = 0;

    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_SET_USER_ACCESS,
                (UINT8 *)&SetUserAccess,
                sizeof (EFI_SET_USER_ACCESS_COMMAND),
                NULL,
                &DataSize);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of EFI_APP_SET_USER_ACCESS Command: %r \n", Status));

    if(EFI_ERROR (Status)) {
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_ACCESS_CMD_FAIL) );
        ServerMgmtConfiguration->AddBmcUserPrivilegeLimit = 0;

        //
        // Set Browser DATA
        //
        Status = HiiLibSetBrowserData (
                    SelectionBufferSize,
                    ServerMgmtConfiguration,
                    &gIpmiConfigFormSetGuid,
                    IpmiVarName );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
        FreePool (ServerMgmtConfiguration);
        return EFI_SUCCESS;
    }

    DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_ACCESS_CMD_PASS) );
    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function Clears previous NVRAM value before performing delete user action.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcDeleteUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    EFI_STATUS                         Status = EFI_SUCCESS;
    CALLBACK_PARAMETERS                *CallbackParameters;
    BmcIpmiConfigData     *ServerMgmtConfiguration = NULL;
    UINTN                              SelectionBufferSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Key Value in BmcDeleteUserCallbackFunction: %x\n", Key));

    //
    // Continue only for BMC_ADD USER KEY
    //
    if ( Key != DELETE_BMC_USER_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action: %x\n", CallbackParameters->Action));
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                   EfiBootServicesData,
                   SelectionBufferSize,
                   (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                   &SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    ZeroMem (ServerMgmtConfiguration->DeleteBmcUserName, MAX_BMC_USER_NAME_LENGTH); 
    ZeroMem (ServerMgmtConfiguration->DeleteBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                   SelectionBufferSize,
                   ServerMgmtConfiguration,
                   &gIpmiConfigFormSetGuid,
                   IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from BmcDeleteUserCallbackFunction...\n"));
    return  EFI_SUCCESS;
}

/**
    This function gets the user name for deleting a user from BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcDeleteUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    BOOLEAN                             UserNameAlreadyAvailable;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcDeleteUserNameCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != DELETE_BMC_USER_NAME_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    //
    // Convert User name format from UNICODE to ASCII
    //
    ZeroMem (BmcDeleteUserDetails.UserName, MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER);
    IpmiUnicode2Ascii (ServerMgmtConfiguration->DeleteBmcUserName, BmcDeleteUserDetails.UserName);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: BmcDeleteUserDetails.UserName = %a\n", BmcDeleteUserDetails.UserName));

    //
    // Check the user name is already available in User Details list
    //
    UserNameAlreadyAvailable = CheckUserNameAvailability (&BmcDeleteUserDetails);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: UserNameAlreadyAvailable:%x\n", UserNameAlreadyAvailable));

    if( !UserNameAlreadyAvailable ) {

        //
        // Display User Name not available pop up message
        //
        // DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_NAME_NOT_AVAILABLE) );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI:User Name Not Available!!!\n"));
        ZeroMem (ServerMgmtConfiguration->DeleteBmcUserName, MAX_BMC_USER_NAME_LENGTH);
        ZeroMem (&BmcDeleteUserDetails, sizeof (BmcDeleteUserDetails));

        //
        // Set Browser DATA
        //
        Status = HiiLibSetBrowserData (
                    SelectionBufferSize,
                    ServerMgmtConfiguration,
                    &gIpmiConfigFormSetGuid,
                    IpmiVarName );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    }

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function gets the user password for deleting a user from BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
BmcDeleteUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData     *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    // TEXT_INPUT_TEMPLATE                 BmcDeleteUserPasswordPrompt = {0};
    // CHAR16                              *UserPassword = NULL;
    // CHAR16                              Password[MAX_BMC_USER_PASSWORD_LENGTH];

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered BmcDeleteUserPasswordCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != DELETE_BMC_USER_PASSWORD_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD,"IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    //
    // Call DisplayTextBox function to get user password
    //
    // UserPassword = Password;
    // ZeroMem (UserPassword, MAX_BMC_USER_PASSWORD_LENGTH);
    // DisplayTextBox (HiiHandle, STR_BMC_USER_PASSWORD, (CHAR16 **)&UserPassword );
    // IpmiUnicode2Ascii ((CHAR16 *)UserPassword, BmcDeleteUserDetails.UserPassword);

    IpmiUnicode2Ascii ((CHAR16 *)ServerMgmtConfiguration->DeleteBmcUserPassword, BmcDeleteUserDetails.UserPassword);
    if ( ( (AsciiStrLen (BmcDeleteUserDetails.UserPassword) < MIN_BMC_USER_PASSWORD_LENGTH) || \
           (AsciiStrLen (BmcDeleteUserDetails.UserPassword) > MAX_BMC_USER_PASSWORD_LENGTH) ) ) {
        //
        // Display User Name not available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_PASSWORD_INVALID_SIZE) );
        ZeroMem (BmcDeleteUserDetails.UserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
        return EFI_INVALID_PARAMETER;
    }

    Status = PerformGivenUserPasswordAction ( BmcDeleteUserDetails.UserId, TwentyByte, TestPassword, BmcDeleteUserDetails.UserPassword);
    if (EFI_ERROR (Status)){
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_PASSWORD_MISMATCH) );
        return Status;
    }

    Status = DeleteUserNameFromBmc (BmcDeleteUserDetails.UserId);
    if ( EFI_ERROR(Status)){
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_NAME_DELETE_FAIL) );
        return Status;
    }

    Status = DeleteUserEntryFromList (BmcDeleteUserDetails.UserName);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "\nStatus of DeleteUserEntryFromList = %r\n", Status));
    if (EFI_ERROR (Status)){
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_NAME_DELETE_FAIL) );
        return Status;
    }

    DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_DELETED) );

    ZeroMem (&BmcDeleteUserDetails, sizeof (BmcDeleteUserDetails));
    ZeroMem (ServerMgmtConfiguration->DeleteBmcUserName, MAX_BMC_USER_NAME_LENGTH);

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                    SelectionBufferSize,
                    ServerMgmtConfiguration,
                    &gIpmiConfigFormSetGuid,
                    IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);

    return EFI_SUCCESS;
}

/**
    This function load default values to controls.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData     *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_BMC_USER_SETTINGS_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if (CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcCurrentUserName, MAX_BMC_USER_NAME_LENGTH);
    ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcCurrentUserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
    ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
    ServerMgmtConfiguration->ChangeUserSettingsValidBmcUser = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUser = 0;
    ServerMgmtConfiguration->ChangeUserSettingsValidBmcUserCredentials = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUserPrivilegeLimit = 0;

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function gets the current user name for changing a user details from BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsCurrentBmcUserNameCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData     *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    BOOLEAN                             UserNameAlreadyAvailable;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsCurrentBmcUserNameCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_CURRENT_USER_NAME_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n",Status));
    ASSERT_EFI_ERROR (Status);

    //
    // Convert User name format from UNICODE to ASCII
    //
    ZeroMem (BmcChangeUserDetails.UserName, MAX_BMC_USER_NAME_LENGTH_WITH_NULL_CHARACTER);
    IpmiUnicode2Ascii (ServerMgmtConfiguration->ChangeUserSettingsBmcCurrentUserName, BmcChangeUserDetails.UserName);

    //
    // Check the user name is already available in User Details list
    //
    UserNameAlreadyAvailable = CheckUserNameAvailability (&BmcChangeUserDetails);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: UserNameAlreadyAvailable:%x\n", UserNameAlreadyAvailable));

    if( !UserNameAlreadyAvailable ) {
        //
        // Display User Name not available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_NAME_NOT_FOUND) );
        ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcCurrentUserName, MAX_BMC_USER_NAME_LENGTH);
        ZeroMem (BmcChangeUserDetails.UserName, sizeof (BmcChangeUserDetails.UserName));
        ServerMgmtConfiguration->ChangeUserSettingsValidBmcUser = 0;
        ServerMgmtConfiguration->ChangeUserSettingsValidBmcUserCredentials = 0;
    } else {
        ServerMgmtConfiguration->ChangeUserSettingsValidBmcUser = 1;
        ServerMgmtConfiguration->ChangeUserSettingsValidBmcUserCredentials = 0;
    }

    ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUserPrivilegeLimit = 0;
    ServerMgmtConfiguration->ChangeUserSettingsBmcUser = 0;
    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function gets the current user password for changing a user details from BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsCurrentBmcUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    // CHAR16                              *UserPassword;
    // CHAR16                              Password[MAX_BMC_USER_PASSWORD_LENGTH];

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsCurrentBmcUserPasswordCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_USER_CURRENT_PASSWORD_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    //
    // Call DisplayTextBox function to get user password
    //
    // UserPassword = Password;
    // ZeroMem (UserPassword, MAX_BMC_USER_PASSWORD_LENGTH);
    // DisplayTextBox (HiiHandle, STR_BMC_USER_PASSWORD, (CHAR16 **)&UserPassword );
    // IpmiUnicode2Ascii ((CHAR16 *)UserPassword, BmcChangeUserDetails.UserPassword);
    IpmiUnicode2Ascii ((CHAR16 *)ServerMgmtConfiguration->ChangeUserSettingsBmcCurrentUserPassword, BmcChangeUserDetails.UserPassword);

    if ( ( (AsciiStrLen (BmcChangeUserDetails.UserPassword) < MIN_BMC_USER_PASSWORD_LENGTH) || \
           (AsciiStrLen (BmcChangeUserDetails.UserPassword) > MAX_BMC_USER_PASSWORD_LENGTH) ) ) {
        //
        // Display User Name not available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_INVALID_USER_PASSWORD) );
        ZeroMem (BmcChangeUserDetails.UserPassword, sizeof (BmcChangeUserDetails.UserPassword));
        return EFI_INVALID_PARAMETER;
    }

    Status = PerformGivenUserPasswordAction ( BmcChangeUserDetails.UserId, TwentyByte, TestPassword, BmcChangeUserDetails.UserPassword);
    if (EFI_ERROR (Status)){
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_PASSWORD_MISMATCH) );
        ZeroMem (BmcChangeUserDetails.UserPassword, sizeof (BmcChangeUserDetails.UserPassword));
        return Status;
    } else {
        ServerMgmtConfiguration->ChangeUserSettingsValidBmcUserCredentials = 1;
    }

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "\nStatus of HiiLibSetBrowserData() = %r\n",Status));

    gBS->FreePool (ServerMgmtConfiguration);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from ChangeUserSettingsCurrentBmcUserPasswordCallbackFunction \n"));
    return EFI_SUCCESS;
}

/**
    This function gets new user password for changing a user details in BMC.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsBmcUserPasswordCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsBmcUserPasswordCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_USER_PASSWORD_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    //
    // Convert User Password format from UNICODE to ASCII
    //
    ZeroMem (BmcChangeUserDetails.UserPassword, MAX_BMC_USER_PASSWORD_LENGTH_WITH_NULL_CHARACTER);
    IpmiUnicode2Ascii (ServerMgmtConfiguration->ChangeUserSettingsBmcUserPassword, BmcChangeUserDetails.UserPassword);

    Status = PerformGivenUserPasswordAction (BmcChangeUserDetails.UserId, TwentyByte, SetPassword, (CHAR8 *)BmcChangeUserDetails.UserPassword);
    if( EFI_ERROR (Status) ) {

        //
        // Display User Name already available pop up message
        //
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_PASSWORD_FAIL) );
        ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH);
        ZeroMem (&BmcChangeUserDetails.UserPassword, sizeof(BmcChangeUserDetails.UserPassword));
    } else {
        ZeroMem (ServerMgmtConfiguration->ChangeUserSettingsBmcUserPassword, MAX_BMC_USER_PASSWORD_LENGTH); //Clearing the password from browser data
    }

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    FreePool (ServerMgmtConfiguration);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Returning from ChangeUserSettingsBmcUserPasswordCallbackFunction\n"));
    return EFI_SUCCESS;
}

/**
    This function enables/disables a user in BMC as per given input.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsEnableOrDisableBmcUserCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsEnableOrDisableBmcUserCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_USER_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "\n CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "\n ServerMgmtConfiguration->ChangeUserSettingsBmcUser: %x\n", ServerMgmtConfiguration->ChangeUserSettingsBmcUser));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI:BmcChangeUserDetails.UserId %x\n", BmcChangeUserDetails.UserId));
    Status = PerformGivenUserPasswordAction( BmcChangeUserDetails.UserId, TwentyByte, ServerMgmtConfiguration->ChangeUserSettingsBmcUser, BmcChangeUserDetails.UserPassword);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User Enable/Disable Status: %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    if (EFI_ERROR (Status)) {
        if (ServerMgmtConfiguration->ChangeUserSettingsBmcUser) {
            DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_ENABLE_OPERATION_FAIL) );
        } else {
            DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_DISABLE_OPERATION_FAIL) );
        }
        ServerMgmtConfiguration->ChangeUserSettingsBmcUser = 0;

        //
        // Set Browser DATA
        //
        Status = HiiLibSetBrowserData (
                    SelectionBufferSize,
                    ServerMgmtConfiguration,
                    &gIpmiConfigFormSetGuid,
                    IpmiVarName );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "\nStatus of HiiLibSetBrowserData() = %r\n", Status));
    } else {
        if (ServerMgmtConfiguration->ChangeUserSettingsBmcUser) {
            DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_ENABLE_OPERATION_PASS) );
        } else {
            DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_USER_DISABLE_OPERATION_PASS) );
        }
    }

    gBS->FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function sends get channel access command for the given channel and
    determines the maximum possible privilege level for the channel.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsBmcUserChannelCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    EFI_GET_CHANNEL_ACCESS_COMMAND      GetChannelAccessCommand = {0};
    EFI_GET_CHANNEL_ACCESS_RESPONSE     GetChannelAccessResponse = {0};
    UINT8                               GetChannelAccessResponseDataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsBmcUserChannelCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_USER_CHANNEL_NO_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR(Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);

    //
    // Get Browser Data
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR (Status);

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: User entered channel no = %x\n", ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo));

    GetChannelAccessCommand.ChannelNo = ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo;
    GetChannelAccessCommand.MemoryType = NonVolatile;// Non Volatile channel Access
    GetChannelAccessResponseDataSize = sizeof (EFI_GET_CHANNEL_ACCESS_RESPONSE);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_GET_CHANNEL_ACCESS,
                (UINT8 *)&GetChannelAccessCommand,
                sizeof(EFI_GET_CHANNEL_ACCESS_COMMAND),
                (UINT8 *)&GetChannelAccessResponse,
                &GetChannelAccessResponseDataSize);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of EFI_APP_GET_CHANNEL_ACCESS Command: %r \n", Status));

    if(EFI_ERROR(Status)) {
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_GET_CHANNEL_ACCESS_CMD_FAIL) );
        ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo = 0;
    } else {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: GetChannelAccessResponse.ChannelPriviledgeLimit: %x \n", GetChannelAccessResponse.ChannelPriviledgeLimit));
        ServerMgmtConfiguration->ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel = GetChannelAccessResponse.ChannelPriviledgeLimit;
    }
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddBmcUserMaximumPossibleUserPrivilegeLevel(Before Set Browser data) = %x\n", ServerMgmtConfiguration->ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel));

    //
    // Set Browser DATA
    //
    Status = HiiLibSetBrowserData (
                SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: AddBmcUserMaximumPossibleUserPrivilegeLevel(After Set Browser data) = %x\n", ServerMgmtConfiguration->ChangeUserSettingsBmcUserMaximumPossibleUserPrivilegeLevel));

    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}

/**
    This function sets the privilege level for a user as per the given input.

    @param HiiHandle - A handle that was previously registered in the
                               HII Database.
    @param Class - Class of the Form Callback Protocol passed in
    @param SubClass - Sub Class of the Form Callback Protocol passed in
    @param Key - Key of the Form Callback Protocol passed in

    @return EFI_STATUS
**/

EFI_STATUS
ChangeUserSettingsBmcUserChannelPrivilegeLimitCallbackFunction(
  IN  EFI_HII_HANDLE    HiiHandle,
  IN  UINT16            Class,
  IN  UINT16            SubClass,
  IN  UINT16            Key )
{
    BmcIpmiConfigData      *ServerMgmtConfiguration = NULL;
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    CALLBACK_PARAMETERS                 *CallbackParameters;
    EFI_SET_USER_ACCESS_COMMAND         SetUserAccess = {0};
    UINT8                               DataSize;

    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Entered ChangeUserSettingsBmcUserChannelPrivilegeLimitCallbackFunction Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( Key != CHANGE_USER_SETTINGS_BMC_USER_PRIVILEGE_LIMIT_KEY ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Callback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Get the call back parameters and verify the action
    //
    CallbackParameters = GetCallbackParameters ();
    if ( CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD ) {
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CallbackParameters->Action == EFI_BROWSER_ACTION_DEFAULT_STANDARD... so return EFI_UNSUPPORTED\n"));
        return  EFI_UNSUPPORTED;
    }
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
                (VOID **)&ServerMgmtConfiguration );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)ServerMgmtConfiguration , SelectionBufferSize);
    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                ServerMgmtConfiguration,
                &gIpmiConfigFormSetGuid,
                IpmiVarName );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibGetBrowserData() = %r\n", Status));
    ASSERT_EFI_ERROR(Status);
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: ChangeUserSettingsBmcUserPrivilegeLimit = %x\n", ServerMgmtConfiguration->ChangeUserSettingsBmcUserPrivilegeLimit));


    //
    // Byte1
    //
    SetUserAccess.ChannelNo = ServerMgmtConfiguration->ChangeUserSettingsBmcChannelNo;
    SetUserAccess.EnableIpmiMessaging = 1;
    SetUserAccess.EnableUserLinkAuthetication = 1;
    SetUserAccess.UserRestrictedToCallback = 0;
    SetUserAccess.EnableChangeBitsInByte1 = 1;

    //
    // Byte2
    //
    SetUserAccess.UserID = BmcChangeUserDetails.UserId;
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: SetUserAccess.UserID = %x\n", SetUserAccess.UserID));

    //
    // Byte3
    //
    SetUserAccess.UserPrivilegeLimit = ServerMgmtConfiguration->ChangeUserSettingsBmcUserPrivilegeLimit;

    //
    // Byte4
    //
    SetUserAccess.UserSimultaneousSessionLimit = 0;

    DataSize = 0;
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_APP,
                BMC_LUN,
                EFI_APP_SET_USER_ACCESS,
                (UINT8 *)&SetUserAccess,
                sizeof(EFI_SET_USER_ACCESS_COMMAND),
                NULL,
                &DataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: CommandCompletionCode: %x \n", gIpmiTransport->CommandCompletionCode));
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of EFI_APP_SET_USER_ACCESS Command: %r \n", Status));

    if(EFI_ERROR(Status)) {
        DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_ACCESS_CMD_FAIL) );
        ServerMgmtConfiguration->ChangeUserSettingsBmcUserPrivilegeLimit = 0;

        //
        // Set Browser DATA
        //
        Status = HiiLibSetBrowserData (
                    SelectionBufferSize,
                    ServerMgmtConfiguration,
                    &gIpmiConfigFormSetGuid,
                    IpmiVarName );
        SERVER_IPMI_DEBUG ((EFI_D_LOAD, "IPMI: Status of HiiLibSetBrowserData() = %r\n", Status));
        FreePool (ServerMgmtConfiguration);
        return EFI_SUCCESS;
    }
    DisplayErrorMessagePopUp ( HiiHandle, STRING_TOKEN(STR_SET_USER_ACCESS_CMD_PASS) );
    FreePool (ServerMgmtConfiguration);
    return EFI_SUCCESS;
}


