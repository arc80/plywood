/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <buildSteps/Project.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace build2 {

void inherit_option(Array<Option>& options, const Option& srcOpt, u64 enabledBits, u64 publicBits) {
    if ((srcOpt.enabled.bits & enabledBits) == 0)
        return;

    s32 i = find(options, [&](const Option& o) { return o == srcOpt; });
    if (i < 0) {
        i = options.numItems();
        options.append({srcOpt.type, srcOpt.key, srcOpt.value});
    }
    options[i].enabled.bits |= (srcOpt.enabled.bits & enabledBits);
    options[i].isPublic.bits |= (srcOpt.isPublic.bits & publicBits);
}

void append_option(Array<Option>& options, const Option& srcOpt) {
    bool wasFound = false;
    for (u32 i = 0; i < options.numItems(); i++) {
        Option& dstOpt = options[i];
        if ((dstOpt.type == srcOpt.type) && (dstOpt.key == srcOpt.key)) {
            if (dstOpt.value == srcOpt.value) {
                wasFound = true;
                dstOpt.enabled.bits |= srcOpt.enabled.bits;
                dstOpt.isPublic.bits |= srcOpt.isPublic.bits;
            } else {
                dstOpt.enabled.bits &= srcOpt.enabled.bits;
                dstOpt.isPublic.bits &= srcOpt.enabled.bits;
                if (dstOpt.enabled.bits == 0) {
                    options.erase(i);
                    i--;
                    continue;
                }
            }
        }
    }
    if (!wasFound) {
        options.append(srcOpt);
    }
}

void inherit_dependency(Array<Dependency>& dependencies, Target* srcTarget, u64 enabledBits) {
    if (enabledBits == 0)
        return;

    s32 i = find(dependencies, [&](const Dependency& o) { return o.target == srcTarget; });
    if (i < 0) {
        i = dependencies.numItems();
        dependencies.append({srcTarget, 0});
    }
    dependencies[i].enabled.bits |= enabledBits;
}

void do_inheritance(Target* target) {
    if (target->didInheritance)
        return;

    Array<Option> options;
    Array<Dependency> dependencies;

    // Inherit from config.
    options = Project.perConfigOptions;

    // Inherit from dependencies.
    for (const Dependency& dep : target->dependencies) {
        do_inheritance(dep.target);

        if (dep.target->type == Target::Executable)
            continue;

        // Inherit dependency's dependencies (for linker inputs).
        for (const Dependency& dep2 : dep.target->dependencies) {
            inherit_dependency(dependencies, dep2.target, dep.enabled.bits & dep2.enabled.bits);
        }
        inherit_dependency(dependencies, dep.target, dep.enabled.bits);

        // Inherit dependency's options.
        for (const Option& opt : dep.target->options) {
            inherit_option(options, opt, dep.enabled.bits, dep.isPublic.bits);
        }
    }

    // Inherit target options.
    for (const Option& opt : target->options) {
        inherit_option(options, opt, Limits<u64>::Max, Limits<u64>::Max);
    }

    // Done.
    target->options = std::move(options);
    target->dependencies = std::move(dependencies);
    target->didInheritance = true;
}

Project_ Project;

void do_inheritance() {
    PLY_ASSERT(Project.name);
    PLY_ASSERT(!Project.configNames.isEmpty());
    PLY_ASSERT(Project.configNames.numItems() < 64);
    PLY_ASSERT(!Project.didInheritance);

    for (Target* target : Project.targets) {
        do_inheritance(target);
    }

    Project.didInheritance = true;
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

} // namespace build2
} // namespace ply
