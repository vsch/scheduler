#!/usr/bin/env bash
EXE_NAME=Scheduler
BUILD_DIR=cmake-build-debug
osascript -e 'tell app "Terminal" to do script "/Users/vlad/src/arduino/libs/Scheduler/sim.sh"'
avr-gdb ./${BUILD_DIR}/${EXE_NAME}.elf -x simavr.dbg
