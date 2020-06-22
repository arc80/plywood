<% title "PlyTool Reference" %>
<% synopsis 
PlyTool is a tool to help generate build pipelines and manage external dependencies.
%>

## Folder Commands

<% member plytool folder list %>

Outputs a list of build folders.

Build folders are found by recursively scanning `data/build` relative to the workspace root for folders containing `info.pylon`.

<% member plytool folder set [em <folderName>] %>

Sets the current build folder to _name_. The current build folder name is stored in `workspace-settings.pylon` in the workspace root.

<% member plytool folder create [em <folderName>] %>

Creates a new build folder inside `data/build` relative to the workspace root and makes it the current build folder.

<% member plytool folder delete [em <folderName>] %>

Deletes the specified build folder.

<% endMembers %>

## Module Commands

<% member plytool module list %>

Outputs a list of available modules across all repos.

<% endMembers %>

## Target Commands

<% member plytool target list %>

Outputs the current build folder's root target list. The root target list is stored in the build folder's `info.pylon`.

<% member plytool target add [em <targetName>] %>

Adds a new root target to the current build folder.

<% member plytool target remove [em <targetName>] %>

Removes a new root target from the current build folder.

<% member plytool target graph %>

Outputs a dependency graph for the current build folder.

<% member plytool target set [em <targetName>] %>

Sets the active target for the current build folder. This is the target that `plytool run` will run by default.

<% endMembers %>

## Extern Commands

<% member plytool extern list %>

Outputs a list of known externs as defined by the available repos.

<% member plytool extern selected %>

Outputs a list of extern providers selected in the current build folder.

<% member plytool extern select [em <providerName>] %>

Selects an extern provider in the current build folder.

<% member plytool extern install [em <providerName>] %>

Installs the specified extern provider.

<% endMembers %>

## Generate Commands

<% member plytool generate \[--config=[em <configName>\]%>

Generates a build system inside the current build folder by running CMake. When using a single-configuration generator, such as "Unix Makefiles", each configuration gets its own build system. For such generators, the optional `--config` argument can be used to override the active configuration for the current build folder.

<% member plytool build \[[em <targetName>]\] \[--config=[em <configName>]\] %>

Runs the build system inside the current build folder. This command will also generate a build system in the current build folder, but only if a `.modules.cpp` file has changed. If *targetName* is specified, only the specified target and its dependencies are built; otherwise, all targets in the build folder are built. If the `--config` option is specified, the specified configuration is built; otherwise, the folder's active configuration is built.

<% member plytool run \[[em <targetName>]\] \[--config=[em <configName>]\] \[--nobuild\] %>

Build and runs an executable target inside the current build folder. By default, this command will also generate a build system in the current build folder, but only if a `.modules.cpp` file has changed. If the `--nobuild` option is specified, the generate & build steps are skipped. If *targetName* is specified, the specified target is built & run; otherwise, the build folder's active target is built & run. If the `--config` option is specified, the specified configuration is built; otherwise, the build folder's active configuration is built.

<% member plytool codegen %>

Performs code generation across all repos.

<% member plytool bootstrap %>

Generate the bootstrap files necessary to set up Plywood on a new system. The generated files, relative to the workspace root, are:

* `repos/plywood/scripts/bootstrap_CMakeLists.txt`
* `repos/plywood/scripts/bootstrap_Config.txt`

<% endMembers %>
