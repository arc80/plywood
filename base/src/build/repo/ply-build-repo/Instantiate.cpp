/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Instantiate.h>
#include <ply-build-repo/BuiltIns.h>
#include <ply-build-repo/BuildFolder.h>

namespace ply {
namespace build {

//--------------------------------------------------------------

template <typename T, typename U, typename Callable>
T& append_or_find(Array<T>& arr, U&& item, const Callable& callable) {
    for (u32 i = 0; i < arr.num_items(); i++) {
        if (callable(arr[i]))
            return arr[i];
    }
    return arr.append(U{std::forward<U>(item)});
}

struct InstantiatingInterpreter {
    biscuit::Interpreter interp;
    TargetInstantiator* ti = nullptr;
    Repository::Function* target_func = nullptr;
    Target* target = nullptr;

    String make_abs_path(StringView rel_path) {
        StringView plyfile_path =
            Path.split(this->target_func->plyfile->tkr.file_location_map.path).first;
        return Path.join(plyfile_path, rel_path);
    }
};

enum class Visibility {
    Error,
    Public,
    Private,
};

Visibility get_visibility(biscuit::Interpreter* interp, const AnyObject& attributes,
                          bool is_target, StringView property_type) {
    Visibility vis = Visibility::Private;
    s32 token_idx = -1;
    if (const StatementAttributes* sa = attributes.cast<StatementAttributes>()) {
        token_idx = sa->visibility_token_idx;
        if (sa->is_public) {
            vis = Visibility::Public;
        }
    }
    if (is_target) {
        if (token_idx < 0) {
            interp->base.error(String::format(
                "{} must have 'public' or 'private' attribute", property_type));
            return Visibility::Error;
        }
    } else {
        if (token_idx >= 0) {
            biscuit::ExpandedToken token =
                interp->current_frame->tkr->expand_token(token_idx);
            interp->base.error(
                String::format("'{}' cannot be used inside config block", token.text));
            return Visibility::Error;
        }
    }
    return vis;
}

bool assign_to_compile_options(PropertyCollector* pc, const AnyObject& attributes,
                               Label label) {
    Visibility vis =
        get_visibility(pc->interp, attributes, pc->is_target, "compile options");
    if (vis == Visibility::Error)
        return false;

    Option opt{Option::Generic, g_labelStorage.view(label),
               *pc->interp->base.return_value.cast<String>()};
    opt.enabled_bits |= pc->config_bit;
    if (vis == Visibility::Public) {
        opt.is_public_bits |= pc->config_bit;
    }
    append_option(*pc->options, opt);
    return true;
}

bool on_evaluate_source_file(InstantiatingInterpreter* ii,
                             const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String abs_path = ii->make_abs_path(*ii->interp.base.return_value.cast<String>());
    SourceGroup& src_group =
        append_or_find(ii->target->source_groups, abs_path,
                       [&](const auto& a) { return a.abs_path == abs_path; });
    bool needs_compilation = false;
    for (WalkTriple& triple : FileSystem.walk(abs_path, 0)) {
        for (const FileInfo& file : triple.files) {
            if (file.name.ends_with(".cpp") || file.name.ends_with(".h")) {
                if (!needs_compilation && file.name.ends_with(".cpp")) {
                    needs_compilation = true;
                }
                String rel_path =
                    Path.make_relative(abs_path, Path.join(triple.dir_path, file.name));
                SourceFile& src_file = append_or_find(
                    src_group.files, std::move(rel_path),
                    [&](const SourceFile& a) { return a.rel_path == rel_path; });
                src_file.enabled_bits |= ii->ti->config_bit;
            }
        }
    }
    if (needs_compilation) {
        ii->target->has_build_step_bits |= ii->ti->config_bit;
    }
    return true;
}

bool on_evaluate_include_directory(PropertyCollector* pc, const AnyObject& attributes) {
    Visibility vis =
        get_visibility(pc->interp, attributes, pc->is_target, "include directory");
    if (vis == Visibility::Error)
        return false;

    Option opt{Option::IncludeDir,
               Path.join(pc->base_path, *pc->interp->base.return_value.cast<String>())};
    Option& found_opt = append_or_find(*pc->options, std::move(opt),
                                       [&](const Option& o) { return o == opt; });
    found_opt.enabled_bits |= pc->config_bit;
    if (vis == Visibility::Public) {
        found_opt.is_public_bits |= pc->config_bit;
    }
    return true;
}

bool on_evaluate_preprocessor_definition(PropertyCollector* pc,
                                         const AnyObject& attributes) {
    Visibility vis = get_visibility(pc->interp, attributes, pc->is_target,
                                    "preprocessor definition");
    if (vis == Visibility::Error)
        return false;

    String key = *pc->interp->base.return_value.cast<String>();
    String value;
    s32 i = key.find_byte('=');
    if (i >= 0) {
        value = key.sub_str(i + 1);
        key = key.left(i);
    }

    Option opt{Option::PreprocessorDef, key, value};
    Option& found_opt =
        append_or_find(*pc->options, std::move(opt), [&](const Option& o) {
            return (o.type == Option::PreprocessorDef) && (o.key == opt.key);
        });
    found_opt.enabled_bits |= pc->config_bit;
    if (vis == Visibility::Public) {
        found_opt.is_public_bits |= pc->config_bit;
    }
    return true;
}

bool on_evaluate_dependency(InstantiatingInterpreter* ii, const AnyObject& attributes) {
    Visibility vis = get_visibility(&ii->interp, attributes, true, "dependency");
    if (vis == Visibility::Error)
        return false;

    // Instantiate the dependency
    Repository::Function* target =
        ii->interp.base.return_value.cast<Repository::Function>();
    Target* dep_target = nullptr;
    if (instantiate_target_for_current_config(
            &dep_target, ii->ti, target->stmt->custom_block()->name) != Fn_OK)
        return false;
    Dependency& found_dep =
        append_or_find(ii->target->dependencies, dep_target,
                       [&](const Dependency& d) { return d.target == dep_target; });
    found_dep.enabled_bits |= ii->ti->config_bit;
    if (vis == Visibility::Public) {
        found_dep.is_public_bits |= ii->ti->config_bit;
    }
    return true;
}

bool on_evaluate_link_library(PropertyCollector* pc, const AnyObject& attributes) {
    PLY_ASSERT(!attributes.data);
    String* path = pc->interp->base.return_value.cast<String>();
    Option desired_opt{Option::LinkerInput, *path, {}};
    Option& opt = append_or_find(*pc->options, desired_opt,
                                 [&](const Option& o) { return o == desired_opt; });
    opt.enabled_bits |= pc->config_bit;
    return true;
}

FnResult custom_block_inside_config(PropertyCollector* pc,
                                    const biscuit::Statement::CustomBlock* cb) {
    biscuit::Interpreter::Hooks hooks;
    if (cb->type == g_common->include_directories_key) {
        hooks.on_evaluate = {on_evaluate_include_directory, pc};
    } else if (cb->type == g_common->preprocessor_definitions_key) {
        hooks.on_evaluate = {on_evaluate_preprocessor_definition, pc};
    } else if (cb->type == g_common->compile_options_key) {
        hooks.assign_to_local = {assign_to_compile_options, pc};
    } else if (cb->type == g_common->link_libraries_key) {
        hooks.on_evaluate = {on_evaluate_link_library, pc};
    } else {
        // FIXME: Make this a runtime error instead of an assert because the config
        // block can call a function that contains, for example, a dependencies {} block
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(pc->interp->current_frame->hooks, std::move(hooks));
    return exec_block(pc->interp->current_frame, cb->body);
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  Prebuild steps  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
struct PrebuildStepInterpreter {
    biscuit::Interpreter interp;
};

FnResult preprocess_labels(const FnParams& params);

FnResult do_prebuild_steps(const biscuit::Statement::CustomBlock* cb,
                           biscuit::Tokenizer* tkr, Target* target) {
    // Create new interpreter.
    PrebuildStepInterpreter psi;
    psi.interp.base.error = [&psi](StringView message) {
        OutStream out = Console.error();
        log_error_with_stack(out, &psi.interp, message);
    };

    // Populate global & library namespaces.
    psi.interp.resolve_name = [&psi](Label identifier) -> AnyObject {
        if (AnyObject* built_in = BuiltInMap.find(identifier))
            return *built_in;
        if (identifier == g_common->preprocess_labels_key)
            return AnyObject::bind(&preprocess_labels);
        return {};
    };

    // Invoke library function.
    biscuit::Interpreter::StackFrame frame;
    // frame.hooks.do_custom_block = {custom_block_inside_target_function, &ii};
    frame.interp = &psi.interp;
    frame.desc = [target]() -> HybridString {
        return String::format("prebuild step for '{}'",
                              g_labelStorage.view(target->name));
    };
    frame.tkr = tkr;
    FnResult result = exec_function(&frame, cb->body);
    return result;
}

FnResult
custom_block_inside_target_function(InstantiatingInterpreter* ii,
                                    const biscuit::Statement::CustomBlock* cb) {
    if (cb->type == g_common->prebuild_step_key) {
        // When a 'crowbar prebuild' command is issued, we add each prebuild_step block
        // to an array that gets executed after the build system is fully instantiated.
        if (ii->ti->prebuild_steps) {
            ii->ti->prebuild_steps->append(
                [cb, tkr = ii->interp.current_frame->tkr, targ = ii->target]() {
                    return do_prebuild_steps(cb, tkr, targ);
                });
        }
        return Fn_OK;
    }

    PropertyCollector pc;
    pc.interp = &ii->interp;
    pc.base_path =
        Path.split(ii->target_func->plyfile->tkr.file_location_map.path).first;
    pc.options = &ii->target->options;
    pc.config_bit = ii->ti->config_bit;
    pc.is_target = true;

    biscuit::Interpreter::Hooks hooks;
    if (cb->type == g_common->source_files_key) {
        hooks.on_evaluate = {on_evaluate_source_file, ii};
    } else if (cb->type == g_common->include_directories_key) {
        hooks.on_evaluate = {on_evaluate_include_directory, &pc};
    } else if (cb->type == g_common->preprocessor_definitions_key) {
        hooks.on_evaluate = {on_evaluate_preprocessor_definition, &pc};
    } else if (cb->type == g_common->compile_options_key) {
        hooks.assign_to_local = {assign_to_compile_options, &pc};
    } else if (cb->type == g_common->link_libraries_key) {
        hooks.on_evaluate = {on_evaluate_link_library, &pc};
    } else if (cb->type == g_common->dependencies_key) {
        hooks.on_evaluate = {on_evaluate_dependency, ii};
    } else {
        PLY_ASSERT(0); // Shouldn't get here
    }
    PLY_SET_IN_SCOPE(ii->interp.current_frame->custom_block, cb);
    PLY_SET_IN_SCOPE(ii->interp.current_frame->hooks, std::move(hooks));
    return exec_block(ii->interp.current_frame, cb->body);
}

FnResult run_generate_block(Repository::Function* target) {
    // Create new interpreter.
    biscuit::Interpreter interp;
    interp.base.error = [&interp](StringView message) {
        OutStream out = Console.error();
        log_error_with_stack(out, &interp, message);
    };

    // Populate dictionaries.
    BuiltInStorage.sys_cmake_path = PLY_CMAKE_PATH;
    BuiltInStorage.script_path = target->plyfile->tkr.file_location_map.path;
    interp.resolve_name = [](Label identifier) -> AnyObject {
        if (AnyObject* built_in = BuiltInMap.find(identifier))
            return *built_in;
        return {};
    };

    // Invoke generate block.
    biscuit::Interpreter::StackFrame frame;
    frame.interp = &interp;
    frame.desc = [target]() -> HybridString {
        return String::format("library '{}'",
                              g_labelStorage.view(target->stmt->custom_block()->name));
    };
    frame.tkr = &target->plyfile->tkr;
    interp.current_frame = &frame;
    return exec_block(&frame, target->generate_block->custom_block()->body);
}

FnResult fn_link_objects_directly(InstantiatingInterpreter* ii,
                                  const FnParams& params) {
    if (params.args.num_items != 0) {
        params.base->error(
            String::format("'link_objects_directly' takes no arguments"));
        return Fn_Error;
    }

    if (ii->target->type == Target::Executable) {
        params.base->error("Target must be a library");
        return Fn_Error;
    }

    ii->target->type = Target::ObjectLibrary;

    return Fn_OK;
}

FnResult instantiate_target_for_current_config(Target** out_target,
                                               TargetInstantiator* ti, Label name) {
    // Check for an existing target; otherwise create one.
    Target* target = nullptr;
    {
        bool was_found = false;
        TargetWithStatus* tws = ti->target_map.insert_or_find(name, &was_found);
        if (!was_found) {
            // No existing target found. Create a new one.
            tws->target = new Target;
            tws->target->name = name;
            Project.targets.append(tws->target);
        } else {
            // Yes. If the library was already fully instantiated in this config, return
            // it.
            if (tws->status_in_current_config == Instantiated) {
                *out_target = tws->target;
                return Fn_OK;
            }
            // Circular dependency check. FIXME: Handle gracefully
            if (tws->status_in_current_config == Instantiating) {
                PLY_ASSERT(0);
            }
            PLY_ASSERT(tws->status_in_current_config == NotInstantiated);
        }
        // Set this library's status as Instantiating so that circular dependencies can
        // be detected.
        tws->status_in_current_config = Instantiating;
        target = tws->target;
        *out_target = tws->target;
    }

    // Set node as active in this config.
    PLY_ASSERT(ti->config_bit);
    target->enabled_bits |= ti->config_bit;

    // Find library function by name.
    Repository::Function** func_ = g_repository->global_scope.find(name);
    if (!func_ || !(*func_)->stmt->custom_block()) {
        PLY_FORCE_CRASH(); // FIXME: Handle gracefully
    }
    Repository::Function* target_func = *func_;

    // Run the generate block if it didn't run already.
    if (!target_func->generated_once) {
        if (target_func->generate_block) {
            FnResult result = run_generate_block(target_func);
            if (result != Fn_OK)
                return result;
        }
        target_func->generated_once = true;
    }

    const biscuit::Statement::CustomBlock* target_def =
        target_func->stmt->custom_block().get();
    if (target_def->type == g_common->executable_key) {
        target->type = Target::Executable;
    } else {
        PLY_ASSERT(target_def->type == g_common->library_key);
        PLY_ASSERT(target->type == Target::Library ||
                   target->type == Target::ObjectLibrary);
    }

    // Create new interpreter.
    InstantiatingInterpreter ii;
    ii.interp.base.error = [&ii](StringView message) {
        OutStream out = Console.error();
        log_error_with_stack(out, &ii.interp, message);
    };
    ii.ti = ti;
    ii.target_func = target_func;
    ii.target = target;

    // Populate global & library namespaces.
    ii.interp.resolve_name = [&ii](Label identifier) -> AnyObject {
        if (AnyObject* built_in = BuiltInMap.find(identifier))
            return *built_in;
        if (AnyObject* obj = ii.target_func->current_options->map.find(identifier))
            return *obj;
        if (identifier == g_common->link_objects_directly_key) {
            AnyObject* obj = ii.interp.base.local_variable_storage.append_object(
                get_type_descriptor<BoundNativeMethod>());
            *obj->cast<BoundNativeMethod>() = {&ii, fn_link_objects_directly};
            return *obj;
        }
        if (Repository::Function** target =
                g_repository->global_scope.find(identifier)) {
            if (auto fn_def = (*target)->stmt->function_definition())
                return AnyObject::bind(fn_def.get());
            else
                return AnyObject::bind(*target);
        }
        return {};
    };

    // Invoke library function.
    biscuit::Interpreter::StackFrame frame;
    frame.hooks.do_custom_block = {custom_block_inside_target_function, &ii};
    frame.interp = &ii.interp;
    frame.desc = [target_def]() -> HybridString {
        return String::format("library '{}'", g_labelStorage.view(target_def->name));
    };
    frame.tkr = &target_func->plyfile->tkr;
    FnResult result = exec_function(&frame, target_def->body);
    ti->target_map.find(name)->status_in_current_config = Instantiated;
    return result;
}

struct ConfigListInterpreter {
    biscuit::Interpreter interp;
    BuildFolder_t* build_folder = nullptr;
    TargetInstantiator* ti = nullptr;
    String run_prebuild_step_for_config;
};

FnResult custom_block_inside_config_list(ConfigListInterpreter* cli,
                                         const biscuit::Statement::CustomBlock* cb) {
    PLY_ASSERT(cb->type == g_common->config_key);

    // Evaluate config name
    FnResult result = eval(cli->interp.current_frame, cb->expr);
    PLY_ASSERT(result == Fn_OK); // FIXME: Make robust
    String current_config_name = *cli->interp.base.return_value.cast<String>();
    PLY_ASSERT(current_config_name); // FIXME: Make robust

    // Initialize all Target::current_options
    for (Repository::Function* target : g_repository->targets) {
        auto new_options = Owned<Repository::ConfigOptions>::create();
        for (const auto& item : target->default_options->map) {
            AnyOwnedObject* dst = new_options->map.insert_or_find(item.key);
            *dst = AnyOwnedObject::create(item.value.type);
            dst->copy(item.value);
        }
        target->current_options = std::move(new_options);
    }

    // By default, enable debug info and disable optimization
    u32 config_index = Project.config_names.num_items();
    PLY_ASSERT(config_index < 64); // FIXME: Handle elegantly
    {
        Option debug_info{Option::Generic, "debug_info", "true"};
        debug_info.enabled_bits |= u64{1} << config_index;
        append_option(Project.per_config_options, debug_info);
        Option optim{Option::Generic, "optimization", "none"};
        optim.enabled_bits |= u64{1} << config_index;
        append_option(Project.per_config_options, optim);
    }

    // Execute config block
    PropertyCollector pc;
    pc.interp = &cli->interp;
    pc.base_path =
        Path.split(cli->interp.current_frame->tkr->file_location_map.path).first;
    pc.options = &Project.per_config_options;
    pc.config_bit = u64{1} << config_index;
    biscuit::Interpreter::Hooks hooks;
    hooks.do_custom_block = {custom_block_inside_config, &pc};
    PLY_SET_IN_SCOPE(cli->interp.current_frame->hooks, std::move(hooks));
    result = exec_block(cli->interp.current_frame, cb->body);
    if (result != Fn_OK)
        return result;

    // Add config to project
    Project.config_names.append(current_config_name);

    // If we're generating a build system, instantiate targets in every config. If we're
    // executing a prebuild step, only instantiate targets in the desired config.
    if (cli->run_prebuild_step_for_config.is_empty()) {
        // We're generating a build system. Instantiate all root targets in this config.
        PLY_SET_IN_SCOPE(cli->ti->config_bit, pc.config_bit);
        for (StringView target_name : cli->build_folder->root_targets) {
            Target* root_target = nullptr;
            FnResult result = instantiate_target_for_current_config(
                &root_target, cli->ti, g_labelStorage.insert(target_name));
            if (result != Fn_OK)
                return result;
        }
    } else if (cli->run_prebuild_step_for_config == current_config_name) {
        // We're executing a prebuild step and this is the desired config. Instantiate
        // all root targets and gather their prebuild steps.
        PLY_SET_IN_SCOPE(cli->ti->config_bit, pc.config_bit);
        Array<Func<FnResult()>> prebuild_steps;
        PLY_SET_IN_SCOPE(cli->ti->prebuild_steps, &prebuild_steps);
        for (StringView target_name : cli->build_folder->root_targets) {
            Target* root_target = nullptr;
            FnResult result = instantiate_target_for_current_config(
                &root_target, cli->ti, g_labelStorage.insert(target_name));
            if (result != Fn_OK)
                return result;
        }

        // Execute all prebuild steps for this config.
        for (const Func<FnResult()>& step : prebuild_steps) {
            if (step() != Fn_OK) {
                exit(1);
            }
        }
    }

    // Nothing left to do for this config. Reset all configuration options and
    // instantiation statuses.
    for (Repository::Function* target : g_repository->targets) {
        target->current_options.clear();
    }
    for (auto& item : cli->ti->target_map) {
        item.value.status_in_current_config = NotInstantiated;
    }

    return Fn_OK;
}

void instantiate_all_configs(BuildFolder_t* build_folder,
                             StringView run_prebuild_step_for_config) {
    TargetInstantiator ti{};
    Project.name = build_folder->solution_name;
    init_toolchain_msvc();

    // Execute the config_list block
    Repository::ConfigList* config_list = g_repository->config_list;
    if (!config_list) {
        Error.log("No config_list block defined.\n");
    }

    {
        // Create new interpreter.
        ConfigListInterpreter cli;
        cli.interp.base.error = [&cli](StringView message) {
            OutStream out = Console.error();
            log_error_with_stack(out, &cli.interp, message);
        };
        cli.ti = &ti;
        cli.run_prebuild_step_for_config = run_prebuild_step_for_config;

        // Add builtin namespace.
        Map<Label, AnyObject> built_ins;
        bool true_ = true;
        bool false_ = false;
        built_ins.assign(g_labelStorage.insert("true"), AnyObject::bind(&true_));
        built_ins.assign(g_labelStorage.insert("false"), AnyObject::bind(&false_));
        cli.build_folder = build_folder;
        cli.interp.resolve_name = [&built_ins, &cli](Label identifier) -> AnyObject {
            if (AnyObject* built_in = built_ins.find(identifier))
                return *built_in;
            if (Repository::Function** target =
                    g_repository->global_scope.find(identifier)) {
                if (auto fn_def = (*target)->stmt->function_definition())
                    return AnyObject::bind(fn_def.get());
                else
                    // FIXME: Don't resolve library names outside config {} block
                    return AnyObject::bind((*target)->current_options.get());
            }
            return {};
        };

        // Invoke block.
        biscuit::Interpreter::StackFrame frame;
        frame.interp = &cli.interp;
        frame.desc = []() -> HybridString { return "config_list"; };
        frame.tkr = &config_list->plyfile->tkr;
        frame.hooks.do_custom_block = {custom_block_inside_config_list, &cli};
        FnResult result =
            exec_function(&frame, config_list->block_stmt->custom_block()->body);
        if (result == Fn_Error) {
            exit(1);
        }
    }

    do_inheritance();
}

} // namespace build
} // namespace ply
