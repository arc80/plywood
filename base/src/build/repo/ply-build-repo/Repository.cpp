/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
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
    Repository::ConfigOptions* option_set = nullptr;
};

bool do_local_assignment(ConfigOptionsInterpreter* coi, const AnyObject& attributes,
                         Label label) {
    PLY_ASSERT(!attributes.data);
    AnyOwnedObject* obj = coi->option_set->map.insert_or_find(label);
    *obj = AnyOwnedObject::create(coi->interp.base.return_value.type);
    obj->move(coi->interp.base.return_value);
    return true;
}

void Repository::create() {
    g_repository = new Repository;

    bool any_error = false;
    for (const FileInfo& entry : FileSystem.list_dir(Workspace.path, 0)) {
        if (!entry.is_dir)
            continue;
        if (entry.name.starts_with("."))
            continue;
        if (entry.name == "data")
            continue;

        // Recursively find all Plyfiles
        String repo_folder = Path.join(Workspace.path, entry.name);
        for (const WalkTriple& triple : FileSystem.walk(repo_folder)) {
            for (const FileInfo& file : triple.files) {
                if (file.name == "Plyfile") {
                    if (!parse_plyfile(Path.join(triple.dir_path, file.name))) {
                        any_error = true;
                    }
                }
            }
        }
    }

    if (any_error) {
        exit(1);
    }

    // Initialize all config_options
    for (Function* target : g_repository->targets) {
        target->default_options = Owned<ConfigOptions>::create();
    }

    for (const TargetConfigBlock& cb : g_repository->target_config_blocks) {
        // Create new interpreter.
        ConfigOptionsInterpreter coi;
        coi.interp.base.error = [&coi](StringView message) {
            OutStream out = Console.error();
            log_error_with_stack(out, &coi.interp, message);
        };
        coi.option_set = cb.target_func->default_options;

        // Add builtin namespace.
        Map<Label, AnyObject> built_ins;
        static bool true_ = true;
        static bool false_ = false;
        built_ins.assign(g_labelStorage.insert("true"), AnyObject::bind(&true_));
        built_ins.assign(g_labelStorage.insert("false"), AnyObject::bind(&false_));
        coi.interp.resolve_name = [&built_ins](Label identifier) -> AnyObject {
            if (AnyObject* built_in = built_ins.find(identifier))
                return *built_in;
            return {};
        };

        // Invoke config_options block.
        biscuit::Interpreter::StackFrame frame;
        frame.interp = &coi.interp;
        frame.hooks.assign_to_local = {do_local_assignment, &coi};
        frame.desc = [tf = cb.target_func]() -> HybridString {
            return String::format("config_options for {} '{}'",
                                  g_labelStorage.view(tf->stmt->custom_block()->type),
                                  g_labelStorage.view(tf->stmt->custom_block()->name));
        };
        frame.tkr = &cb.target_func->plyfile->tkr;
        FnResult result = exec_function(&frame, cb.block->custom_block()->body);
        if (result == Fn_Error) {
            exit(1);
        }
    }
}

PLY_NO_INLINE MethodTable get_method_table_repository_config_options() {
    MethodTable methods;
    methods.property_lookup = [](BaseInterpreter* interp, const AnyObject& obj,
                                 StringView property_name) -> FnResult {
        Label label = g_labelStorage.find(property_name);
        if (label) {
            auto* config_opts = obj.cast<Repository::ConfigOptions>();
            AnyOwnedObject* prop = config_opts->map.insert_or_find(label);
            interp->return_value = *prop;
            return Fn_OK;
        }
        // read-only access?

        interp->return_value = {};
        interp->error(String::format("configuration option '{}' not found in library",
                                     property_name));
        return Fn_Error;
    };
    return methods;
}

TypeKey TypeKey_Repository_ConfigOptions{
    // get_name
    [](const TypeDescriptor* type_desc) -> HybridString { //
        return "ConfigOptions";
    },

    // hash_descriptor
    TypeKey::hash_empty_descriptor,

    // equal_descriptors
    TypeKey::always_equal_descriptors,
};

} // namespace build

PLY_DEFINE_TYPE_DESCRIPTOR(build::Repository::ConfigOptions) {
    static TypeDescriptor type_desc{
        &build::TypeKey_Repository_ConfigOptions,
        (build::Repository::ConfigOptions*) nullptr,
        NativeBindings::make<build::Repository::ConfigOptions>() PLY_METHOD_TABLES_ONLY(
            , build::get_method_table_repository_config_options())};
    return &type_desc;
}

} // namespace ply

#include "codegen/Repository.inl"
