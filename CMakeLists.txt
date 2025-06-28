cmake_minimum_required(VERSION 3.5...3.31 FATAL_ERROR)
set(CMAKE_TOOLCHAIN_FILE /usr/local/etc/arduino-cmake/ArduinoToolchain.cmake)
set(CMAKE_CXX_STANDARD 11)
set(PROJECT_NAME Scheduler)

# Arduino.CMake internal, does not seem to work
set(${PROJECT_NAME}_DEBUG true)

# Configuration
set(BOARD_NANO 1)
set(DEBUG_BUILD 0)

if (BOARD_NANO)
    set(ARDUINO_CPU 16MHzatmega328)
    set(${PROJECT_NAME}_BOARD pro)
else ()
    set(${PROJECT_NAME}_BOARD pro)
    set(ARDUINO_CPU 16MHzatmega328)
endif ()

# Enable ASM language
project(${PROJECT_NAME} LANGUAGES C CXX ASM)

# Config defines for ssd1306 with TwiController
add_compile_definitions(GFX_PAGED_UPDATES)
add_compile_definitions(GFX_UPDATE_PAGES=1)
add_compile_definitions(INCLUDE_TWI_INT)

# Define additional source and header files or default arduino sketch files
set(${PROJECT_NAME}_SRCS
        src/Queue.cpp
        src/Mutex.cpp
        src/ByteStream.cpp
        src/Signals.cpp
        src/TinySwitcher.S
        src/Scheduler.cpp
        src/Controller.cpp
        src/TwiController.cpp
        src/twiint.c
        )
set(${PROJECT_NAME}_HDRS
        src/Queue.h
        src/Mutex.h
        src/CByteStream.h
        src/ByteStream.h
        src/Signals.h
        src/TinySwitcher.h
        src/Scheduler.h
        src/Controller.h
        src/TwiController.h
        src/CTwiController.h
        src/twiint.h
        )

#add_compile_definitions(SERIAL_DEBUG)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_MAX_STACKS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_ERRORS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_DELAYS)
#add_compile_definitions(SERIAL_DEBUG_SCHEDULER_RUN)

# Disable optimizations and enable debug symbols for debugging
if (DEBUG_BUILD)
    set(CMAKE_CXX_FLAGS_DEBUG "-O1 -g -fno-inline -fverbose-asm")
endif ()

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

# Generate the Arduino firmware
generate_arduino_firmware(${PROJECT_NAME})

# print_board_list()
# print_programmer_list()
