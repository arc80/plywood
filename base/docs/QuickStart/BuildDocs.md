<% title "Building the Documentation" %>

Building Plywood's documentation is similar to building and running the [Hello World](HelloWorld) sample, except that it involves two steps:

1. Build and run the WebCooker, which generates data files needed by the WebServer.
2. Build and run the WebServer.

Additionally, building the WebCooker requires choosing an [extern provider](KeyConcepts#extern-providers), which helps demonstrate how Plywood works with third-party libraries.

Before you proceed, make sure you've built [Crowbar](Crowbar) using the steps described in [Quick Start](QuickStart).

## 1. Build and run the WebCooker

First, open a command shell and run the following command from the [workspace root](KeyConcepts#workspaces). If you're running on Linux or macOS, replace `crowbar` with `./crowbar` instead. (It doesn't really matter what the current directory is as long as your command shell finds the `crowbar` executable.)

    $ crowbar run --auto WebCooker

You should see output similar to the following:

    Created build folder 'WebCooker' with root target 'WebCooker' at: C:\Jeff\plywood\data\build\WebCooker\
    Can't generate build system in folder 'WebCooker' because extern 'libsass' is not selected.
    1 compatible provider is available:
        libsass.prebuilt (not installed)

This command created a new [build folder](KeyConcepts#build-folders) `WebCooker` and added the [root target](KeyConcepts#targets) `WebCooker` to that folder. However, it fails after that because we haven't told Crowbar where to get [LibSass](https://sass-lang.com/libsass).

To solve this problem, run the `crowbar extern select --install` command using the name of the [provider](KeyConcepts#extern-providers) suggested in the previous output. On Windows, it will be `libsass.prebuilt`; on Debian/Ubuntu Linux, it will be `libsass.apt`; and on macOS, where [MacPorts](https://www.macports.org/) is currently needed, it'll be `libsass.macports`. New providers can be added in the future to support additional package managers or installation methods.

    $ crowbar extern select --install libsass.prebuilt

<% note In the future, Crowbar will have a mechanism to automatically select and install appropriate extern providers for your system, so the above step will be performed automatically. %>

Now run `crowbar run --auto WebCooker` again. It will re-use the existing build folder, and this time it should successfully generate, build and run the WebCooker:

    $ crowbar run --auto WebCooker
    Generating build system for 'WebCooker'...
    Successfully generated build system in folder 'WebCooker'.
    Building Debug configuration of 'WebCooker'...
      ...
      Main.cpp
      WebCooker.vcxproj -> C:\Jeff\plywood\data\build\WebCooker\build\Debug\WebCooker.exe
    Running 'C:\Jeff\plywood\data\build\WebCooker\build\Debug\WebCooker.exe'...    

Once the WebCooker finishes running, there will be a bunch of data files in the `data/docsite` directory relative to your workspace root. These are the data files required by the WebServer.

## 2. Build and run the WebServer

Next, build and run the WebServer using the following command. Note that this time, we specify `--add` instead of `--auto`. The `--add` option makes Crowbar re-use the same build folder; `WebServer` is simply added an additional target. (We could use `--auto` here, in which case Crowbar will create a separate build folder, but `--add` is quicker because several required modules are already built in the existing folder.)

    $ crowbar run --add WebServer
    Added target 'WebServer' to folder 'WebCooker'.
    Generating build system for 'WebCooker'...
    Successfully generated build system in folder 'WebCooker'.
    Building Debug configuration of 'WebServer'...
      ...
      WebServer.vcxproj -> C:\Jeff\plywood\data\build\WebCooker\build\Debug\WebServer.exe
    Running 'C:\Jeff\plywood\data\build\WebCooker\build\Debug\WebServer.exe'...
    Serving from C:\Jeff\plywood\data\docsite on port 8080

While the WebServer is still running, navigate to the following URL in your web browser: [http://127.0.0.1:8080](http://127.0.0.1:8080)

If everything went well, you'll be greeted with the Plywood documentation running on your local machine!

If you chose **Visual Studio** or **Xcode** as your generator in [Quick Start](QuickStart), you may now find the generated project files (such as a Visual Studio `.sln` or Xcode `.xcodeproj`) in the `data/build/WebCooker/build` (or similar) relative to the workspace root.

You can also open the project files automatically in your IDE from the Crowbar command line:

    $ crowbar open
