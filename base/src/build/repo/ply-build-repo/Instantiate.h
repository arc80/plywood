/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-build-repo/Common.h>
#include <ply-build-steps/Project.h>
#include <ply-build-repo/Repository.h>
#include <ply-biscuit/Interpreter.h>
#include <ply-build-repo/BuildFolder.h>

namespace ply {
namespace build {

enum Status {
    NotInstantiated,
    Instantiating,
    Instantiated,
};

struct TargetWithStatus {
    Target* target = nullptr;
    Status statusInCurrentConfig = NotInstantiated;
};

struct TargetInstantiator {
    Map<Label, AnyObject> globalNamespace;

    // These members are modified during instantiation.
    Map<Label, TargetWithStatus> targetMap;
    u64 configBit = 0;
};

FnResult instantiateTargetForCurrentConfig(Target** target, TargetInstantiator* mi,
                                               Label name);

struct PropertyCollector {
    biscuit::Interpreter* interp;
    String basePath;
    Array<Option>* options;
    u64 configBit = 0;
    bool isTarget = false;
};

FnResult doCustomBlockInsideConfig(PropertyCollector* pc,
                                       const biscuit::Statement::CustomBlock* cb);
void instantiate_all_configs(BuildFolder_t* build_folder);

} // namespace build
} // namespace ply
