/** @file BmcLanParam.c
    Reads the LAN parameters from BMC and updates in Setup
    and Verifies the Static BMC Network configuration with Setup Callback

**/

//----------------------------------------------------------------------

// #include <Token.h>
// #include <AmiDxeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
// #include "BmcLanConfig.h"
// #include <ServerMgmtSetup.h>
// #include <ServerMgmtSetupVariable.h>
// #include <Protocol/AMIPostMgr.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Library/HiiLib.h>
#include <Library/DebugLib.h>
// #include <Include/IpmiNetFnTransportDefinitions.h>
#include <Protocol/IPMITransportProtocol.h>
#include "IpmiLib.h"
#include "NVDataStruc.h"
#include "BmcLan.h"

//----------------------------------------------------------------------

#define STRING_BUFFER_LENGTH        4000

//
// Function Prototype
//
VOID InitString(EFI_HII_HANDLE HiiHandle, STRING_REF StrRef, CHAR16 *sFormat, ...);

/**
    It will return the String length of the array.

    @param String It holds the base address of the array.

    @return UINTN Returns length of the char array.

    @note  Function variable description

            String - It holds the base address of the array.

            Length - Holds the String length.

**/

UINTN
EfiStrLen (
  IN CHAR16   *String )
{
    INTN Length = 0;

    while(*String++) Length++;

    return Length;
}

/**
    Validate the IP address, and also convert the string
    IP to decimal value.If the IP is invalid then 0 Ip
    is entered.

    The Source string is parsed from right to left with
    following rules
    1) no characters other than numeral and dot is allowed
    2) the first right most letter should not be a dot
    3) no consecutive dot allowed
    4) values greater than 255 not allowed
    5) not more than four parts allowed

    @param Destination - contains validated IP address in decimal
             system
    @param Source - string IP to be validated

    @retval EFI_STATUS

    @note  Function variable description

              Index           - Counter for for loop and store the length
                                of the source array initially.
              LookingIp       - IP address is taken into four parts, one
                                part by one will be extracted from the
                                string and saved to Destination variable.
                                LookingIp variable contains which IP part
                                currently we are extracting.
              SumIp           - Contains sum of the digit of an IP address
                                multiplied by its base power. SumIp=
                               (unit digit * 1) + (Tenth digit * 10) +
                               (hundredth digit * 100)
              IpDigit         - base power of the digit we are extracting
              DigitCount      - digit number - 1 we are extracting in a 
                                IP part
              IsIpValid       - flag to set if an invalid IP is entered
                                and break the loop.
              GotTheFirstDigit- flag is set when the first decimal value is
                                found in the string,So if a dot is 
                                encountered first, immediately the loop can
                                be terminated.
              TotalSumIp      - sum of all 4 SumIp part of an IP address.
                                this variable is used to avoid 
                                000.000.000.000 IP case in the
                                BmcLanConfiguration.intial value is set to
                                zero.if sum of all 4 SumIp part of an IP
                                address is zero then that is invalid
                                input.

**/

