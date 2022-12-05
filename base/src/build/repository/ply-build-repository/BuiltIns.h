/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repository/Common.h>

namespace ply {
namespace build2 {

struct ReadOnlyDict {
    String name;
    LabelMap<AnyObject> map;
};

struct BuiltInStorage_ {
    bool true_ = true;
    bool false_ = false;
    String sys_target_platform;
    String sys_target_arch;
    String sys_build_folder;
    String sys_cmake_path;
    String script_path;
    ReadOnlyDict dict_build;
    ReadOnlyDict dict_sys;
    ReadOnlyDict dict_sys_fs;
};

extern BuiltInStorage_ BuiltInStorage;
extern LabelMap<AnyObject> BuiltInMap;

void init_built_ins();

} // namespace build2
} // namespace ply
