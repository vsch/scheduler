#!/usr/bin/env bash
EXE_NAME=Scheduler
BUILD_DIR=cmake-build-debug
cd /Users/vlad/src/arduino/libs/${EXE_NAME}/ || exit
echo Loading /Users/vlad/src/arduino/libs/${EXE_NAME}/${BUILD_DIR}/${EXE_NAME}.elf [copied the clipboard]
echo /Users/vlad/src/arduino/libs/${EXE_NAME}/${BUILD_DIR}/${EXE_NAME}.elf | pbcopy
simavr -g -t -f 16000000 -m atmega328p ./${BUILD_DIR}/${EXE_NAME}.elf
