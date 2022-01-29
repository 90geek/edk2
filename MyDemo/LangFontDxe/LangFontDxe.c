#include <Uefi.h>
#include <Protocol/HiiFont.h>
#include "Protocol/HiiDatabase.h"
#include <Library/UefiHiiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#ifndef __EFI_SHELL_PARAMETERS_PROTOCOL__
typedef struct _EFI_SHELL_PARAMETERS_PROTOCOL {
     CHAR16 **Argv;
     UINTN Argc;
} EFI_SHELL_PARAMETERS_PROTOCOL;
typedef struct {
    EFI_HANDLE                ImageHandle;
    VOID *Info;
    CHAR16                    **Argv;
    UINTN                     Argc;
}EFI_SHELL_INTERFACE;
#endif
extern EFI_WIDE_GLYPH  gSimpleFontWideGlyphData[];
extern UINT32 gSimpleFontBytes;

extern EFI_WIDE_GLYPH  gUsStdNarrowGlyphData[];
extern UINT32 mNarrowFontSize;

static EFI_GUID gSimpleFontPackageListGuid = {0xf6643673, 0x6006, 0x3738, {0x5c, 0xcd, 0xda, 0x1a, 0xeb, 0x3b, 0x26, 0xa2}};

EFI_GUID mFontPackageListGuid = {0xf5f219d3, 0x7006, 0x4648, {0xac, 0x8d, 0xd6, 0x1d, 0xfb, 0x7b, 0xc6, 0xad}};
// 
// Add 4 bytes to the header for entire length for HiiAddPackages use only. 
// 
// +--------------------------------+ <-- Package 
// | | 
// | packageLen(4 bytes) | 
// | | 
// |--------------------------------| <-- simpleFont 
// | | 
// |EFI_HII_SIMPLE_FONT_PACKAGE_HDR | 
// | | 
// |--------------------------------| <-- Location 
// | | 
// | EFI_NARROW_GLYPH       NarrowGlyphs[];
// | EFI_WIDE_GLYPH         WideGlyphs[];
// | | 
// +--------------------------------+ 


// 0 |   0       |   1       |   2       |   3       |
// 4 |            Package Length                     | Package Length
// 8 |               Length              | Type: 07  | EFI_HII_PACKAGE_HEADER         
// 12|  NumberOfNarrowGlyphs |  NumberOfWideGlyphs   |
// 16|  [EFI_NARROW_GLYPH[] | EFI_WIDE_GLYPH[]]      |
 

EFI_STATUS CreatesimpleFontPkg(EFI_WIDE_GLYPH* WideGlyph, UINT32 SizeInBytes)
{
    EFI_STATUS Status; 
    EFI_HII_SIMPLE_FONT_PACKAGE_HDR *simpleFont; 
    UINT8 *Package; 
    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
    // 
    // Locate HII Database Protocol 
    // 
    EFI_HII_DATABASE_PROTOCOL *HiiDatabase = 0; 
    Status = gBS->LocateProtocol ( 
            &gEfiHiiDatabaseProtocolGuid, 
            NULL, 
            (VOID **) &HiiDatabase 
            ); 
    if(EFI_ERROR(Status)){
      DEBUG((DEBUG_INFO,"Locate Protocol HiiDatabase Error, Status %r\n",Status));
      return Status;
    }

    UINT32    packageLen = sizeof (EFI_HII_SIMPLE_FONT_PACKAGE_HDR) + SizeInBytes + mNarrowFontSize + 4; 
    DEBUG((DEBUG_INFO,"line is %d, func %a----%d-%d-%d\n",__LINE__,__FUNCTION__,packageLen,SizeInBytes , mNarrowFontSize));
    Package = (UINT8*)AllocateZeroPool (packageLen); 
    ASSERT (Package != NULL);

    DEBUG((DEBUG_INFO,"line is %d, func %a----%p\n",__LINE__,__FUNCTION__,Package));
    WriteUnaligned32((UINT32 *) Package,packageLen); 
    simpleFont = (EFI_HII_SIMPLE_FONT_PACKAGE_HDR *) (Package + 4); 
    simpleFont->Header.Length = (UINT32) (packageLen - 4); 
    simpleFont->Header.Type = EFI_HII_PACKAGE_SIMPLE_FONTS; 
    simpleFont->NumberOfNarrowGlyphs = (UINT16) (mNarrowFontSize / sizeof (EFI_NARROW_GLYPH));
    simpleFont->NumberOfWideGlyphs = (UINT16) (SizeInBytes / sizeof (EFI_WIDE_GLYPH)); 

    UINT8 * Location = (UINT8 *) (&simpleFont->NumberOfWideGlyphs + 1); 
    DEBUG((DEBUG_INFO,"line is %d, func %a---%p\n",__LINE__,__FUNCTION__,Location));
    CopyMem (Location, gUsStdNarrowGlyphData, mNarrowFontSize); 
    Location = (UINT8 *) (&simpleFont->NumberOfWideGlyphs + 1 + mNarrowFontSize/sizeof(UINT16)); 
    DEBUG((DEBUG_INFO,"line is %d, func %a---%p\n",__LINE__,__FUNCTION__,Location));
    CopyMem (Location, WideGlyph, SizeInBytes); 


    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
    //
    // remove old simpole font package
    //

    EFI_HII_HANDLE    * simpleFontHiiHandleOld = NULL;
    simpleFontHiiHandleOld = HiiGetHiiHandles(&mFontPackageListGuid);
    if(simpleFontHiiHandleOld == NULL)
    {
        DEBUG((DEBUG_INFO,"simpleFontHiiHandleOld is NULL\n"));
        return -1;
    }
    else
    {
      DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
      HiiRemovePackages ( *simpleFontHiiHandleOld);
      DEBUG((DEBUG_INFO,"simpleFontHiiHandleOld remove ok\n"));
    }

    //
    // Add this simplified font package to a package list then install it. 
    //
    EFI_HII_HANDLE  simpleFontHiiHandle = HiiAddPackages ( 
            &gSimpleFontPackageListGuid,
            NULL, 
            Package, 
            NULL 
            ); 
    if(simpleFontHiiHandle == NULL)
    {
        DEBUG((DEBUG_INFO,"simpleFontHiiHandle is NULL\n"));
        return -1;
    }
    DEBUG((DEBUG_INFO,"line is %d, func %a----%p\n",__LINE__,__FUNCTION__,Package));
    FreePool (Package); 
    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
    return EFI_SUCCESS;
}

