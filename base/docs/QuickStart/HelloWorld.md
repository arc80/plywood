<% title "Hello World" %>

Plywood has a built-in sample application that prints `Hello world!` to standard output. The source code is located in [`repos/plywood/src/apps/HelloWorld`](https://github.com/arc80/plywood/tree/main/repos/plywood/src/apps/HelloWorld) relative to the [workspace root](KeyConcepts#workspaces). Building and running this application is a good way to familiarize yourself with the Plywood workflow.

There are two ways to build and run this application:

* [The Fast Way: Using `plytool run --auto`](#fast)
* [The Slow Way: Performing Each Step Manually](#slow)

Make sure you've built [PlyTool](PlyTool) using the steps described in [Quick Start](QuickStart) before proceeding.

## [The Fast Way: Using `plytool run --auto`](#fast)

Simply open a command shell and run the following command from the [workspace root](KeyConcepts#workspaces). If you're running on Linux or macOS, replace `plytool` with `./plytool` instead. (It doesn't really matter what the current directory is as long as your command shell finds the `plytool` executable.)

    $ plytool run --auto HelloWorld

This command performs all of the following steps:

1. Looks for an existing [build folder](KeyConcepts#build-folders) that contains the `HelloWorld` [module](KeyConcepts#modules) as a root target. If no such folder exists, it creates a new build folder (usually named `HelloWorld`) and adds the `HelloWorld` module to that build folder as a root target. The build folder will be located at `data/build/HelloWorld` (or similar) relative to the workspace root.

2. Uses CMake to generate a build system in that folder, using the same CMake generator you selected in [Quick Start](QuickStart).

3. Builds the Debug configuration of `HelloWorld` using the build system that was just generated.

4. Runs the `HelloWorld` application that was just built.

Here's what the last few lines of output look like in Windows:

      ...
      UTCTime.cpp
      runtime.vcxproj -> C:\Jeff\plywood\data\build\HelloWorld\build\Debug\runtime.lib
      Main.cpp
      HelloWorld.vcxproj -> C:\Jeff\plywood\data\build\HelloWorld\build\Debug\HelloWorld.exe
    Running 'C:\Jeff\plywood\data\build\HelloWorld\build\Debug\HelloWorld.exe'...
    Hello world!

Note that if you run `plytool run --auto HelloWorld` a second time, it will re-use the existing build folder and skip the CMake generation step.

If you chose **Visual Studio** or **Xcode** as your generator in [Quick Start](QuickStart), you can now find the generated project files (such as a Visual Studio `.sln` or Xcode `.xcodeproj`) in the `data/build/HelloWorld/build` (or similar) relative to the workspace root.

You can also open the project files automatically in your IDE from the PlyTool command line:

    $ plytool open

For a more advanced example of working with Plywood, try [building the Plywood documentation](BuildDocs) next.

## [The Slow Way: Performing Each Step Manually](#slow)

This method is equivalent to the previous method, but we perform each step manually using [PlyTool](PlyTool).

Open a command shell and navigate to the [workspace root](KeyConcepts#workspaces). If you're running on Linux or macOS, replace `plytool` with `./plytool` in each of the following commands. (It doesn't really matter what the current directory is as long as your command shell finds the `plytool` executable.)

First, create a new [build folder](KeyConcepts#build-folders) named `HelloWorld` by running the following command:

    $ plytool folder create HelloWorld

The new build folder is located at `data/build/HelloWorld` relative to the workspace root. This build folder becomes the current build folder, which means that subsequent PlyTool commands will act on it.

Add the `HelloWorld` target to that build folder:

    $ plytool target add HelloWorld

Next, run `plytool generate` to generate a build system in that folder.

    $ plytool generate

The build system is now located in the `data/build/HelloWorld/build` directory relative to the workspace root. At this point, you can (optionally) open the project files in your IDE (such as Visual Studio or Xcode), either manually or by running the following command:

    $ plytool open

Once the IDE is open, you can build & run the application yourself.

Alternatively, you can build and run the application from the command line:

    $ plytool build
    $ plytool run

`plytool build` builds all targets in the current build folder, and `plytool run` runs the active target in the current build folder. Note that `run` automatically builds the active target before running it, so it isn't strictly necessary to run `plytool build` first.

For a more advanced example of working with Plywood, try [building the Plywood documentation](BuildDocs) next.
