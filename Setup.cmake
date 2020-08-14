cmake_minimum_required(VERSION 3.8)

# Check command line
if(DEFINED PRESET)
    unset(CMAKE_GENERATOR)
    unset(CMAKE_GENERATOR_PLATFORM)
    unset(CMAKE_GENERATOR_TOOLSET)
    set(CMAKE_BUILD_TYPE "Debug")
    if(PRESET STREQUAL vs2019)
        set(CMAKE_GENERATOR "Visual Studio 16 2019")
        set(CMAKE_GENERATOR_PLATFORM "x64")
    elseif(PRESET STREQUAL vs2019-32)
        set(CMAKE_GENERATOR "Visual Studio 16 2019")
        set(CMAKE_GENERATOR_PLATFORM "Win32")
    elseif(PRESET STREQUAL vs2017)
        set(CMAKE_GENERATOR "Visual Studio 15 2017")
        set(CMAKE_GENERATOR_PLATFORM "x64")
    elseif(PRESET STREQUAL vs2017-32)
        set(CMAKE_GENERATOR "Visual Studio 15 2017")
        set(CMAKE_GENERATOR_PLATFORM "Win32")
    elseif(PRESET STREQUAL vs2015)
        set(CMAKE_GENERATOR "Visual Studio 14 2015")
        set(CMAKE_GENERATOR_PLATFORM "x64")
    elseif(PRESET STREQUAL xcode)
        set(CMAKE_GENERATOR "Xcode")
    elseif(PRESET STREQUAL make)
        set(CMAKE_GENERATOR "Unix Makefiles")
    else()
        message("*** Error: Unknown generator type '${PRESET}' ***\n")
    endif()
