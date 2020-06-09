<% title "Adding New Modules" %>

In Plywood, a [module](KeyConcepts#modules) is defined by adding a special C++ function to a file with the suffix `.modules.cpp` somewhere in a [repo](KeyConcepts#repos)'s directory tree. This function is called an **module function** and must be preceded by a line comment of the form:

    // [ply module="<module-name>"]

For example, the [`primesieve`](https://github.com/arc80/primesieve) add-on repo, which was already described in the [Creating New Repos](NewRepos) section, contains a single [`PrimeSieve.modules.cpp`](https://github.com/arc80/primesieve/blob/master/src/PrimeSieve/PrimeSieve.modules.cpp) file:

    #include <ply-build-repo/Module.h>
    
    // [ply module="PrimeSieve"]
    void module_PrimeSieve(ModuleArgs* args) {
        args->buildTarget->targetType = BuildTargetType::EXE;
        args->addSourceFiles(".", false);
        args->addTarget(Visibility::Private, "runtime");
    }

The name of the function itself (in this case, `module_PrimeSieve`) isn't really important, except that it must be unique across all `.modules.cpp` files in the current repo. By convention, the function name usually starts with `module_`.

Each time PlyTool instantiates a [compilation target](KeyConcepts#targets), such as when running [`plytool generate`](PlyTool), it calls a module function like the one above to initialize the target.

## Operations Performed by a Module Function

The following is a list of operations that can be performed when initializing a target:

* [Setting the target type](#setting-the-target-type)
* [Adding source files](#adding-source-files)
* [Adding include directories](#adding-include-directories)
* [Adding dependencies on other targets](#adding-dependencies-on-other-targets)
* [Adding dependencies on externs](#adding-dependencies-on-externs)
* [Other operations](#other-operations)

### [Setting the target type](#setting-the-target-type)

By default, every target has the type `BuildTargetType::Lib`, which means it will be built as a static library. To build an executable instead, you must change the target type to `EXE`.

    args->buildTarget->targetType = BuildTargetType::EXE;

It's also possible to build a shared library instead of a static library, but this is done by passing the `--shared` option to the [`plytool target add`](PlyTool) command. More documentation on shared libraries is forthcoming.

### [Adding source files](#adding-source-files)

You can add source files to a target by calling `addSourceFiles`:

    args->addSourceFiles(".", false);

The first argument is a relative path that's interpreted relative to the directory containing the `.modules.cpp` file itself. All `.cpp` files in this directory will be added to the target and compiled when the target is built. All `.h` files in this directory will also be added to the target so that they'll appear in IDEs such as Visual Studio and Xcode.

If the second argument is `true`, all subdirectories will be searched recursively for additional files to add.

### [Adding include directories](#adding-include-directories)

You can add include directories to a target by calling `addIncludeDir`:

    args->addIncludeDir(Visibility::Public, ".");

If the first argument is `Visibility::Public`, the include directory will be inherited by every target that depends on this one. If it's `Visibility::Private`, the include directory won't be inherited.

The second argument is a relative path that's interpreted relative to the directory containing the `.modules.cpp` file itself. Always use forward slashes `/` in the relative path even if the host machine is running Windows.

### [Adding dependencies on other targets](#adding-dependencies-on-other-targets)

You can add a dependency on another target by calling `addTarget`. When you add a dependency on another target, the other target's public include directories will be inherited by the current target, and the other target will be compiled and linked into the executable along with all of its own dependencies:

    args->addTarget(Visibility::Private, "runtime");

If the first argument is `Visibility::Public`, all include directories inherited from the dependency will also be inherited by every target that depends on this one. If it's `Visibility::Private`, none of the include directories inherited from the dependency will be inherited by other targets.

The second argument specifies the name of the module from which the dependency will be instantiated. This module is found by searching the current repo along with every repo that the current repo depends on. All other repos in the workspace excluded from the search. If the module name is ambiguous, you must specify the module's fully qualified name, such as as `"plywood.runtime"` instead of just `"runtime"`.

### [Adding dependencies on externs](#adding-dependencies-on-externs)

You can add a dependency on an [extern](KeyConcepts#externs) by calling `addExtern`. When you add a dependency on an extern, no additional source will be compiled by the generated build system, but the extern's public include directories will be inherited by the current target, and the extern's libraries will be linked into the executable along with the libraries of all of its dependencies:

    args->addExtern(Visibility::Public, "libsass");

If the first argument is `Visibility::Public`, all include directories inherited from the extern will also be inherited by every target that depends on this one. If it's `Visibility::Private`, none of the include directories inherited from the extern will be inherited by other targets.

The second argument specifies the name of the extern. An extern name is only valid if there is at least one [extern provider](KeyConcepts#extern-providers) using that name defined in the current repo or in any repo that the current repo depends on.

Note that, any time a target depends on an extern, and that target is added to a build folder, the user must select an extern provider for that build folder before PlyTool can successfully generate a build system.

### [Other operations](#other-operations)

It's possible to do other work inside a module function. For example, the [`platform` module function](https://github.com/arc80/plywood/blob/master/repos/plywood/src/platform/platform.modules.cpp) in the `plywood` repo generates a configuration header file and writes it to the build folder whenever a build system is generated using this module:

    FileSystem::native()->makeDirsAndSaveTextIfDifferent(
        NativePath::join(args->projInst->env->buildFolderPath,
                         "codegen/ply-platform/ply-platform/Config.h"),
        configFile, TextFormat::platformPreference());

Earlier in the same function, a public include directory is also added to the target so that the header file can be found.
