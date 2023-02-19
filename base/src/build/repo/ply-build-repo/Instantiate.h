/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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
    Status status_in_current_config = NotInstantiated;
};

struct TargetInstantiator {
    Map<Label, AnyObject> global_namespace;

    // These members are modified during instantiation.
    Map<Label, TargetWithStatus> target_map;
    u64 config_bit = 0;
    Array<Func<FnResult()>>* prebuild_steps = nullptr;
};

FnResult instantiate_target_for_current_config(Target** target, TargetInstantiator* mi,
                                               Label name);

struct PropertyCollector {
    biscuit::Interpreter* interp;
    String base_path;
    Array<Option>* options;
    u64 config_bit = 0;
    bool is_target = false;
};

FnResult do_custom_block_inside_config(PropertyCollector* pc,
                                       const biscuit::Statement::CustomBlock* cb);
void instantiate_all_configs(BuildFolder_t* build_folder,
                             StringView run_prebuild_step_for_config = {});

} // namespace build
} // namespace ply