elseif(DEFINED CMAKE_GENERATOR)
    if(NOT DEFINED CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Debug")
    endif()
else()
    message("*** Error: No generator chosen ***\n")
endif()

if(NOT DEFINED CMAKE_GENERATOR)
    message("Please choose a generator by running one of the following commands:\n")
    foreach(pair
        "vs2019;Visual Studio 2019"
        "vs2017;Visual Studio 2017"
        "xcode;Xcode"
        "make;Unix Makefiles")
        list(GET pair 0 shortName)
        list(GET pair 1 description)
        message("For ${description}:")
        message("    cmake -DPRESET=${shortName} -P Setup.cmake\n")
    endforeach()
    message("Alternatively, you can define CMAKE_GENERATOR directly. If this variable is
set, the optional CMAKE_GENERATOR_PLATFORM, CMAKE_GENERATOR_TOOLSET and
CMAKE_BUILD_TYPE variables can also be set.")
    return()
endif()

# Set workspace folder
set(workspaceFolder "${CMAKE_CURRENT_LIST_DIR}")
if(NOT workspaceFolder MATCHES ".*/$")
    set(workspaceFolder "${workspaceFolder}/")
endif()

# Set build folder
set(buildFolder "${workspaceFolder}data/build/bootstrap/plytool/")
if(BUILD_FOLDER)
    set(buildFolder "${BUILD_FOLDER}")
    if(NOT buildFolder MATCHES ".*/$")
        set(buildFolder "${buildFolder}/")
    endif()
    if (NOT EXISTS "${buildFolder}")
        message(FATAL_ERROR "Directory '${BUILD_FOLDER}' does not exist")
    endif()
endif()

# Copy all the source code needed for PlyTool (unless -DNO_BACKUP was passed)
set(srcCodeToBuildFolder "${workspaceFolder}repos/plywood/src/")
if(NOT NO_BACKUP)
    message("Backing up source files...")
    set(srcCodeToBuildFolder "${buildFolder}src/")
    foreach(relPath
        "apps/plytool"
        "build"
        "reflect"
        "runtime"
        "platform"
        "pylon/reflect"
        "pylon/pylon"
        "cpp")
        set(srcPath "${workspaceFolder}repos/plywood/src/${relPath}")
        set(dstPath "${srcCodeToBuildFolder}${relPath}")
        string(REGEX REPLACE "/[^/]*$" "" dstPath "${dstPath}") # Strip last component
        file(COPY "${srcPath}" DESTINATION "${dstPath}")
        # FIXME: Make destination files read-only and add message to the top of each one
    endforeach()
endif()

# Generate CMakeLists.txt in the build folder
file(READ "${workspaceFolder}repos/plywood/scripts/bootstrap_CMakeLists.txt" cmakeListsContents)
string(REPLACE "<<<WORKSPACE_FOLDER>>>" "${workspaceFolder}" cmakeListsContents "${cmakeListsContents}")
string(REPLACE "<<<SRC_FOLDER>>>" "${srcCodeToBuildFolder}" cmakeListsContents "${cmakeListsContents}")
string(REPLACE "<<<BUILD_FOLDER>>>" "${buildFolder}" cmakeListsContents "${cmakeListsContents}")
file(WRITE "${buildFolder}CMakeLists.txt" "${cmakeListsContents}")

# Generate codegen/ply-platform/ply-platform/Config.h in the build folder
file(READ "${workspaceFolder}repos/plywood/scripts/bootstrap_Config.h" configContents)
string(REPLACE "<<<WORKSPACE_FOLDER>>>" "${workspaceFolder}" configContents "${configContents}")
string(REPLACE "<<<SRC_FOLDER>>>" "${srcCodeToBuildFolder}" configContents "${configContents}")
string(REPLACE "<<<BUILD_FOLDER>>>" "${buildFolder}" configContents "${configContents}")
string(REPLACE "<<<CMAKE_PATH>>>" "${CMAKE_COMMAND}" configContents "${configContents}")
file(WRITE "${buildFolder}codegen/ply-platform/ply-platform/Config.h" "${configContents}")

# Generate codegen/ply-build-target/ply-build-target/NativeToolchain.inl in the build folder
set(nativeToolchainContents
"CMakeGeneratorOptions NativeToolchain = {
    \"${CMAKE_GENERATOR}\",
    \"${CMAKE_GENERATOR_PLATFORM}\",
    \"${CMAKE_GENERATOR_TOOLSET}\",
    \"\",
};
String DefaultNativeConfig = \"${CMAKE_BUILD_TYPE}\";
")
file(WRITE "${buildFolder}codegen/ply-build-target/ply-build-target/NativeToolchain.inl" "${nativeToolchainContents}")

# Copy Helper.cmake to the build folder
file(COPY "${workspaceFolder}repos/plywood/scripts/Helper.cmake"
     DESTINATION "${buildFolder}")

# Generate build system for PlyTool
message("Generating build system for PlyTool...")
file(MAKE_DIRECTORY "${buildFolder}build")
set(cmakeOptions .. -G "${CMAKE_GENERATOR}")
if(DEFINED CMAKE_GENERATOR_PLATFORM)
    list(APPEND cmakeOptions -A "${CMAKE_GENERATOR_PLATFORM}")
endif()
if(DEFINED CMAKE_GENERATOR_TOOLSET)
    list(APPEND cmakeOptions -T "${CMAKE_GENERATOR_TOOLSET}")
endif()
if(DEFINED CMAKE_BUILD_TYPE)
    list(APPEND cmakeOptions "-DCMAKE_BUILD_TYPE=\"${CMAKE_BUILD_TYPE}\"")
endif()
list(APPEND cmakeOptions -DCMAKE_C_COMPILER_FORCED=1 -DCMAKE_CXX_COMPILER_FORCED=1)
execute_process(COMMAND "${CMAKE_COMMAND}" ${cmakeOptions}
                WORKING_DIRECTORY "${buildFolder}build"
                RESULT_VARIABLE resultCode)
if (NOT resultCode EQUAL "0")
    message(FATAL_ERROR "Error generating PlyTool: ${resultCode}")
endif()

# Write default workspace-settings.pylon
if(NOT EXISTS "${workspaceFolder}workspace-settings.pylon")
    file(WRITE "${workspaceFolder}workspace-settings.pylon"
"{
}
")
endif()

# Build PlyTool
message("Building PlyTool...")
if (CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    execute_process(COMMAND "make" "-j" "4"
                    WORKING_DIRECTORY "${buildFolder}build"
                    RESULT_VARIABLE resultCode)
else()
    set(args --build .)
    if(DEFINED CMAKE_BUILD_TYPE)
        set(args ${args} --config "${CMAKE_BUILD_TYPE}")
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" ${args}
                    WORKING_DIRECTORY "${buildFolder}build"
                    RESULT_VARIABLE resultCode)
endif()
if (NOT resultCode EQUAL "0")
    message(FATAL_ERROR "Error building PlyTool: ${resultCode}")
endif()

# Check for plytool executable
if(EXISTS "${workspaceFolder}plytool.exe")
    set(PLYTOOL_PATH "${workspaceFolder}plytool.exe")
elseif(EXISTS "${workspaceFolder}plytool")
    set(PLYTOOL_PATH "${workspaceFolder}plytool")
else()
    message(FATAL_ERROR "Can't find PlyTool executable")
endif()

# Build instantiator DLLs
execute_process(COMMAND "${PLYTOOL_PATH}" module update
                RESULT_VARIABLE resultCode)
if (NOT resultCode EQUAL "0")
    message(FATAL_ERROR "Error running PlyTool: ${resultCode}")
endif()

# Do codegen
message("Generating code using PlyTool...")
execute_process(COMMAND "${PLYTOOL_PATH}" codegen
                RESULT_VARIABLE resultCode)
if (NOT resultCode EQUAL "0")
    message(FATAL_ERROR "Error running PlyTool: ${resultCode}")
endif()

# Success!
message("PlyTool is located at ${PLYTOOL_PATH}")
