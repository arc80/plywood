/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-steps/Project.h>

namespace ply {
namespace build {

void inherit_option(Array<Option>& options, const Option& src_opt, u64 enabled_bits,
                    u64 public_bits) {
    if ((src_opt.enabled_bits & enabled_bits) == 0)
        return;

    s32 i = find(options, [&](const Option& o) { return o == src_opt; });
    if (i < 0) {
        i = options.num_items();
        options.append({src_opt.type, src_opt.key, src_opt.value});
    }
    options[i].enabled_bits |= (src_opt.enabled_bits & enabled_bits);
    options[i].is_public_bits |= (src_opt.is_public_bits & public_bits);
}

void append_option(Array<Option>& options, const Option& src_opt) {
    bool was_found = false;
    for (u32 i = 0; i < options.num_items(); i++) {
        Option& dst_opt = options[i];
        if ((dst_opt.type == src_opt.type) && (dst_opt.key == src_opt.key)) {
            if (dst_opt.value == src_opt.value) {
                was_found = true;
                dst_opt.enabled_bits |= src_opt.enabled_bits;
                dst_opt.is_public_bits |= src_opt.is_public_bits;
            } else {
                dst_opt.enabled_bits &= ~src_opt.enabled_bits;
                dst_opt.is_public_bits &= ~src_opt.enabled_bits;
                if (dst_opt.enabled_bits == 0) {
                    options.erase(i);
                    i--;
                    continue;
                }
            }
        }
    }
    if (!was_found) {
        options.append(src_opt);
    }
}

void inherit_dependency(Array<Dependency>& dependencies, Target* src_target,
                        u64 enabled_bits) {
    if (enabled_bits == 0)
        return;

    s32 i =
        find(dependencies, [&](const Dependency& o) { return o.target == src_target; });
    if (i < 0) {
        i = dependencies.num_items();
        dependencies.append({src_target, 0});
    }
    dependencies[i].enabled_bits |= enabled_bits;
}

void do_inheritance(Target* target) {
    if (target->did_inheritance)
        return;

    Array<Option> options;
    Array<Dependency> dependencies;

    // Inherit from config.
    options = Project.per_config_options;

    // Inherit from dependencies.
    for (const Dependency& dep : target->dependencies) {
        do_inheritance(dep.target);

        if (dep.target->type == Target::Executable)
            continue;

        // Inherit dependency's dependencies (for linker inputs).
        for (const Dependency& dep2 : dep.target->dependencies) {
            inherit_dependency(dependencies, dep2.target,
                               dep.enabled_bits & dep2.enabled_bits);
        }
        inherit_dependency(dependencies, dep.target, dep.enabled_bits);

        // Inherit dependency's options.
        for (const Option& opt : dep.target->options) {
            inherit_option(options, opt, dep.enabled_bits, dep.is_public_bits);
        }
    }

    // Inherit target options.
    for (const Option& opt : target->options) {
        inherit_option(options, opt, Limits<u64>::Max, Limits<u64>::Max);
    }

    // Done.
    target->options = std::move(options);
    target->dependencies = std::move(dependencies);
    target->did_inheritance = true;
}

Project_ Project;

void do_inheritance() {
    PLY_ASSERT(Project.name);
    PLY_ASSERT(!Project.config_names.is_empty());
    PLY_ASSERT(Project.config_names.num_items() < 64);
    PLY_ASSERT(!Project.did_inheritance);

    for (Target* target : Project.targets) {
        do_inheritance(target);
    }

    Project.did_inheritance = true;
}

Array<Option> get_combined_options() {
    Array<Option> result;
    for (Target* target : Project.targets) {
        for (const Option& opt : target->options) {
            append_option(result, opt);
        }
    }
    return result;
}

} // namespace build
} // namespace ply
