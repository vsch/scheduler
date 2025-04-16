#!/usr/bin/env bash
# dump both VS Code and CLion builds
EXE_NAME=Scheduler
BUILD_DIR=cmake-build-debug
avr-objdump -S cmake-build-debug/${EXE_NAME}.elf >cmake-build-debug/${EXE_NAME}.lst
avr-objdump -S build/${EXE_NAME}.elf >build/${EXE_NAME}.lst