EFI_STATUS
ValidateIpAddress(
  OUT  UINT8      Destination[],
  IN  CHAR16      Source[] )
{
    UINTN      Index;
    UINT8      LookingIp;
    UINT8      SumIp;
    UINT8      IpDigit;
    UINT8      DigitCount;
    UINT8      IsIpValid;
    UINT8      GotTheFirstDigit;
    UINT16     TotalSumIp;

    LookingIp = 3;
    SumIp = 0;
    IpDigit = 1;
    DigitCount = 0;
    IsIpValid = 1;
    GotTheFirstDigit = 0;
    TotalSumIp = 0 ;

    Index = (INTN) EfiStrLen(Source);

    //
    //Assigning index = 15 because it is not returning 15 as strlen if ip is
    //like xxx.xxx.xxx.xxx this.
    //

    if ( Index > 15) {
        Index = 15;
    }

    do{
      Index--;
        if (Source[Index] <= 57 && Source[Index] >= 48) {
          GotTheFirstDigit = 1;
          if (DigitCount > 2) {
            IsIpValid = 0;
            break;
          }
          if (DigitCount == 2) {
            if (Source[Index] - 48 >2) {
              IsIpValid = 0;
              break;
            }
            if (Source[Index] - 48 == 2 && ((Source[Index + 1] - 48 > 5) ||
            (Source[Index + 1] - 48 == 5 && Source[Index + 2] - 48 > 5))) {
              IsIpValid = 0;
              break;
            } 
          }
          SumIp = SumIp + ((UINT8)Source[Index] - 48) * IpDigit;
          IpDigit = IpDigit * 10;
          DigitCount = DigitCount + 1;
        } else if (Source[Index] == 46) {
          if (GotTheFirstDigit == 0 || Source[Index + 1] == 46) {
            IsIpValid = 0;
            break;
          }
          Destination[LookingIp] = SumIp;
          TotalSumIp = TotalSumIp + SumIp;
          LookingIp--;
          if(LookingIp < 0) {
            IsIpValid = 0;
            break;
          }
          SumIp = 0;
          IpDigit = 1;
          DigitCount = 0;
        } else if (Source[Index] != 0 ||
            (Source[Index] == 0 && GotTheFirstDigit == 1)) {
            IsIpValid = 0;
            break;
          }
    }while(Index!=0);
    if (LookingIp == 0) {
      Destination[LookingIp] = SumIp;
      TotalSumIp = TotalSumIp + SumIp;
    }
    if(LookingIp !=0 || IsIpValid == 0 || TotalSumIp == 0) {
      Destination[0] = 0;
      Destination[1] = 0;
      Destination[2] = 0;
      Destination[3] = 0;
      return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
}

/**
    Validate the MAC address, and also convert the string MAC to
    decimal value.If the MAC is invalid then 0 MAC is entered.

    The Source string is parsed from right to left with following
    rules

    1) no characters other than numeral, alphabets a-f, A-F and
    - is allowed
    2) the first right most letter should not be a -
    3) no consecutive - allowed
    4) not more than six parts allowed

    @param Destination - contains validated MAC address in decimal 
        system
    @param  Source      - string MAC to be validated

    @return EFI_STATUS

    @note  Function variable description
  
              Index        - counter for loop.
              LookingMac   - MAC address is taken into six parts, one part
                             by one will be extracted from the string and
                             saved to Destination variable.
                             LookingMac variable contains which MAC part
                             currently we are extracting.
              SumMac       - contains sum of the digit of an MAC address
                             multiplied by its base power. SumMac=
                             (unit digit * 1) + (Tenth digit * 16)
              MacDigit     - base power of the digit we are extracting
              DigitCount   - digit number - 1 we are extracting in a MAC
                             part         
              IsMacValid   - flag to set if an invalid MAC is entered and
                             break the loop.
              GotTheFirstDigit - flag is set when the first hex value is
                                 found in the string,So if a - is 
                                 encountered first,immediately the loop
                                 can be terminated. 
              TmpValue     - Used to convert all small letters to capital
                             letters
              TotalSumMac  - sum of all 6 SumMac part of an MAC address.
                             this variable is used to avoid 
                             00-00-00-00-00-00 MAC case in the
                             BmcLanConfiguration.intial value is set to 
                             zero.if sum of all 6 SumMac is zero then 
                             that is invalid MAC.

**/

EFI_STATUS
ValidateMacAddress(
  OUT UINT8       Destination[],
  IN  CHAR16      Source[] )
{
    UINT8      Index = 16;
    UINT8      LookingMac;
    UINT8      SumMac;
    UINT8      MacDigit;
    UINT8      DigitCount;
    UINT8      IsMacValid;
    UINT8      GotTheFirstDigit;
    UINT8      TmpValue;
    UINT16     TotalSumMac; 

    LookingMac = 5;
    SumMac = 0;
    MacDigit = 1;
    DigitCount = 0;
    IsMacValid = 1;
    GotTheFirstDigit = 0;
    TotalSumMac = 0;

    do{
      Index--;
      if ( (Source[Index] <= 57 && Source[Index] >= 48) ||
        (Source[Index] >= 65 && Source[Index] <= 70) ||
        (Source[Index] >= 97 && Source[Index] <= 102) ) {
         GotTheFirstDigit = 1;
         if (DigitCount > 1) {
           IsMacValid = 0;
           break;
         }
         TmpValue = (UINT8)Source[Index];
         if (TmpValue >= 97 && TmpValue <= 102) {
           TmpValue = TmpValue - 32;
         }
         TmpValue = TmpValue - 48;
         if (TmpValue > 9) {
           TmpValue = TmpValue - 7;
         }
         SumMac = SumMac + (TmpValue * MacDigit);
         MacDigit = MacDigit * 16;
         DigitCount = DigitCount + 1;
        } else if (Source[Index] == 45) {
          if (GotTheFirstDigit == 0 || Source[Index + 1] == 45) {
            IsMacValid = 0;
            break;
          }
          Destination[LookingMac] = SumMac;
          TotalSumMac = TotalSumMac + SumMac;
          LookingMac--;
          if(LookingMac < 0) {
            IsMacValid = 0;
            break;
          }
          SumMac = 0;
          MacDigit = 1;
          DigitCount = 0;
        } else {
         IsMacValid = 0;
         break;
       }
    }while(Index!=0);
    if (LookingMac == 0) {
      Destination[LookingMac] = SumMac;
      TotalSumMac = TotalSumMac + SumMac;
    } 
    if (LookingMac !=0 || IsMacValid == 0 || TotalSumMac == 0) {
      Destination[0] = 0;
      Destination[1] = 0;
      Destination[2] = 0;
      Destination[3] = 0;
      Destination[4] = 0;
      Destination[5] = 0;
      return EFI_INVALID_PARAMETER;
    }

    return EFI_SUCCESS;
}

/**
    Set IP address to 0.0.0.0 to memory pointed by ZeroIp

    @param ZeroIp - Pointer to Ip address in BIOS setup variable

    @retval VOID

**/

VOID
SetZeroIp(
  OUT  CHAR16      ZeroIp[] )
{

    ZeroIp[0] = 48;
    ZeroIp[1] = 46;
    ZeroIp[2] = 48;
    ZeroIp[3] = 46;
    ZeroIp[4] = 48;
    ZeroIp[5] = 46;
    ZeroIp[6] = 48;
    ZeroIp[7] = 0;
    ZeroIp[8] = 0;
    ZeroIp[9] = 0;
    ZeroIp[10] = 0;
    ZeroIp[11] = 0;
    ZeroIp[12] = 0;
    ZeroIp[13] = 0;
    ZeroIp[14] = 0;

}

/**
    Set MAC address to 00-00-00-00-00-00 to memory pointed by
    ZeroMac

    @param ZeroMac - Pointer to MAC address in BIOS setup variable

    @retval VOID

**/

VOID
SetZeroMac(
  OUT  CHAR16      ZeroMac[] )
{

    ZeroMac[0] = 48;
    ZeroMac[1] = 48;
    ZeroMac[2] = 45;
    ZeroMac[3] = 48;
    ZeroMac[4] = 48;
    ZeroMac[5] = 45;
    ZeroMac[6] = 48;
    ZeroMac[7] = 48;
    ZeroMac[8] = 45;
    ZeroMac[9] = 48;
    ZeroMac[10] = 48;
    ZeroMac[11] = 45;
    ZeroMac[12] = 48;
    ZeroMac[13] = 48;
    ZeroMac[14] = 45;
    ZeroMac[15] = 48;
    ZeroMac[16] = 48;

}
/**
    Checks for Set-In Progress Bit and Wait to get it Clear

    @param  LanChannelNumber - Channel Number of LAN

    @return EFI_STATUS
    @retval  EFI_SUCCESS - Set-In Progress Bit is Cleared
    @retval  EFI_TIMEOUT - Specified Time out over
    
**/

EFI_STATUS
IpmiWaitSetInProgressClear (
   UINT8 LanChannelNumber ) 
{
    EFI_STATUS                      Status;
    UINT8                           ResponseSize;
    UINT8                           Retries = 10;
    EFI_IPMI_GET_LAN_CONFIG_PRAM    GetLanParamCmd;
    EFI_GET_LAN_CONFIG_PARAM0          GetSetInProgResponse;

    GetLanParamCmd.LanChannel.ChannelNumber = LanChannelNumber;
    GetLanParamCmd.LanChannel.Reserved = 0;
    GetLanParamCmd.LanChannel.GetParam = 0; 
    GetLanParamCmd.ParameterSelector = 0;       
    GetLanParamCmd.SetSelector = 0;
    GetLanParamCmd.BlockSelector = 0;

    //
    // Wait for Set-In progress bit to clear
    //
    do {
        ResponseSize = sizeof (EFI_GET_LAN_CONFIG_PARAM0);
        Status = gIpmiTransport->SendIpmiCommand (
                     gIpmiTransport,
                     EFI_SM_NETFN_TRANSPORT,
                     BMC_LUN,
                     EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                     (UINT8*) &GetLanParamCmd,
                     sizeof (EFI_IPMI_GET_LAN_CONFIG_PRAM),
                     (UINT8*) &GetSetInProgResponse,
                     &ResponseSize );

        if ( EFI_ERROR (Status) ) {
            return Status;
        }

        if (GetSetInProgResponse.Param0.SetInProgress == 0) {
            break;
        }
        gBS->Stall (IPMI_STALL);
    } while (Retries-- > 0);

    if (Retries == 0) {
        return EFI_TIMEOUT;
    }
    return EFI_SUCCESS;
}


/**
    Set the LAN configuration values from BIOS setup to BMC

    @param LanChannelNumber - LAN channel to use

    @param BmcLanIpSetupValues -  Structure containing IP and MAC to be
                                  entered in BMC

    @return EFI_STATUS

**/

EFI_STATUS 
SetStaticBmcNetworkParameters(
  IN  UINT8       LanChannelNumber,
  IN  BMC_IP_BIOS_SETTINGS    BmcLanIpSetupValues )
{

    EFI_STATUS               Status;
    UINT8                    CommandData[8];
    UINT8                    ResponseData[1];
    UINT8                    ResponseDataSize;

    CommandData[0] = LanChannelNumber;

    //
    //Set station IP address
    //
    CommandData[1] = EfiIpmiLanIpAddress;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.StationIp, sizeof (BmcLanIpSetupValues.StationIp));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanIpAddress: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    //
    //Set Subnet Mask
    //
    CommandData[1] = EfiIpmiLanSubnetMask;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.Subnet, sizeof (BmcLanIpSetupValues.Subnet));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanSubnetMask: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    //
    //Set Default Gateway Ip Address
    //
    CommandData[1] = EfiIpmiLanDefaultGateway;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.RouterIp, sizeof(BmcLanIpSetupValues.RouterIp));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanDefaultGateway: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    //
    //Set Default Gateway MAC Address
    //
    CommandData[1] = EfiIpmiLanDefaultGatewayMac;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.RouterMac, sizeof (BmcLanIpSetupValues.RouterMac));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                8,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanDefaultGatewayMac: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    return EFI_SUCCESS;
}

