<% title "Building the Documentation" %>

Building Plywood's documentation on your local machine is a good way to familiarize yourself with Plywood's workflow.

First, make sure you've built [PlyTool](PlyTool) using the steps described in [Quick Start](QuickStart).

To build and view the documentation, you must perform two steps:

1. Build and run the WebCooker, which generates data files needed by the WebServer.
2. Build and run the WebServer.

## 1. Build and run the WebCooker

The WebCooker is an application written using Plywood. You must build and run it using the `plytool` (or `plytool.exe` on Windows) executable located in your workspace root.

First, create a [build folder](KeyConcepts#build-folders) named `WebCooker` by running the following command:

    $ plytool folder create WebCooker

The new build folder is located at `data/build/WebCooker` relative to the workspace root. This build folder becomes the current build folder, which means that subsequent PlyTool commands will act on it.

Add the `WebCooker` target to that build folder:

    $ plytool target add WebCooker

Next, run `plytool generate` to attempt to generate a build system in that folder. You should see output similar to the following:

    $ plytool generate
    Initializing repo registry...
    Can't generate build system in folder 'WebCooker' because extern 'libsass' is not selected.
    1 compatible provider is available:
        libsass.prebuilt (not installed)

The command failed because we haven't told PlyTool where to get [LibSass](https://sass-lang.com/libsass). Run the `plytool extern select --install` command using the name of the [provider](KeyConcepts#extern-providers) suggested in the output. On Windows, it will be `libsass.prebuilt`; on Debian/Ubuntu Linux, it will be `libsass.apt`; and on macOS, where [MacPorts](https://www.macports.org/) is currently needed, it'll be `libsass.macports`. New providers can be added in the future to support additional package managers or installation methods.

    $ plytool extern select --install libsass.prebuilt

Now run `plytool generate` again. This time, it should succeed:

    $ plytool generate

The build system is now located in the `data/build/WebCooker/build` directory relative to the workspace root. Open the project files in your IDE (such as Visual Studio or Xcode), then build and it yourself.

Once the WebCooker finishes running, there will be a bunch of data files in the `data/docsite` directory relative to your workspace root. These are the data files required by the WebServer.

## 2. Build and run the WebServer

The WebServer is another application written using Plywood. Building and running the WebServer is similar to building and running the WebCooker, except that you'll instantiate the `WebServer` target in a build folder named `WebServer`, and no third-party libraries are required:

    $ plytool folder create WebServer
    $ plytool target add WebServer
    $ plytool generate

Once the above commands have completed, another build system will be located in the `data/build/WebServer/build` directory relative to the workspace root. Open the project files in your IDE (such as Visual Studio or Xcode), then build and run it yourself.

While the WebServer is still running, navigate to the following URL in your web browser: [http://127.0.0.1:8080](http://127.0.0.1:8080)

If everything went well, you'll be greeted with the Plywood documentation running on your local machine!
