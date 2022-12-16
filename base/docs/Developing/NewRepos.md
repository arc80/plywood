<% title "Creating New Repos" %>

When starting a new project using Plywood, you must typically create a new [repo](KeyConcepts#repos) to hold that project. This new repo will hold the source code, [modules](KeyConcepts#modules) and [extern providers](KeyConcepts#extern-providers) for your project. You can also combine multiple projects into a single repo, but either way, you'll have to create a new repo at some point.

Each repo is stored in a directory immediately below the `repos` directory in the workspace root. At a minimum, each repo must contain the following:

* An `info.pylon` file.
* A `.modules.cpp` file.
* Some C++ source code for each module.

To demonstrate what an add-on repo looks like, there's a example repo on GitHub named [primesieve](https://github.com/arc80/primesieve). Try cloning this repo into your workspace and building the `PrimeSieve` sample application it contains. This repo should be cloned directly underneath the `repos` folder relative to your [workspace root](DirectoryStructure):

    $ cd repos
    $ git clone https://github.com/arc80/primesieve

After cloning this add-on repo, there should be a `repos/primesieve` folder relative to the workspace root.

<% html
<svg xmlns="http://www.w3.org/2000/svg" height="244" width="372" xmlns:xlink="http://www.w3.org/1999/xlink" class="center">
 <defs>
  <linearGradient id="a">
   <stop stop-color="#fdfda0" offset="0"/>
   <stop stop-color="#f5d334" offset="1"/>
  </linearGradient>
  <linearGradient id="d" y2="476.6" xlink:href="#a" gradientUnits="userSpaceOnUse" x2="334.28" gradientTransform="translate(-246,419)" y1="467.31" x1="334.28"/>
  <linearGradient id="c" y2="479.69" xlink:href="#a" gradientUnits="userSpaceOnUse" x2="334.28" gradientTransform="translate(-246,419)" y1="471.99" x1="334.28"/>
  <linearGradient id="f" y2="476.6" xlink:href="#a" gradientUnits="userSpaceOnUse" x2="334.28" gradientTransform="translate(-327,-6)" y1="467.31" x1="334.28"/>
  <linearGradient id="e" y2="480.56" xlink:href="#a" gradientUnits="userSpaceOnUse" x2="340.19" gradientTransform="translate(-327,-6)" y1="471.67" x1="340.19"/>
 </defs>
 <g transform="translate(0,-353)" stroke="#d6d6d6" stroke-width="2" fill="none">
  <path d="m65 423v35c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m65 434c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m93 471v35c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m93 482c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m121 519v11c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m149 543v35c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m149 554c0 2.5223 1.7508 4 4 4h10"/>
  <path d="m37 399v11c0 2.5223 1.7508 4 4 4h10"/>
 </g>
 <g transform="translate(0 -808.36)">
  <text style="word-spacing:0px;letter-spacing:0px" xml:space="preserve" font-size="15px" line-height="125%" y="848.36218" x="56" font-family="&apos;Liberation Sans&apos;" fill="#4d4d4d"><tspan x="56" y="848.36218">plywood</tspan></text>
  <g id="b" transform="translate(25,377)">
   <path stroke-linejoin="round" d="m2.5 459.86c0-.5.5-1 1-1h5c.61314-.0101 1 .50686 1 1v2h10c.58677 0 1 .42085 1 1l.00005 10c.0178.55171-.43366 1-1 1h-16c-.50733 0-1-.44383-1-1z" stroke="#dcbf6a" fill="url(#f)"/>
   <path stroke-linejoin="round" d="m7.3586 465.86c.2858-.5 1.0716-1 1.5716-1h16c.58677 0 .75944.42085.4284 1l-4.0011 7c-.29756.55171-1.0053 1-1.5716 1h-16c-.50733 0-.74631-.44383-.4284-1z" stroke="#dcbf6a" fill="url(#e)"/>
  </g>
  <text style="word-spacing:0px;letter-spacing:0px" font-size="15px" line-height="125%" y="872.36218" x="84" font-family="&apos;Liberation Sans&apos;" xml:space="preserve" fill="#4d4d4d"><tspan y="872.36218" x="84">repos</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" xml:space="preserve" font-size="15px" line-height="125%" y="896.36218" x="112" font-family="&apos;Liberation Sans&apos;" fill="#4d4d4d"><tspan x="112" y="896.36218">plywood</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" xml:space="preserve" font-size="15px" line-height="125%" y="944.36218" x="140" font-family="&apos;Liberation Sans&apos;" fill="#4d4d4d"><tspan x="140" y="944.36218">info.pylon</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" xml:space="preserve" font-size="15px" line-height="125%" y="968.36218" x="140" font-family="&apos;Liberation Sans&apos;" fill="#4d4d4d"><tspan x="140" y="968.36218">src</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" font-size="15px" line-height="125%" y="992.36218" x="168" font-family="&apos;Liberation Sans&apos;" xml:space="preserve" fill="#4d4d4d"><tspan y="992.36218" x="168">PrimeSieve</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" xml:space="preserve" font-size="15px" line-height="125%" y="1016.3622" x="196" font-family="&apos;Liberation Sans&apos;" fill="#4d4d4d"><tspan x="196" y="1016.3622">PrimeSieve.modules.cpp</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px" font-size="15px" line-height="125%" y="1040.3622" x="196" font-family="&apos;Liberation Sans&apos;" xml:space="preserve" fill="#4d4d4d"><tspan y="1040.3622" x="196">Main.cpp</tspan></text>
  <use xlink:href="#b" transform="translate(28,24)" height="100%" width="100%" y="0" x="0"/>
  <path stroke-linejoin="round" d="m83.5 884.86c0-.5.5-1 1-1h5c.61314-.0101 1 .50686 1 1v2c.27978 3.9591-4.8568 3.8564-7 1z" stroke="#dcbf6a" fill="url(#d)"/>
  <use xlink:href="#b" transform="translate(84,120)" height="100%" width="100%" y="0" x="0"/>
  <use xlink:href="#b" transform="translate(112,144)" height="100%" width="100%" y="0" x="0"/>
  <text style="word-spacing:0px;letter-spacing:0px" font-size="15px" line-height="125%" y="920.36218" x="112" font-family="&apos;Liberation Sans&apos;" xml:space="preserve" fill="#4d4d4d"><tspan y="920.36218" x="112">primesieve</tspan></text>
  <use xlink:href="#b" transform="translate(56,72)" height="100%" width="100%" y="0" x="0"/>
  <g transform="translate(81,449)">
   <path d="m32.5 481.86h13l3 3v15h-16z" stroke="#acacac" fill="#fff"/>
   <path d="m36 489.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 491.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 493.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 495.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m45.5 481.86v3h3" stroke="#acacac" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
  </g>
  <g transform="translate(137,521)">
   <path d="m32.5 481.86h13l3 3v15h-16z" stroke="#acacac" fill="#fff"/>
   <path d="m36 489.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 491.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 493.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 495.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m45.5 481.86v3h3" stroke="#acacac" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
  </g>
  <g transform="translate(137,545)">
   <path d="m32.5 481.86h13l3 3v15h-16z" stroke="#acacac" fill="#fff"/>
   <path d="m36 489.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 491.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 493.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 495.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m45.5 481.86v3h3" stroke="#acacac" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
   <path d="m36 487.86h9" stroke="#aac1ce" stroke-width="1px" fill="none"/>
  </g>
  <rect stroke-linejoin="round" stroke-dasharray="4.00000014, 4.00000014" transform="scale(-1)" stroke-dashoffset="1.8" rx="11" ry="11" height="147" width="297" stroke="#d04545" stroke-linecap="round" y="-1050.4" x="-369" stroke-width="2" fill="none"/>
  <text style="word-spacing:0px;letter-spacing:0px;text-anchor:middle;text-align:center" font-size="15px" line-height="125%" y="941.86218" x="31.621338" font-family="Asap" xml:space="preserve" fill="#d04545"><tspan font-style="italic" line-height="100%" y="941.86218" x="31.621338" font-family="Asap" fill="#d04545">new files</tspan></text>
  <text style="word-spacing:0px;letter-spacing:0px;text-anchor:middle;text-align:center" xml:space="preserve" font-size="15px" line-height="125%" y="821.86218" x="183.62134" font-family="Asap" fill="#d04545"><tspan font-style="italic" line-height="100%" y="821.86218" x="183.62134" font-family="Asap" fill="#d04545">workspace root</tspan></text>
  <path d="m117 844.36c21.937-3.1323 34.044-9.541 40-18" stroke="#d04545" stroke-width="1px" fill="none"/>
  <path d="m121.44 839.21-4.1694 4.9675 5.4675 3.773" stroke="#d04545" stroke-width="1px" fill="none"/>
  <path stroke-linejoin="round" d="m83.5 887.86c0-.5.5-1 1-1l16 .00002c.58677 0 1 .42085 1 1l.00005 10c.0178.55171-.43366 1-1 1h-16c-.50733 0-1-.44383-1-1z" stroke="#dcbf6a" fill="url(#c)"/>
 </g>
</svg>
%>

Let's look at the contents of `primesieve`.

## Contents of the `primesieve` Repo

<% member info.pylon %>

The [`info.pylon`](https://github.com/arc80/primesieve/blob/main/info.pylon) file holds basic information about the repo. This file contains a single JSON object. The object's `dependsOn` property lists other repos are required by this repo. In this case, the `primesieve` repo depends only on the built-in `plywood` repo:

    {
      "dependsOn": ["plywood"]
    }

Repos are only allowed to use modules and extern providers defined inside themselves, inside one of the repo dependencies listed in `info.pylon`, or inside a transitive dependency. Cyclic dependencies between repos are not allowed.

<% member src/PrimeSieve/PrimeSieve.modules.cpp %>

In Plywood, files having the filename suffix `.modules.cpp` that are located anywhere within a repo are used to add [modules](KeyConcepts#modules) and [extern providers](KeyConcepts#extern-providers) to that repo. The file must begin with the directive `#include <ply-build-repo/Module.h>`.

In this case, the [`PrimeSieve.modules.cpp`](https://github.com/arc80/primesieve/blob/main/src/PrimeSieve/PrimeSieve.modules.cpp) file contains a single C++ function. The comment before the function, `// [ply module="PrimeSieve"]`, tells Crowbar that the function is a **module function** that defines a new module named `PrimeSieve`:

    #include <ply-build-repo/Module.h>

    // [ply module="PrimeSieve"]
    void module_PrimeSieve(ModuleArgs* args) {
        args->buildTarget->targetType = BuildTargetType::EXE;
        args->addSourceFiles(".", false);
        args->addTarget(Visibility::Private, "runtime");
    }

Any time a target based on `PrimeSieve` is added to a build folder (by running `crowbar target add PrimeSieve`) and a build system is generated for that build folder, Crowbar executes the C++ function provided here in order to initialize that target. In this case, the function does three things:

1. Marks the target as an executable.
2. Adds all the source code in the current directory `"."` to the target. In this case, the current directory contains a single source file, `Main.cpp`. The `false` argument tells Crowbar not to recurse into subdirectories, which isn't relevant here since there aren't any subdirectories.
3. Adds a target named `runtime` as a dependency of the current target. Because `plywood` is listed as child repo of the current repo, the dependency target will be initialized from the `runtime` module located in the `plywood` repo.

<% member src/PrimeSieve/Main.cpp %>

This is the only source file belonging to the `PrimeSieve` module. It contains the source code for the `PrimeSieve` application itself. [This source file](https://github.com/arc80/primesieve/blob/main/src/PrimeSieve/Main.cpp) begins with an `#include` statement:

    #include <ply-runtime/Base.h>

Because the `plywood.runtime` module was listed as a dependency of the the `PrimeSieve` module, the header file will be found in [`repos/plywood/src/runtime`](https://github.com/arc80/plywood/tree/main/repos/plywood/src/runtime) relative to the workspace root.

<% endMembers %>

## Building the `PrimeSieve` Application

To build `PrimeSieve` in its own build folder, execute the following commands in the workspace root, where the `crowbar` executable is located. (This executable was created when you [set up the Plywood workspace](QuickStart).) If you're running on Linux or macOS, replace `crowbar` with `./crowbar` instead:

    $ crowbar folder create PrimeSieve
    $ crowbar target add PrimeSieve
    $ crowbar generate

The `crowbar folder create` command creates a new [build folder](KeyConcepts.md#build-folder). The name of the build folder is not important as long as it's unique.

The `crowbar target add` command adds a new compilation target to the build folder based on the `PrimeSieve` module. The `PrimeSieve` module will be found in the `primesieve` repo because that's the only repo that contains a module with that name. If you ever have multiple repos that define modules with the same name, you'll have to specify the fully qualified name of the module, which in this case would be `primesieve.PrimeSieve`.

The `crowbar generate` command creates a new build system in the build folder. This build system will contain a compilation target based on the `PrimeSieve` module as well as compilation targets for all dependencies of that module, which in this case are the `runtime` module (defined in the `plywood` repo) and the `platform` module (also defined in the `plywood` repo). Incidentally, you can view a tree diagram of the modules required by a build folder by running `crowbar target graph`:

    $ crowbar target graph
    Initializing repo registry...
    Dependency graph for folder 'PrimeSieve':
        PrimeSieve
        `-- runtime
            `-- platform

The build system for `PrimeSieve` should now be located in the `data/build/PrimeSieve/build` directory relative to the workspace root. Open the project files in your IDE (such as Visual Studio or Xcode), then build and it yourself.