EFI_STATUS 
SetStaticBmcLanIpParameters(
  IN  UINT8       LanChannelNumber,
  IN  BMC_IP_BIOS_SETTINGS    BmcLanIpSetupValues )
{

    EFI_STATUS               Status = EFI_SUCCESS;
    UINT8                    CommandData[8];
    UINT8                    ResponseData[1];
    UINT8                    ResponseDataSize;

    CommandData[0] = LanChannelNumber;

    //
    //Set station IP address
    //
    CommandData[1] = EfiIpmiLanIpAddress;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.StationIp, sizeof (BmcLanIpSetupValues.StationIp));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanIpAddress: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    return Status;
}

EFI_STATUS 
SetStaticBmcSubMaskParameters(
  IN  UINT8       LanChannelNumber,
  IN  BMC_IP_BIOS_SETTINGS    BmcLanIpSetupValues )
{

    EFI_STATUS               Status = EFI_SUCCESS;
    UINT8                    CommandData[8];
    UINT8                    ResponseData[1];
    UINT8                    ResponseDataSize;

    CommandData[0] = LanChannelNumber;

    //
    //Set Subnet Mask
    //
    CommandData[1] = EfiIpmiLanSubnetMask;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.Subnet, sizeof (BmcLanIpSetupValues.Subnet));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanSubnetMask: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    return Status;
}

EFI_STATUS 
SetStaticBmcGatewayIpParameters(
  IN  UINT8       LanChannelNumber,
  IN  BMC_IP_BIOS_SETTINGS    BmcLanIpSetupValues )
{

    EFI_STATUS               Status=EFI_SUCCESS;
    UINT8                    CommandData[8];
    UINT8                    ResponseData[1];
    UINT8                    ResponseDataSize;

    CommandData[0] = LanChannelNumber;

    //
    //Set Default Gateway Ip Address
    //
    CommandData[1] = EfiIpmiLanDefaultGateway;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.RouterIp, sizeof(BmcLanIpSetupValues.RouterIp));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                6,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanDefaultGateway: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    return Status;
}

