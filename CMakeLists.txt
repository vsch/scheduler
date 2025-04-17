cmake_minimum_required(VERSION 3.5...3.31 FATAL_ERROR)
set(CMAKE_TOOLCHAIN_FILE /usr/local/etc/arduino-cmake/ArduinoToolchain.cmake)
set(CMAKE_CXX_STANDARD 98)
set(PROJECT_NAME Scheduler)

set(BOARD_NANO 1)

if (BOARD_NANO)
    set(ARDUINO_CPU atmega328old)
    set(${PROJECT_NAME}_BOARD nano)
else ()
    set(${PROJECT_NAME}_BOARD pro)
    set(ARDUINO_CPU 16MHzatmega328)
endif ()

set(${PROJECT_NAME}_DEBUG true)

# Enable ASM language
project(${PROJECT_NAME} LANGUAGES C CXX ASM)

# Define additional source and header files or default arduino sketch files
set(${PROJECT_NAME}_SRCS
        src/Scheduler.cpp
        src/ResourceLock.cpp
        src/Queue.cpp
        src/Signal.cpp
        src/TinySwitcher.S
        src/SignalBase.cpp
        )

set(${PROJECT_NAME}_HDRS
        src/Scheduler.h
        src/ResourceLock.h
        src/Queue.h
        src/Signal.h
        src/TinySwitcher.h
        src/SignalBase.h
        src/RequestBase.h
        )

#add_compile_definitions(SERIAL_DEBUG)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_MAX_STACKS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_ERRORS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_DELAYS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_RUN)

# Disable optimizations and enable debug symbols for debugging
#set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fno-inline -fverbose-asm")
#set(CMAKE_C_FLAGS_DEBUG "-O1 -g -fno-inline -fverbose-asm")

### Additional static libraries to include in the target.
# set(${PROJECT_NAME}_LIBS)

### Main sketch file
 set(${PROJECT_NAME}_SKETCH scheduler_test.ino)

### Add project directories into the build
# add_subdirectory()

### Additional settings to add non-standard or your own Arduino libraries.
# For this example (libs will contain additional arduino libraries)
# An Arduino library my_lib will contain files in libs/my_lib/: my_lib.h, my_lib.cpp + any other cpp files
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/~/Documents/Arduino/libraries)

# This is no longer need but left in for those occasions where automatics do not work out.
# Current Arduino.cmake will recurse into any libraries which contain source files in src/ subdirectory.
# Files in subdirectories whose name starts with cmake-build or examples are excluded
# For nested library sources replace ${LIB_NAME} with library name for each library
# set(_RECURSE true)

#### Additional settings for programmer. From programmers.txt
set(${PROJECT_NAME}_PROGRAMMER avrispmkii)

if (BOARD_NANO)
    set(${PROJECT_NAME}_PORT /dev/cu.usbserial-A5069RR4)
    set(nano.upload.speed 57600)
    add_compile_definitions(BAUD=${nano.upload.speed})
else ()
    set(${PROJECT_NAME}_PORT /dev/cu.usbserial-00000000)
    set(pro.upload.speed 57600)
    add_compile_definitions(BAUD=${pro.upload.speed})
endif ()

add_compile_definitions(TWI_FREQUENCY=400000)

## Verbose build process
set(${PROJECT_NAME}_AFLAGS -v)

generate_arduino_firmware(${PROJECT_NAME})
