<% title "Quick Start" %>
<% childOrder
HelloWorld
BuildDocs
%>


To get started using Plywood, make sure you have the following software installed:

* [Git](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git)
* [CMake](https://cmake.org/install/) 3.8 or higher
* A C++14 compliant compiler such as Visual Studio 2015 or higher, the latest Xcode, GCC 5 or higher, or Clang 3.4 or higher.

## Getting the source code

First, clone the Plywood repository somewhere on your local machine.

    $ git clone https://github.com/arc80/plywood

Change to the directory you just cloned. This directory is known as the **workspace root**.

    $ cd plywood

## Building PlyTool

The first thing you must do in a new Plywood workspace is to build [PlyTool](PlyTool). PlyTool is a command-line application upon which the entire Plywood workflow is based.

To build PlyTool, you must run `Setup.cmake` using appropriate command-line arguments for your local machine. To figure out what command-line arguments to use, run `cmake -P Setup.cmake` first. It should display output similar to the following:

    $ cmake -P Setup.cmake
    *** Error: No generator chosen ***

    Please choose a generator by running one of the following commands:

    For Visual Studio 2019:
        cmake -DPRESET=vs2019 -P Setup.cmake

    For Visual Studio 2017:
        cmake -DPRESET=vs2017 -P Setup.cmake

    For Xcode:
        cmake -DPRESET=xcode -P Setup.cmake

    For Unix Makefiles:
        cmake -DPRESET=make -P Setup.cmake

Re-run `Setup.cmake` using the command line that applies to your case. For Visual Studio 2019, it would be:

    $ cmake -DPRESET=vs2019 -P Setup.cmake

This command will generate a build system for PlyTool, build PlyTool, then run PlyTool to generate code used by Plywood's reflection system. The entire process should take about 30 seconds to complete. Once it's finished, there will be a new executable `plytool` (or `plytool.exe` on Windows) in the workspace root.

To familiarize yourself further with Plywood, try building and running the [Hello World](HelloWorld) sample next!