EFI_STATUS 
SetStaticBmcGatewayMacParameters(
  IN  UINT8       LanChannelNumber,
  IN  BMC_IP_BIOS_SETTINGS    BmcLanIpSetupValues )
{

    EFI_STATUS               Status=EFI_SUCCESS;
    UINT8                    CommandData[8];
    UINT8                    ResponseData[1];
    UINT8                    ResponseDataSize;

    CommandData[0] = LanChannelNumber;

    //
    //Set Default Gateway MAC Address
    //
    CommandData[1] = EfiIpmiLanDefaultGatewayMac;
    CopyMem (&CommandData[2], BmcLanIpSetupValues.RouterMac, sizeof (BmcLanIpSetupValues.RouterMac));
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                8,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: EfiIpmiLanDefaultGatewayMac: LanChannelNumber: %x Status: %r\n", LanChannelNumber, Status));

    return Status;
}

/**
    Set the LAN IP address source values from BIOS setup to BMC

    @param LanChannelNumber - LAN channel to use

    @param IpAddessSource - Value of address source

    @return EFI_STATUS

**/

EFI_STATUS
SetIpAddressSource (
  IN  UINT8      LanChannelNumber,
  IN  UINT8      IpAddessSource )
{
    EFI_STATUS                    Status=EFI_SUCCESS;
    UINT8                         CommandData[4];
    UINT8                         ResponseData[1];
    UINT8                         ResponseDataSize;
    //
    //Set IP address source
    //
    CommandData[0] = LanChannelNumber;
    CommandData[1] = EfiIpmiLanIpAddressSource;
    CommandData[2] = IpAddessSource;
    ResponseDataSize = 1;

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (LanChannelNumber);
    Status = gIpmiTransport->SendIpmiCommand (
                gIpmiTransport,
                EFI_SM_NETFN_TRANSPORT,
                BMC_LUN,
                EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS,
                CommandData,
                3,
                ResponseData,
                &ResponseDataSize );
    SERVER_IPMI_DEBUG ((EFI_D_LOAD, " EFI_TRANSPORT_SET_LAN_CONFIG_PARAMETERS: IpAddessSource:%x LanChannelNumber: %x Status: %r\n", IpAddessSource, LanChannelNumber, Status));

    return Status;
}

