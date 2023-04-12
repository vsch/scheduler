cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 98)
set(PROJECT_NAME LedFlashers)

set(${PROJECT_NAME}_BOARD pro)
set(ARDUINO_CPU 8MHzatmega328)
project(${PROJECT_NAME})

# Define additional source and header files or default arduino sketch files
# set(${PROJECT_NAME}_SRCS)
# set(${PROJECT_NAME}_HDRS)

### Additional static libraries to include in the target.
# set(${PROJECT_NAME}_LIBS)

### Main sketch file
set(${PROJECT_NAME}_SKETCH LedFlashers.ino)

### Add project directories into the build
# add_subdirectory()

### Additional settings to add non-standard or your own Arduino libraries.
# For this example (libs will contain additional arduino libraries)
# An Arduino library my_lib will contain files in libs/my_lib/: my_lib.h, my_lib.cpp + any other cpp files
link_directories(../../..)

# For nested library sources replace ${LIB_NAME} with library name for each library
# set(_RECURSE true)

#### Additional settings for programmer. From programmers.txt
set(${PROJECT_NAME}_PROGRAMMER avrispmkii)
set(${PROJECT_NAME}_PORT /dev/cu.usbserial-00000000)
set(pro.upload.speed 57600)

## Verbose build process
set(${PROJECT_NAME}_AFLAGS -v)

generate_arduino_firmware(${PROJECT_NAME})
