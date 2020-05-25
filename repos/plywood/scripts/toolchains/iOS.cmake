cmake_minimum_required(VERSION 3.6)     # For CMAKE_TRY_COMPILE_TARGET_TYPE
set(CMAKE_SYSTEM_NAME Generic)
set(UNIX True)
set(APPLE True)
set(IOS True)

execute_process(COMMAND xcode-select -print-path OUTPUT_VARIABLE XCODE_SELECT OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT EXISTS ${XCODE_SELECT})
    message(FATAL_ERROR "xcode-select not found")
endif()

# Setting CMAKE_OSX_SYSROOT seems to do almost everything:
set(CMAKE_OSX_SYSROOT "${XCODE_SELECT}/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")

# CMAKE_SYSTEM_FRAMEWORK_PATH is needed for find_library to find frameworks in the SDK:
set(CMAKE_SYSTEM_FRAMEWORK_PATH "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks")

# CMAKE_TRY_COMPILE_TARGET_TYPE is used to avoid having to deal with code signing and bundles
# when CMake checks for a working C/C++ compiler. (Requires CMake 3.6.)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
