﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Repository.h>
#include <ply-build-repo/Workspace.h>
#include <ply-biscuit/Interpreter.h>

namespace ply {
namespace build {

Repository* g_repository = nullptr;

struct ConfigOptionsInterpreter {
    biscuit::Interpreter interp;
    Repository::ConfigOptions* optionSet = nullptr;
};

bool doLocalAssignment(ConfigOptionsInterpreter* coi, const AnyObject& attributes,
                       Label label) {
    PLY_ASSERT(!attributes.data);
    AnyOwnedObject* obj = coi->optionSet->map.insert_or_find(label);
    *obj = AnyOwnedObject::create(coi->interp.base.returnValue.type);
    obj->move(coi->interp.base.returnValue);
    return true;
}

void Repository::create() {
    g_repository = new Repository;

    bool anyError = false;
    for (const FileInfo& entry : FileSystem.listDir(Workspace.path, 0)) {
        if (!entry.isDir)
            continue;
        if (entry.name.startsWith("."))
            continue;
        if (entry.name == "data")
            continue;

        // Recursively find all Plyfiles
        String repoFolder = Path.join(Workspace.path, entry.name);
        for (const WalkTriple& triple : FileSystem.walk(repoFolder)) {
            for (const FileInfo& file : triple.files) {
                if (file.name == "Plyfile") {
                    if (!parsePlyfile(Path.join(triple.dirPath, file.name))) {
                        anyError = true;
                    }
                }
            }
        }
    }

    if (anyError) {
        exit(1);
    }

    // Initialize all config_options
    for (Function* target : g_repository->targets) {
        target->defaultOptions = Owned<ConfigOptions>::create();
    }

    for (const TargetConfigBlock& cb : g_repository->targetConfigBlocks) {
        // Create new interpreter.
        ConfigOptionsInterpreter coi;
        coi.interp.base.error = [&coi](StringView message) {
            OutStream out = Console.error();
            logErrorWithStack(out, &coi.interp, message);
        };
        coi.optionSet = cb.target_func->defaultOptions;

        // Add builtin namespace.
        Map<Label, AnyObject> builtIns;
        static bool true_ = true;
        static bool false_ = false;
        builtIns.assign(g_labelStorage.insert("true"), AnyObject::bind(&true_));
        builtIns.assign(g_labelStorage.insert("false"), AnyObject::bind(&false_));
        coi.interp.resolveName = [&builtIns](Label identifier) -> AnyObject {
            if (AnyObject* builtIn = builtIns.find(identifier))
                return *builtIn;
            return {};
        };

        // Invoke config_options block.
        biscuit::Interpreter::StackFrame frame;
        frame.interp = &coi.interp;
        frame.hooks.assignToLocal = {doLocalAssignment, &coi};
        frame.desc = [tf = cb.target_func]() -> HybridString {
            return String::format("config_options for {} '{}'",
                                  g_labelStorage.view(tf->stmt->customBlock()->type),
                                  g_labelStorage.view(tf->stmt->customBlock()->name));
        };
        frame.tkr = &cb.target_func->plyfile->tkr;
        FnResult result = execFunction(&frame, cb.block->customBlock()->body);
        if (result == Fn_Error) {
            exit(1);
        }
    }
}

PLY_NO_INLINE MethodTable getMethodTable_Repository_ConfigOptions() {
    MethodTable methods;
    methods.propertyLookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                StringView propertyName) -> FnResult {
        Label label = g_labelStorage.find(propertyName);
        if (label) {
            auto* configOpts = obj.cast<Repository::ConfigOptions>();
            AnyOwnedObject* prop = configOpts->map.insert_or_find(label);
            interp->returnValue = *prop;
            return Fn_OK;
        }
        // read-only access?

        interp->returnValue = {};
        interp->error(String::format("configuration option '{}' not found in library",
                                     propertyName));
        return Fn_Error;
    };
    return methods;
}

TypeKey TypeKey_Repository_ConfigOptions{
    // getName
    [](const TypeDescriptor* typeDesc) -> HybridString { //
        return "ConfigOptions";
    },

    // hashDescriptor
    TypeKey::hashEmptyDescriptor,

    // equalDescriptors
    TypeKey::alwaysEqualDescriptors,
};

} // namespace build

PLY_DEFINE_TYPE_DESCRIPTOR(build::Repository::ConfigOptions) {
    static TypeDescriptor typeDesc{
        &build::TypeKey_Repository_ConfigOptions,
        (build::Repository::ConfigOptions*) nullptr,
        NativeBindings::make<build::Repository::ConfigOptions>()
            PLY_METHOD_TABLES_ONLY(, build::getMethodTable_Repository_ConfigOptions())};
    return &typeDesc;
}

} // namespace ply

#include "codegen/Repository.inl"