EFI_STATUS
BmcSetEthCallbackFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key )
{
    EFI_STATUS                  Status = EFI_SUCCESS;
    UINT32                      Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS;
    BmcIpmiConfigData           BmcConfigData ;
    UINTN                       Size;

    SERVER_IPMI_TRACE ((-1, "\nEntered %a with Key: %x \n", __FUNCTION__,Key));
    //
    // Check for the key and Return if Key value does not match
    //
    if ( (Key != BMC_NETWORK_ETH0_KEY) && \
         (Key != BMC_NETWORK_ETH1_KEY) ) {
        SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }
    //
    // Get the call back parameters and verify the action
    //
    CALLBACK_PARAMETERS *CallbackParameters = GetCallbackParameters();
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING ) {
        SERVER_IPMI_TRACE ((-1,"\n CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    Size = sizeof (BmcIpmiConfigData); 
    ZeroMem((VOID *)&BmcConfigData , Size);
    Status = gRT->GetVariable (
                IpmiVarName,
                &gIpmiConfigFormSetGuid,
                &Attributes,
                &Size,
                &BmcConfigData);

    switch( Key )
    {
        case BMC_NETWORK_ETH0_KEY:
          BmcConfigData.BmcEth = 0;
          BmcConfigData.BmcIpPara[BmcConfigData.BmcEth].BmcEthValue = BMC_LAN1_CHANNEL_NUMBER;
          break;
        case BMC_NETWORK_ETH1_KEY:
          BmcConfigData.BmcEth = 1;
          BmcConfigData.BmcIpPara[BmcConfigData.BmcEth].BmcEthValue = BMC_LAN2_CHANNEL_NUMBER;
          break;
        default:
          SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
            return EFI_UNSUPPORTED;
    }

    Status = gRT->SetVariable (
              IpmiVarName,
              &gIpmiConfigFormSetGuid,
              Attributes,
              Size,
              &BmcConfigData );

    return EFI_SUCCESS;
}

/**
    This function validate the given ip and MAC addresses and
    display error messages if given input is invalid data

    @param HiiHandle A handle that was previously registered in the
                     HII Database.
    @param Class    Formset Class of the Form Callback Protocol passed in
    @param SubClass Formset sub Class of the Form Callback Protocol passed in
    @param Key Formset Key of the Form Callback Protocol passed in

    @return EFI_STATUS Return Status

**/
EFI_STATUS
BmcLanConfigCallbackFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key )
{
    BmcIpmiConfigData                  *BmcConfigData = NULL;
    BMC_IP_BIOS_SETTINGS                BmcLanIpSetupValues;
    CHAR16                              OutputString[STRING_BUFFER_LENGTH];
    EFI_STATUS                          Status = EFI_SUCCESS;
    UINTN                               SelectionBufferSize;
    EFI_STATUS                          OutputStrStatus = EFI_SUCCESS;
    UINTN                               BufferSize = STRING_BUFFER_LENGTH;
    UINT32                              Attributes = EFI_VARIABLE_NON_VOLATILE |EFI_VARIABLE_BOOTSERVICE_ACCESS;

    SERVER_IPMI_TRACE ((-1, "\nEntered BmcLanConfigCallbackFunction with Key: %x \n", Key));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( (Key != BMC_NETWORK_STATION_IP_KEY) && \
         (Key != BMC_NETWORK_SUBNET_KEY) && \
         (Key != BMC_NETWORK_ROUTER_IP_KEY) && \
         (Key != BMC_NETWORK_ROUTER_MAC_KEY) && \
         (Key != BMC_NETWORK_ADDR_MODE) ) {
        SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }

    //
    // Allocate memory for BmcIpmiConfigData
    //
    SelectionBufferSize = sizeof (BmcIpmiConfigData);
    Status = gBS->AllocatePool (
                EfiBootServicesData,
                SelectionBufferSize,
                (VOID **)&BmcConfigData );
    if (EFI_ERROR (Status)) {
        return Status;
    }
    ZeroMem((VOID *)BmcConfigData , SelectionBufferSize);
    //
    // Get Variable data
    //
    Status = gRT->GetVariable (
                IpmiVarName,
                &gIpmiConfigFormSetGuid,
                &Attributes,
                &SelectionBufferSize,
                BmcConfigData);
    SERVER_IPMI_TRACE ((-1, "Status of GetVariable %r, BmcEth=%d,BmcEthValue=%d \n",Status,BmcConfigData->BmcEth ,BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcEthValue ));
    ASSERT_EFI_ERROR (Status);
    //
    // Get Browser DATA
    //
    Status = HiiLibGetBrowserData (
                &SelectionBufferSize,
                BmcConfigData,
                &gIpmiConfigFormSetGuid,
                IpmiVarName);
    SERVER_IPMI_TRACE ((-1,"\nStatus of HiiLibGetBrowserData() = %r,BmcEth=%d\n",Status, BmcConfigData->BmcEth));
    ASSERT_EFI_ERROR (Status);

    //
    // Get the call back parameters and verify the action
    //
    CALLBACK_PARAMETERS *CallbackParameters = GetCallbackParameters();
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED ) {
        gBS->FreePool(BmcConfigData);
        SERVER_IPMI_TRACE ((-1,"\n CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }
    //
    // Validate Ip/MAC, for error case, display error message
    //
    switch( Key )
    {
        case BMC_NETWORK_ADDR_MODE:
          Status = SetIpAddressSource (1,BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcLanMode);
          break;
        case BMC_NETWORK_STATION_IP_KEY :
            Status = ValidateIpAddress (
                        BmcLanIpSetupValues.StationIp,
                        BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].StationIp );
            if ( EFI_ERROR (Status) ) {
                SetZeroIp (BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].StationIp);
                OutputStrStatus = IpmiHiiGetString (HiiHandle, STRING_TOKEN(STR_INVALID_STATION_IP), OutputString, &BufferSize, NULL);
            }
            if(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcLanMode== StaticAddress)
              SetStaticBmcLanIpParameters(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcEthValue,BmcLanIpSetupValues);
            break;

        case BMC_NETWORK_SUBNET_KEY :
            Status = ValidateIpAddress (
                           BmcLanIpSetupValues.Subnet,
                           BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].Subnet );
            if ( EFI_ERROR (Status) ) {
                SetZeroIp(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].Subnet);
                OutputStrStatus = IpmiHiiGetString (HiiHandle, STRING_TOKEN(STR_INVALID_SUBNETMASK), OutputString, &BufferSize, NULL);
            }
            if(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcLanMode== StaticAddress)
              SetStaticBmcSubMaskParameters(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcEthValue,BmcLanIpSetupValues);
            break;

        case BMC_NETWORK_ROUTER_IP_KEY :
            Status = ValidateIpAddress (
                        BmcLanIpSetupValues.RouterIp,
                        BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].RouterIp );
            if ( EFI_ERROR (Status) ) {
                SetZeroIp(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].RouterIp);
                OutputStrStatus = IpmiHiiGetString (HiiHandle, STRING_TOKEN(STR_INVALID_ROUTER_IP), OutputString, &BufferSize, NULL);
            }
            if(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcLanMode== StaticAddress)
              SetStaticBmcGatewayIpParameters(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcEthValue,BmcLanIpSetupValues);
            break;

        case BMC_NETWORK_ROUTER_MAC_KEY :
            Status = ValidateMacAddress (
                        BmcLanIpSetupValues.RouterMac,
                        BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].RouterMac );
            if ( EFI_ERROR (Status) ) {
                SetZeroMac(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].RouterMac);
                OutputStrStatus = IpmiHiiGetString (HiiHandle, STRING_TOKEN(STR_INVALID_ROUTER_MAC), OutputString, &BufferSize, NULL);
            }
            if(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcLanMode== StaticAddress)
              SetStaticBmcGatewayMacParameters(BmcConfigData->BmcIpPara[BmcConfigData->BmcEth].BmcEthValue,BmcLanIpSetupValues);
            break;
        default :
            gBS->FreePool(BmcConfigData);
            SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
            return EFI_UNSUPPORTED;
    }
    if ( EFI_ERROR (Status) ) {
      DisplayErrorMessagePopUp(HiiHandle, STRING_TOKEN(STR_BMCLAN_ERROR_INFO));
      //
      // Set Browser DATA
      //
      Status = HiiLibSetBrowserData (
                  SelectionBufferSize,
                  BmcConfigData,
                  &gIpmiConfigFormSetGuid,
                  IpmiVarName );
      SERVER_IPMI_TRACE ((-1,"\nStatus of HiiLibSetBrowserData() = %r\n",Status));
    }

    gBS->FreePool(BmcConfigData);
    SERVER_IPMI_TRACE ((-1, "\nExiting...... BmcLanConfigCallbackFunction\n"));

    return EFI_SUCCESS;
}

EFI_STATUS
BmcEthParamCallBakcFunction(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key )