EFI_STATUS
UnLoadFont()
{
    EFI_STATUS                             Status = 0;
    EFI_HII_HANDLE    * handles = 0;
    UINT32 i = 0;
    handles = HiiGetHiiHandles(&gSimpleFontPackageListGuid); 
    if(handles == 0){
        return -1;
    }
    while(handles[i] != 0){
        HiiRemovePackages( handles[i]); 
        i++;
    }
    FreePool(handles);
    return Status;
}

#if 1
EFI_STATUS
EFIAPI
LoadFont(
        IN EFI_HANDLE           ImageHandle,
        IN EFI_SYSTEM_TABLE     *SystemTable
        )
{
    CHAR16 **Argv = 0;
    UINTN Argc = 0;
    EFI_SHELL_PARAMETERS_PROTOCOL *EfiShellParametersProtocol = NULL;
    EFI_SHELL_INTERFACE           *EfiShellInterface = NULL;
    EFI_STATUS                    Status;
    EFI_GUID mEfiShellParametersProtocolGuid = {0x752f3136, 0x4e16, 0x4fdc, {0xa2, 0x2a, 0xe5, 0xf4, 0x68, 0x12, 0xf4, 0xca}};
    EFI_GUID mEfiShellInterfaceGuid          = {0x47c7b223, 0xc42a, 0x11d2, {0x8e, 0x57, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
    Status = SystemTable->BootServices->OpenProtocol(ImageHandle,
            &mEfiShellParametersProtocolGuid,
            (VOID **)&EfiShellParametersProtocol,
            ImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL
            );
    if (!EFI_ERROR(Status)) {
        Argc = EfiShellParametersProtocol->Argc; 
        Argv = EfiShellParametersProtocol->Argv; 
    }else{
        Status = SystemTable->BootServices->OpenProtocol(ImageHandle,
                &mEfiShellInterfaceGuid,
                (VOID **)&EfiShellInterface,
                ImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
                );
        if (!EFI_ERROR(Status)) {
            Argc = EfiShellInterface->Argc; 
            Argv = EfiShellInterface->Argv; 
        }

    }

    if( Argc > 1){
        CHAR16* p = Argv[1];
        if(p[0] == '-' && ( p[1] == 'u' || p[1] == 'U')){
            UnLoadFont();
        }
    }else{

    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
        EFI_HII_HANDLE    * handles = 0;
        handles = HiiGetHiiHandles(&gSimpleFontPackageListGuid); 
        if(handles == 0)
        {
            Status = CreatesimpleFontPkg(gSimpleFontWideGlyphData, gSimpleFontBytes);
            DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
        }
        else
            FreePool(handles);
    }
        
    DEBUG((DEBUG_INFO,"line is %d, func %a\n",__LINE__,__FUNCTION__));
    return Status;
}
#else

EFI_STATUS
EFIAPI
LoadFont(
        IN EFI_HANDLE           ImageHandle,
        IN EFI_SYSTEM_TABLE     *SystemTable
        )
{
    UnLoadFont();
    return 0;
}
#endif
