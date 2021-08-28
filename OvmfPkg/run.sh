#!/usr/bin/env bash

#qemu-system-x86_64 -bios /home/zhubo/00_loongson/02-code/edk/edk2/Build/OvmfIa32/DEBUG_GCC5/FV/OVMF.fd -serial stdio

path="/home/zhubo/00_loongson/02-code/edk/edk2"
# qemu-system-x86_64 -bios $path/Build/OvmfX64/DEBUG_GCC5/FV/OVMF.fd -hda fat:rw:$path/data -serial stdio 
qemu-system-x86_64 -bios $path/Build/OvmfX64/DEBUG_GCC48/FV/OVMF.fd -hda fat:rw:$path/data -serial stdio 