{

    EFI_STATUS     Status;
    UINT8          CommandDataSize;
    EFI_IPMI_GET_LAN_CONFIG_PRAM  GetLanParamCmd;
    EFI_GET_LAN_IP_ADDRESS_SRC    LanAddressSource;
    EFI_GET_LAN_MAC_ADDRESS       LanMacAddress;
    EFI_GET_LAN_IP_ADDRESS        LanIPAddress;
    EFI_GET_LAN_SUBNET_MASK       LanSubnetMask;
    UINT8          ResponseDataSize;
    UINT16         IPSource[5] = { STRING_TOKEN(STR_UNSPECIFIED),
                                   STRING_TOKEN(STR_IPSOURCE_STATIC_ADDRESS),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BMC_DHCP),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BIOS_DHCP),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BMC_NON_DHCP)
                                  };
    UINT8                       IPSourceIndex;
    CHAR16                      CharPtr[STRING_BUFFER_LENGTH];
    EFI_STATUS                  CharPtrStatus = EFI_SUCCESS;
    UINTN                       BufferLength = STRING_BUFFER_LENGTH;
    STRING_REF                  StrReafCurrMode ;
    STRING_REF                  StrReafIp;
    STRING_REF                  StrReafMask;
    STRING_REF                  StrReafMac;
    STRING_REF                  StrReafGatewayIp;
    STRING_REF                  StrReafGatewayMac;
    UINT32                      Attributes = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_RUNTIME_ACCESS |EFI_VARIABLE_BOOTSERVICE_ACCESS;
    BmcIpmiConfigData           BmcConfigData ;
    UINTN                       Size;

    SERVER_IPMI_TRACE ((-1, " %a \n", __FUNCTION__));

    //
    // Check for the key and Return if Key value does not match
    //
    if ( (Key != BMC_NETWORK_ETH0_KEY) && \
         (Key != BMC_NETWORK_ETH1_KEY) ) {
        SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong Key Value. \
                                              Returning EFI_UNSUPPORTED\n"));
        return EFI_UNSUPPORTED;
    }
    //
    // Get the call back parameters and verify the action
    //
    CALLBACK_PARAMETERS *CallbackParameters = GetCallbackParameters();
    if ( CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGING ) {
        SERVER_IPMI_TRACE ((-1,"\n CallbackParameters->Action != EFI_BROWSER_ACTION_CHANGED... so return EFI_SUCCESS\n"));
        return  EFI_SUCCESS;
    }

    Size = sizeof (BmcIpmiConfigData); 
    ZeroMem((VOID *)&BmcConfigData , Size);
    Status = gRT->GetVariable (
                IpmiVarName,
                &gIpmiConfigFormSetGuid,
                &Attributes,
                &Size,
                &BmcConfigData);

    SERVER_IPMI_TRACE ((-1, " BmcEth=%d,BmcEthValue=%d \n",BmcConfigData.BmcEth ,BmcConfigData.BmcIpPara[BmcConfigData.BmcEth].BmcEthValue ));
    switch( BmcConfigData.BmcEth )
    {
        case 0:
          StrReafCurrMode = STRING_TOKEN(STR_CURR_LANCAS1_VAL);
          StrReafIp = STRING_TOKEN(STR_STATION_IP1_VAL);
          StrReafMask = STRING_TOKEN(STR_SUBNET_MASK1_VAL);
          StrReafMac = STRING_TOKEN(STR_STATION_MAC1_VAL);
          StrReafGatewayIp = STRING_TOKEN(STR_ROUTER_IP1_VAL);
          StrReafGatewayMac = STRING_TOKEN(STR_ROUTER_MAC1_VAL);
          //
          // Common for all LAN 1 Ethernet
          //
          GetLanParamCmd.LanChannel.ChannelNumber = BmcConfigData.BmcIpPara[BmcConfigData.BmcEth].BmcEthValue;
          GetLanParamCmd.LanChannel.Reserved = 0;
          GetLanParamCmd.LanChannel.GetParam = 0; 
          GetLanParamCmd.SetSelector = 0;
          GetLanParamCmd.BlockSelector = 0;
          CommandDataSize = sizeof (EFI_IPMI_GET_LAN_CONFIG_PRAM);

          break;
        case 1:
          StrReafCurrMode = STRING_TOKEN(STR_CURR_LANCAS2_VAL);
          StrReafIp = STRING_TOKEN(STR_STATION_IP2_VAL);
          StrReafMask = STRING_TOKEN(STR_SUBNET_MASK2_VAL);
          StrReafMac = STRING_TOKEN(STR_STATION_MAC2_VAL);
          StrReafGatewayIp = STRING_TOKEN(STR_ROUTER_IP2_VAL);
          StrReafGatewayMac = STRING_TOKEN(STR_ROUTER_MAC2_VAL);
          //
          // Common for all LAN 2 Ethernet
          //
          GetLanParamCmd.LanChannel.ChannelNumber = BmcConfigData.BmcIpPara[BmcConfigData.BmcEth].BmcEthValue;
          GetLanParamCmd.LanChannel.Reserved = 0;
          GetLanParamCmd.LanChannel.GetParam = 0; 
          GetLanParamCmd.SetSelector = 0;
          GetLanParamCmd.BlockSelector = 0;
          CommandDataSize = sizeof (EFI_IPMI_GET_LAN_CONFIG_PRAM);

          break;
        default:
          SERVER_IPMI_TRACE ((-1,"\nCallback function is called with Wrong BmcEth Value %d. \
                                              Returning EFI_UNSUPPORTED\n",BmcConfigData.BmcEth));
          return EFI_UNSUPPORTED;
    }

