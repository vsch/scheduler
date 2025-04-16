#!/usr/bin/env bash
EXE_NAME=Scheduler
BUILD_DIR=cmake-build-debug
cd /Users/vlad/src/arduino/libs/${EXE_NAME}/ || exit
simavr -g -t -f 16000000 -m atmega328p ./${BUILD_DIR}/${EXE_NAME}.elf
