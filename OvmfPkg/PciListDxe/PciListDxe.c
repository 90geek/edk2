/** @file

  PCI list print DXE driver.

  Copyright (c) 2026 Loongson Technology Corporation Limited (www.loongson.cn).

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Pci23.h>
#include <IndustryStandard/PciCodeId.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>

#include <Protocol/DevicePath.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PciIo.h>

#define PCI_LIST_OUTPUT_BUFFER_SIZE  512

typedef struct {
  EFI_HANDLE           Handle;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT16               Segment;
  UINT8                Bus;
  UINT8                Device;
  UINT8                Function;
  PCI_TYPE_GENERIC     Pci;
  CHAR16               *DevicePathText;
  UINT32               VendorDeviceDword;
  UINT32               Status;
  UINTN                RomImage;
  UINTN                RomSize;
} PCI_LIST_ENTRY;

STATIC BOOLEAN  mPciListPrinted = FALSE;

STATIC
VOID
EFIAPI
PciListOutput (
  IN CONST CHAR16  *Format,
  ...
  )
{
  VA_LIST  Marker;
  CHAR16   Buffer[PCI_LIST_OUTPUT_BUFFER_SIZE];

  VA_START (Marker, Format);
  UnicodeVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  Print (L"%s", Buffer);
  DEBUG ((DEBUG_INFO, "%S", Buffer));
}

STATIC
CONST CHAR8 *
PciListClassName (
  IN CONST PCI_TYPE_GENERIC  *Pci
  )
{
  switch (Pci->Device.Hdr.ClassCode[2]) {
  case PCI_CLASS_MASS_STORAGE:
    if (Pci->Device.Hdr.ClassCode[1] == 0x06) {
      return "SATA AHCI";
    }

    if (Pci->Device.Hdr.ClassCode[1] == PCI_CLASS_MASS_STORAGE_SOLID_STATE) {
      return "NVMe";
    }

    if (Pci->Device.Hdr.ClassCode[1] == PCI_CLASS_MASS_STORAGE_RAID) {
      return "RAID controller";
    }

    return "Mass storage controller";

  case PCI_CLASS_NETWORK:
    if (Pci->Device.Hdr.ClassCode[1] == PCI_CLASS_NETWORK_ETHERNET) {
      return "Ethernet controller";
    }

    return "Network controller";

  case PCI_CLASS_DISPLAY:
    if (Pci->Device.Hdr.ClassCode[1] == PCI_CLASS_DISPLAY_VGA) {
      return "VGA controller";
    }

    return "Display controller";

  case PCI_CLASS_MEDIA:
    return "Multimedia device";

  case PCI_CLASS_BRIDGE:
    switch (Pci->Device.Hdr.ClassCode[1]) {
    case PCI_CLASS_BRIDGE_HOST:
      return "Host bridge";
    case PCI_CLASS_BRIDGE_ISA:
      return "ISA bridge";
    case PCI_CLASS_BRIDGE_EISA:
      return "EISA bridge";
    case PCI_CLASS_BRIDGE_MCA:
      return "MCA bridge";
    case PCI_CLASS_BRIDGE_P2P:
      return "PCI bridge";
    case PCI_CLASS_BRIDGE_PCMCIA:
      return "PCMCIA bridge";
    case PCI_CLASS_BRIDGE_CARDBUS:
      return "CardBus bridge";
    case PCI_CLASS_BRIDGE_RACEWAY:
      return "Raceway bridge";
    default:
      return "Bridge";
    }

  case PCI_CLASS_SYSTEM_PERIPHERAL:
    if (Pci->Device.Hdr.ClassCode[1] == PCI_SUBCLASS_PERIPHERAL_OTHER) {
      return "Other system peripheral";
    }

    return "Base system peripherals";

  case PCI_CLASS_SERIAL:
    if (Pci->Device.Hdr.ClassCode[1] == PCI_CLASS_SERIAL_USB) {
      switch (Pci->Device.Hdr.ClassCode[0]) {
      case PCI_IF_UHCI:
        return "USB UHCI";
      case PCI_IF_OHCI:
        return "USB OHCI";
      case PCI_IF_EHCI:
        return "USB EHCI";
      case PCI_IF_XHCI:
        return "USB XHCI";
      default:
        return "USB controller";
      }
    }

    return "Serial bus controller";

  default:
    return "PCI device";
  }
}

STATIC
BOOLEAN
PciListIsBridgeNode (
  IN CONST PCI_LIST_ENTRY  *Entry
  )
{
  return IS_PCI_BRIDGE (&Entry->Pci.Device);
}

STATIC
INTN
PciListCompareEntry (
  IN CONST PCI_LIST_ENTRY  *Left,
  IN CONST PCI_LIST_ENTRY  *Right
  )
{
  if (Left->Segment != Right->Segment) {
    return (Left->Segment < Right->Segment) ? -1 : 1;
  }

  if (Left->Bus != Right->Bus) {
    return (Left->Bus < Right->Bus) ? -1 : 1;
  }

  if (Left->Device != Right->Device) {
    return (Left->Device < Right->Device) ? -1 : 1;
  }

  if (Left->Function != Right->Function) {
    return (Left->Function < Right->Function) ? -1 : 1;
  }

  return 0;
}

STATIC
VOID
PciListSortEntries (
  IN OUT PCI_LIST_ENTRY  *Entries,
  IN UINTN               EntryCount
  )
{
  PCI_LIST_ENTRY  Temp;
  UINTN           Index;
  UINTN           Insert;

  for (Index = 1; Index < EntryCount; Index++) {
    CopyMem (&Temp, &Entries[Index], sizeof (PCI_LIST_ENTRY));
    Insert = Index;

    while ((Insert > 0) && (PciListCompareEntry (&Temp, &Entries[Insert - 1]) < 0)) {
      CopyMem (&Entries[Insert], &Entries[Insert - 1], sizeof (PCI_LIST_ENTRY));
      Insert--;
    }

    CopyMem (&Entries[Insert], &Temp, sizeof (PCI_LIST_ENTRY));
  }
}

STATIC
BOOLEAN
PciListFindBusRange (
  IN CONST PCI_LIST_ENTRY  *Entries,
  IN UINTN                 EntryCount,
  IN UINT16                Segment,
  IN UINT8                 Bus,
  OUT UINTN                *StartIndex,
  OUT UINTN                *EndIndex
  )
{
  UINTN  Index;
  UINTN  Start;
  UINTN  End;

  Start = EntryCount;
  End   = 0;

  for (Index = 0; Index < EntryCount; Index++) {
    if (Entries[Index].Segment < Segment) {
      continue;
    }

    if (Entries[Index].Segment > Segment) {
      break;
    }

    if (Entries[Index].Bus < Bus) {
      continue;
    }

    if (Entries[Index].Bus > Bus) {
      break;
    }

    if (Start == EntryCount) {
      Start = Index;
    }

    End = Index;
  }

  if (Start == EntryCount) {
    return FALSE;
  }

  *StartIndex = Start;
  *EndIndex   = End;
  return TRUE;
}

STATIC
VOID
PciListPrintFlatEntry (
  IN CONST PCI_LIST_ENTRY  *Entry
  )
{
  PciListOutput (
    L"(%04X,%02X,%02X,%02X) %08X (%02X,%02X,%02X) %02X %04X ROM(%LX,%LX) DP:%s\n",
    Entry->Segment,
    Entry->Bus,
    Entry->Device,
    Entry->Function,
    Entry->VendorDeviceDword,
    Entry->Pci.Device.Hdr.ClassCode[2],
    Entry->Pci.Device.Hdr.ClassCode[1],
    Entry->Pci.Device.Hdr.ClassCode[0],
    Entry->Pci.Device.Hdr.RevisionID,
    Entry->Status,
    (UINT64)Entry->RomImage,
    (UINT64)Entry->RomSize,
    (Entry->DevicePathText != NULL) ? Entry->DevicePathText : L"(null)"
    );
}

STATIC
VOID
PciListPrintTreeBus (
  IN CONST PCI_LIST_ENTRY  *Entries,
  IN UINTN                 EntryCount,
  IN UINT16                Segment,
  IN UINT8                 Bus,
  IN CONST CHAR16          *Indent,
  IN BOOLEAN               IsInline
  )
{
  UINTN                 StartIndex;
  UINTN                 EndIndex;
  UINTN                 Index;
  UINTN                 ChildStart;
  UINTN                 ChildEnd;
  CHAR16                ChildIndent[128];
  CONST PCI_LIST_ENTRY  *Entry;

  if (!PciListFindBusRange (Entries, EntryCount, Segment, Bus, &StartIndex, &EndIndex)) {
    return;
  }

  for (Index = StartIndex; Index <= EndIndex; Index++) {
    Entry = &Entries[Index];

    if ((Index != StartIndex) || !IsInline) {
      PciListOutput (L"\n%s", Indent);
    }

    PciListOutput (
      ((Index == EndIndex) && (Index != StartIndex)) ? L"\\-%02X.%X" : L"+-%02X.%X",
      Entry->Device,
      Entry->Function
      );

    if (PciListIsBridgeNode (Entry)) {
      PciListOutput (
        L"-[%02X-%02X]--",
        Entry->Pci.Bridge.Bridge.SecondaryBus,
        Entry->Pci.Bridge.Bridge.SubordinateBus
        );

      if ((Entry->Pci.Bridge.Bridge.SecondaryBus != Bus) &&
          PciListFindBusRange (
            Entries,
            EntryCount,
            Segment,
            Entry->Pci.Bridge.Bridge.SecondaryBus,
            &ChildStart,
            &ChildEnd
            ))
      {
        UnicodeSPrint (ChildIndent, sizeof (ChildIndent), L"%s                ", Indent);
        PciListPrintTreeBus (
          Entries,
          EntryCount,
          Segment,
          Entry->Pci.Bridge.Bridge.SecondaryBus,
          ChildIndent,
          TRUE
          );
      }
    } else {
      PciListOutput (
        L"  [%04X:%04X] - %a",
        Entry->Pci.Device.Hdr.VendorId,
        Entry->Pci.Device.Hdr.DeviceId,
        PciListClassName (&Entry->Pci)
        );
    }
  }
}

STATIC
VOID
PciListPrintTreeSegment (
  IN CONST PCI_LIST_ENTRY  *Entries,
  IN UINTN                 EntryCount,
  IN UINT16                Segment,
  IN BOOLEAN               IsFirstSegment
  )
{
  UINTN  StartIndex;
  UINTN  EndIndex;

  if (!PciListFindBusRange (Entries, EntryCount, Segment, 0, &StartIndex, &EndIndex)) {
    return;
  }

  PciListOutput (L"%s[%04X:%02X]-", IsFirstSegment ? L"-+-" : L" +-", Segment, 0);
  PciListPrintTreeBus (Entries, EntryCount, Segment, 0, L" |           ", TRUE);
  PciListOutput (L"\n");
}

STATIC
VOID
PciListPrintAll (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_HANDLE                *HandleBuffer;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  PCI_LIST_ENTRY            *Entries;
  UINTN                     HandleCount;
  UINTN                     EntryCount;
  UINTN                     Index;
  UINTN                     SegmentCount;
  UINTN                     CurrentSegment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;
  UINT32                    DeviceDword;

  HandleBuffer = NULL;
  Entries      = NULL;
  EntryCount   = 0;
  HandleCount  = 0;

  PciListOutput (L"PciList:\n");

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    PciListOutput (L"No PCI I/O handles found: %r\n", Status);
    return;
  }

  Entries = AllocateZeroPool (sizeof (PCI_LIST_ENTRY) * HandleCount);
  if (Entries == NULL) {
    FreePool (HandleBuffer);
    PciListOutput (L"Failed to allocate PCI list for %u handles\n", HandleCount);
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiPciIoProtocolGuid,
                    (VOID **)&PciIo
                    );
    if (EFI_ERROR (Status) || (PciIo == NULL)) {
      continue;
    }

    Status = PciIo->GetLocation (
                      PciIo,
                      &CurrentSegment,
                      &Bus,
                      &Device,
                      &Function
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = PciIo->Pci.Read (
                      PciIo,
                      EfiPciIoWidthUint32,
                      0,
                      sizeof (PCI_TYPE_GENERIC) / sizeof (UINT32),
                      &Entries[EntryCount].Pci
                      );
    if (EFI_ERROR (Status)) {
      continue;
    }

    DevicePath = DevicePathFromHandle (HandleBuffer[Index]);
    Entries[EntryCount].DevicePathText = NULL;
    if (DevicePath != NULL) {
      Entries[EntryCount].DevicePathText = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
    }

    DeviceDword = ((UINT32)Entries[EntryCount].Pci.Device.Hdr.DeviceId << 16) | Entries[EntryCount].Pci.Device.Hdr.VendorId;

    Entries[EntryCount].Handle            = HandleBuffer[Index];
    Entries[EntryCount].PciIo             = PciIo;
    Entries[EntryCount].Segment           = (UINT16)CurrentSegment;
    Entries[EntryCount].Bus               = (UINT8)Bus;
    Entries[EntryCount].Device            = (UINT8)Device;
    Entries[EntryCount].Function          = (UINT8)Function;
    Entries[EntryCount].VendorDeviceDword = DeviceDword;
    Entries[EntryCount].Status            = Entries[EntryCount].Pci.Device.Hdr.Status;
    Entries[EntryCount].RomImage          = (UINTN)PciIo->RomImage;
    Entries[EntryCount].RomSize           = PciIo->RomSize;
    EntryCount++;
  }

  if (EntryCount > 1) {
    PciListSortEntries (Entries, EntryCount);
  }

  for (Index = 0; Index < EntryCount; Index++) {
    PciListPrintFlatEntry (&Entries[Index]);
  }

  PciListOutput (L"PciTree:\n");
  PciListOutput (L"-----------------------------------------------------------------------------\n");

  SegmentCount   = 0;
  CurrentSegment = MAX_UINTN;
  for (Index = 0; Index < EntryCount; Index++) {
    if (Entries[Index].Segment != CurrentSegment) {
      PciListPrintTreeSegment (
        Entries,
        EntryCount,
        Entries[Index].Segment,
        (SegmentCount == 0)
        );
      CurrentSegment = Entries[Index].Segment;
      SegmentCount++;
    }
  }

  PciListOutput (L"-----------------------------------------------------------------------------\n");

  for (Index = 0; Index < EntryCount; Index++) {
    if (Entries[Index].DevicePathText != NULL) {
      FreePool (Entries[Index].DevicePathText);
    }
  }

  FreePool (Entries);
  FreePool (HandleBuffer);
}

STATIC
VOID
EFIAPI
PciListOnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;
  VOID        *Interface;

  (VOID)Event;
  (VOID)Context;

  if (mPciListPrinted) {
    return;
  }

  Status = gBS->LocateProtocol (
                  &gEfiPciEnumerationCompleteProtocolGuid,
                  NULL,
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[PciListDxe] PCI enumeration is not complete yet: %r\n", Status));
    return;
  }

  mPciListPrinted = TRUE;
  PciListOutput (L"[PciListDxe] PCI enumeration complete\n");
  PciListPrintAll ();
}

EFI_STATUS
EFIAPI
PciListDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_EVENT  ProtocolNotifyEvent;
  VOID       *Registration;

  (VOID)ImageHandle;
  (VOID)SystemTable;

  ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                          &gEfiPciEnumerationCompleteProtocolGuid,
                          TPL_CALLBACK,
                          PciListOnPciEnumerationComplete,
                          NULL,
                          &Registration
                          );
  if (ProtocolNotifyEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "[PciListDxe] EfiCreateProtocolNotifyEvent failed\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  PciListOnPciEnumerationComplete (ProtocolNotifyEvent, NULL);
  return EFI_SUCCESS;
}