//     Status = gBS->LocateProtocol (
//                 &gEfiDxeIpmiTransportProtocolGuid,
//                 NULL,
//                 (VOID **)&gIpmiTransport );
//     SERVER_IPMI_TRACE ((-1, " gEfiDxeIpmiTransportProtocolGuid Status: %r \n", Status));
//     if ( EFI_ERROR (Status) ) {
//         return ;
//     }

    //
    //Get IP address Source for Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddressSource;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS_SRC);

    //
    // Wait until Set In Progress field is cleared          
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if (!EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanAddressSource,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            if( LanAddressSource.AddressSrc > 4) {
                IPSourceIndex = 0;
            } else {
                IPSourceIndex = LanAddressSource.AddressSrc ;
            }

            CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[IPSourceIndex], CharPtr, &BufferLength, NULL);
            if (!EFI_ERROR(CharPtrStatus)) {
                InitString (
                    HiiHandle,
                    StrReafCurrMode ,
                    CharPtr );
            }
        } 
    }
    if ( EFI_ERROR (Status)) {
        CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[0], CharPtr, &BufferLength, NULL);
        if (!EFI_ERROR(CharPtrStatus)) {
            InitString (
                HiiHandle,
                StrReafCurrMode ,
                CharPtr);
        }
    }

    //
    //Get station IP address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                StrReafIp ,
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            StrReafIp ,
            L"%d.%d.%d.%d",
            0,0,0,0);
        }

    //
    //Get Subnet MASK of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanSubnetMask;
    ResponseDataSize = sizeof (EFI_GET_LAN_SUBNET_MASK);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanSubnetMask,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                StrReafMask ,
                L"%d.%d.%d.%d",
                LanSubnetMask.IpAddress[0], LanSubnetMask.IpAddress[1], LanSubnetMask.IpAddress[2], LanSubnetMask.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            StrReafMask ,
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get MAC address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanMacAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                StrReafMac,
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            StrReafMac,
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }

    //
    //Get Router Gateway IP Address of Channel-1
    //

    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGateway;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                StrReafGatewayIp,
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        }
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            StrReafGatewayIp,
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get Router MAC address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGatewayMac;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (GetLanParamCmd.LanChannel.ChannelNumber);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );
        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                StrReafGatewayMac,
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            StrReafGatewayMac,
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }

    SERVER_IPMI_TRACE ((-1, "BmcLanParamDisplayInSetup: Returning..........\n"));
    return EFI_SUCCESS;
}


/**
    Reads the LAN parameters from BMC and updates in Setup

    @param  HiiHandle - Handle to HII database
    @param  Class - Indicates the setup class

    @retval VOID

**/

VOID
BmcLanParamDisplayInSetup(
  IN  EFI_HII_HANDLE     HiiHandle,
  IN  UINT16             Class,
  IN  UINT16             SubClass,
  IN  UINT16             Key )

{

    EFI_STATUS     Status;
    UINT8          CommandDataSize;
    EFI_IPMI_GET_LAN_CONFIG_PRAM  GetLanParamCmd;
    EFI_GET_LAN_IP_ADDRESS_SRC    LanAddressSource;
    EFI_GET_LAN_MAC_ADDRESS       LanMacAddress;
    EFI_GET_LAN_IP_ADDRESS        LanIPAddress;
    EFI_GET_LAN_SUBNET_MASK       LanSubnetMask;
    UINT8          ResponseDataSize;
    UINT16         IPSource[5] = { STRING_TOKEN(STR_UNSPECIFIED),
                                   STRING_TOKEN(STR_IPSOURCE_STATIC_ADDRESS),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BMC_DHCP),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BIOS_DHCP),
                                   STRING_TOKEN(STR_IPSOURCE_DYNAMIC_ADDRESS_BMC_NON_DHCP)
                                  };
    UINT8          IPSourceIndex;
    CHAR16         CharPtr[STRING_BUFFER_LENGTH];
    EFI_STATUS     CharPtrStatus = EFI_SUCCESS;
    UINTN          BufferLength = STRING_BUFFER_LENGTH;

    SERVER_IPMI_TRACE ((-1, "BmcLanParamDisplayInSetup: Class ID:  %x\n", Class));

    //
    // Continue only for Server Mgmt setup page
    //
    // if ( Class!= SERVER_MGMT_CLASS_ID ) {
    //     return;
    // }
    //
    // Locate the IPMI transport protocol
    //
    // Status = gBS->LocateProtocol (
    //             &gEfiDxeIpmiTransportProtocolGuid,
    //             NULL,
    //             (VOID **)&gIpmiTransport );
    // SERVER_IPMI_TRACE ((-1, " gEfiDxeIpmiTransportProtocolGuid Status: %r \n", Status));
    // if ( EFI_ERROR (Status) ) {
    //     return ;
    // }

    //
    // Common for all LAN 1 Channel
    //
    GetLanParamCmd.LanChannel.ChannelNumber = BMC_LAN1_CHANNEL_NUMBER;
    GetLanParamCmd.LanChannel.Reserved = 0;
    GetLanParamCmd.LanChannel.GetParam = 0; 
    GetLanParamCmd.SetSelector = 0;
    GetLanParamCmd.BlockSelector = 0;
    CommandDataSize = sizeof (EFI_IPMI_GET_LAN_CONFIG_PRAM);

    //
    //Get IP address Source for Channel-1
    //

    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddressSource;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS_SRC);

    //
    // Wait until Set In Progress field is cleared          
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if (!EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanAddressSource,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            if( LanAddressSource.AddressSrc > 4) {
                IPSourceIndex = 0;
            } else {
                IPSourceIndex = LanAddressSource.AddressSrc ;
            }

            CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[IPSourceIndex], CharPtr, &BufferLength, NULL);
            if (!EFI_ERROR(CharPtrStatus)) {
                InitString (
                    HiiHandle,
                    STRING_TOKEN(STR_CURR_LANCAS1_VAL),
                    CharPtr );
            }
        } 
    }
    if ( EFI_ERROR (Status)) {
        CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[0], CharPtr, &BufferLength, NULL);
        if (!EFI_ERROR(CharPtrStatus)) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_CURR_LANCAS1_VAL),
                CharPtr);
        }
    }

    //
    //Get station IP address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_STATION_IP1_VAL),
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_STATION_IP1_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
        }

    //
    //Get Subnet MASK of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanSubnetMask;
    ResponseDataSize = sizeof (EFI_GET_LAN_SUBNET_MASK);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanSubnetMask,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_SUBNET_MASK1_VAL),
                L"%d.%d.%d.%d",
                LanSubnetMask.IpAddress[0], LanSubnetMask.IpAddress[1], LanSubnetMask.IpAddress[2], LanSubnetMask.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_SUBNET_MASK1_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get MAC address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanMacAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_STATION_MAC1_VAL),
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_STATION_MAC1_VAL),
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }

    //
    //Get Router Gateway IP Address of Channel-1
    //

    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGateway;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_ROUTER_IP1_VAL),
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        }
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_ROUTER_IP1_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get Router MAC address of Channel-1
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGatewayMac;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN1_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );
        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_ROUTER_MAC1_VAL),
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_ROUTER_MAC1_VAL),
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }
#if BMC_LAN_COUNT == 2
    //
    // Common for all LAN 2 Channel
    //
    GetLanParamCmd.LanChannel.ChannelNumber = BMC_LAN2_CHANNEL_NUMBER;
    GetLanParamCmd.LanChannel.Reserved = 0;
    GetLanParamCmd.LanChannel.GetParam = 0; 
    GetLanParamCmd.SetSelector = 0;
    GetLanParamCmd.BlockSelector = 0;
    CommandDataSize = sizeof (EFI_IPMI_GET_LAN_CONFIG_PRAM);

    //
    //Get IP address Source for Channel-2
    //

    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddressSource;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS_SRC);
    
    //
    // Wait until Set In Progress field is cleared
    //
    BufferLength = STRING_BUFFER_LENGTH;
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanAddressSource,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {

            if( LanAddressSource.AddressSrc > 4) {
                IPSourceIndex = 0;
            } else {
                IPSourceIndex = LanAddressSource.AddressSrc;
            }
            CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[IPSourceIndex], CharPtr, &BufferLength, NULL);
            if (!EFI_ERROR(CharPtrStatus)) {
                InitString (
                    HiiHandle,
                    STRING_TOKEN(STR_CURR_LANCAS2_VAL),
                    CharPtr);
            }
        } 
    }
    if ( EFI_ERROR (Status) ) {
        CharPtrStatus = IpmiHiiGetString (HiiHandle, IPSource[0], CharPtr, &BufferLength, NULL);
        if (!EFI_ERROR(CharPtrStatus)) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_CURR_LANCAS2_VAL),
                CharPtr);
        }
    }

    //
    //Get station IP address of Channel-2
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanIpAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status)) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_STATION_IP2_VAL),
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_STATION_IP2_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get Subnet MASK of Channel-2
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanSubnetMask;
    ResponseDataSize = sizeof (EFI_GET_LAN_SUBNET_MASK);

    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanSubnetMask,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_SUBNET_MASK2_VAL),
                L"%d.%d.%d.%d",
                LanSubnetMask.IpAddress[0], LanSubnetMask.IpAddress[1], LanSubnetMask.IpAddress[2], LanSubnetMask.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_SUBNET_MASK2_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get MAC address of Channel-2
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanMacAddress;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);
    
    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_STATION_MAC2_VAL),
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_STATION_MAC2_VAL),
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }

    //
    //Get Router Gateway IP Address of Channel-2
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGateway;
    ResponseDataSize = sizeof (EFI_GET_LAN_IP_ADDRESS);
    
    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {
        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanIPAddress,
                    &ResponseDataSize );

        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_ROUTER_IP2_VAL),
                L"%d.%d.%d.%d",
                LanIPAddress.IpAddress[0], LanIPAddress.IpAddress[1], LanIPAddress.IpAddress[2], LanIPAddress.IpAddress[3]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_ROUTER_IP2_VAL),
            L"%d.%d.%d.%d",
            0,0,0,0);
    }

    //
    //Get Router MAC address of Channel-2
    //
    GetLanParamCmd.ParameterSelector = EfiIpmiLanDefaultGatewayMac;
    ResponseDataSize = sizeof (EFI_GET_LAN_MAC_ADDRESS);
    
    //
    // Wait until Set In Progress field is cleared
    //
    Status = IpmiWaitSetInProgressClear (BMC_LAN2_CHANNEL_NUMBER);
    if ( !EFI_ERROR (Status) ) {

        Status = gIpmiTransport->SendIpmiCommand (
                    gIpmiTransport,
                    EFI_SM_NETFN_TRANSPORT,
                    BMC_LUN,
                    EFI_TRANSPORT_GET_LAN_CONFIG_PARAMETERS,
                    (UINT8*)&GetLanParamCmd,
                    CommandDataSize,
                    (UINT8*)&LanMacAddress,
                    &ResponseDataSize );
        if ( !EFI_ERROR (Status) ) {
            InitString (
                HiiHandle,
                STRING_TOKEN(STR_ROUTER_MAC2_VAL),
                L"%02x-%02x-%02x-%02x-%02x-%02x",
                LanMacAddress.MacAddress[0], LanMacAddress.MacAddress[1], LanMacAddress.MacAddress[2], LanMacAddress.MacAddress[3], LanMacAddress.MacAddress[4], LanMacAddress.MacAddress[5]);
        } 
    }
    if ( EFI_ERROR (Status) ) {
        InitString (
            HiiHandle,
            STRING_TOKEN(STR_ROUTER_MAC2_VAL),
            L"%02x-%02x-%02x-%02x-%02x-%02x",
            0,0,0,0,0,0);
    }
#endif //#if BMC_LAN_COUNT == 2

    SERVER_IPMI_TRACE ((-1, "BmcLanParamDisplayInSetup: Returning..........\n"));
    return;
}


