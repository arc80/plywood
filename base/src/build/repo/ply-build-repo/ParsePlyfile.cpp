/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-build-repo/Repository.h>
#include <ply-biscuit/Parser.h>

namespace ply {
namespace build {

struct ExtendedParser {
    biscuit::Parser* parser = nullptr;
    Repository::Plyfile* current_plyfile = nullptr;
    Repository::Function* target_func = nullptr;
};

Owned<biscuit::Statement> parse_custom_block(ExtendedParser* ep,
                                             const biscuit::Parser::Filter& filter,
                                             Label type, StringView trailing = {}) {
    auto custom_block = Owned<biscuit::Statement>::create();
    auto* cb = custom_block->custom_block().switch_to().get();
    cb->type = type;
    PLY_SET_IN_SCOPE(ep->parser->outer_scope, custom_block);
    PLY_SET_IN_SCOPE(ep->parser->filter, filter);
    StringView type_str = g_labelStorage.view(type);
    cb->body = parse_statement_block(ep->parser,
                                     {type_str, trailing ? trailing : type_str, true});
    return custom_block;
}

biscuit::KeywordResult
handle_keyword_inside_target_or_function(ExtendedParser* ep,
                                         const biscuit::KeywordParams& kp);
biscuit::KeywordResult
handle_keyword_inside_config_list(ExtendedParser* ep, const biscuit::KeywordParams& kp);
biscuit::KeywordResult handle_keyword_public_private(ExtendedParser* ep,
                                                     const biscuit::KeywordParams& kp);

biscuit::KeywordResult handle_keyword_at_file_scope(ExtendedParser* ep,
                                                    const biscuit::KeywordParams& kp) {
    if ((kp.kw_token.label == LABEL_library) ||
        (kp.kw_token.label == LABEL_executable)) {
        // Create new Repository::Target
        auto target = Owned<Repository::Function>::create();
        target->plyfile = ep->current_plyfile;
        target->stmt = Owned<biscuit::Statement>::create();
        auto cb = target->stmt->custom_block().switch_to();
        cb->type = kp.kw_token.label;

        // Parse library name.
        biscuit::ExpandedToken name_token = ep->parser->tkr->read_token();
        if (name_token.type == biscuit::TokenType::Identifier) {
            cb->name = name_token.label;
        } else {
            error_at_token(ep->parser, name_token, biscuit::ErrorTokenAction::PushBack,
                           String::format("expected {} name after '{}'; got {}",
                                          kp.kw_token.text, kp.kw_token.text,
                                          name_token.desc()));
        }

        // Add to Repository
        bool was_found = false;
        Repository::Function** prev_target =
            g_repository->global_scope.insert_or_find(cb->name, &was_found);
        if (was_found) {
            StringView type = "function";
            if (auto prev_cb = (*prev_target)->stmt->custom_block()) {
                type = g_labelStorage.view(prev_cb->type);
            }
            error_at_token(ep->parser, kp.kw_token,
                           biscuit::ErrorTokenAction::DoNothing,
                           String::format("'{}' was already defined as {}",
                                          kp.kw_token.text, type));
            ep->parser->recovery.mute_errors = false;
            ep->parser->error(String::format(
                "{}: ... see previous definition\n",
                (*prev_target)
                    ->plyfile->tkr.file_location_map.format_file_location(
                        (*prev_target)->stmt->file_offset)));
        }
        (*prev_target) = target;

        PLY_SET_IN_SCOPE(ep->target_func, target);
        biscuit::Parser::Filter filter;
        filter.keyword_handler = {handle_keyword_inside_target_or_function, ep};
        filter.allow_instructions = true;
        target->stmt->custom_block()->body =
            std::move(parse_custom_block(ep, filter, kp.kw_token.label,
                                         kp.kw_token.text + " name")
                          ->custom_block()
                          ->body);
        g_repository->targets.append(std::move(target));
        return biscuit::KeywordResult::Block;
    } else if (kp.kw_token.label == LABEL_config_list) {
        Repository::ConfigList* config_list = g_repository->config_list;
        if (config_list) {
            error_at_token(ep->parser, kp.kw_token,
                           biscuit::ErrorTokenAction::DoNothing,
                           "a config_list was already defined");
            ep->parser->recovery.mute_errors = false;
            ep->parser->error(String::format(
                "{}: see previous definition\n",
                config_list->plyfile->tkr.file_location_map.format_file_location(
                    config_list->file_offset)));
        }

        g_repository->config_list = Owned<Repository::ConfigList>::create();
        config_list = g_repository->config_list;
        config_list->plyfile = ep->current_plyfile;
        config_list->file_offset = kp.kw_token.file_offset;

        biscuit::Parser::Filter filter;
        filter.keyword_handler = {handle_keyword_inside_config_list, ep};
        filter.allow_instructions = true;
        config_list->block_stmt = parse_custom_block(ep, filter, kp.kw_token.label);
        return biscuit::KeywordResult::Block;
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult
handle_keyword_inside_target_or_function(ExtendedParser* ep,
                                         const biscuit::KeywordParams& kp) {
    if ((kp.kw_token.label == LABEL_source_files) ||
        (kp.kw_token.label == LABEL_include_directories) ||
        (kp.kw_token.label == LABEL_preprocessor_definitions) ||
        (kp.kw_token.label == LABEL_dependencies) ||
        (kp.kw_token.label == LABEL_link_libraries) ||
        (kp.kw_token.label == LABEL_prebuild_step) ||
        (kp.kw_token.label == LABEL_compile_options)) {
        biscuit::Parser::Filter filter;
        if (kp.kw_token.label == LABEL_prebuild_step) {
            filter.keyword_handler = [](const biscuit::KeywordParams&) {
                return biscuit::KeywordResult::Illegal;
            };
        } else {
            filter.keyword_handler = {handle_keyword_public_private, ep};
        }
        filter.allow_instructions = true;
        kp.stmt_block->statements.append(
            parse_custom_block(ep, filter, kp.kw_token.label));
        return biscuit::KeywordResult::Block;
    } else if (kp.kw_token.label == LABEL_config_options) {
        if (ep->target_func) {
            biscuit::Parser::Filter filter;
            filter.keyword_handler = [](const biscuit::KeywordParams&) {
                return biscuit::KeywordResult::Illegal;
            };
            filter.allow_instructions = true;
            Owned<biscuit::Statement> custom_block =
                parse_custom_block(ep, filter, kp.kw_token.label);
            g_repository->target_config_blocks.append(
                {ep->target_func, std::move(custom_block)});
            return biscuit::KeywordResult::Block;
        }
    } else if (kp.kw_token.label == LABEL_generate) {
        if (ep->target_func) {
            biscuit::Parser::Filter filter;
            filter.keyword_handler = [](const biscuit::KeywordParams&) {
                return biscuit::KeywordResult::Illegal;
            };
            filter.allow_instructions = true;
            ep->target_func->generate_block =
                parse_custom_block(ep, filter, kp.kw_token.label);
            return biscuit::KeywordResult::Block;
        }
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult
handle_keyword_inside_config_list(ExtendedParser* ep,
                                  const biscuit::KeywordParams& kp) {
    if (kp.kw_token.label == LABEL_config) {
        PLY_ASSERT(ep->parser->outer_scope->custom_block()->type ==
                   LABEL_config_list);

        Owned<biscuit::Expression> expr = ep->parser->parse_expression();

        biscuit::Parser::Filter filter;
        filter.keyword_handler = {handle_keyword_inside_target_or_function, ep};
        filter.allow_instructions = true;
        Owned<biscuit::Statement> cb =
            parse_custom_block(ep, filter, kp.kw_token.label);
        cb->custom_block()->expr = std::move(expr);
        kp.stmt_block->statements.append(std::move(cb));
        return biscuit::KeywordResult::Block;
    }

    return biscuit::KeywordResult::Illegal;
}

biscuit::KeywordResult handle_keyword_public_private(ExtendedParser* ep,
                                                     const biscuit::KeywordParams& kp) {
    auto cb = ep->parser->outer_scope->custom_block();

    bool is_legal = (cb->type == LABEL_include_directories) ||
                    (cb->type == LABEL_preprocessor_definitions) ||
                    (cb->type == LABEL_dependencies) ||
                    (cb->type == LABEL_compile_options);
    is_legal = is_legal && ((kp.kw_token.label == LABEL_public) ||
                            (kp.kw_token.label == LABEL_private));
    if (is_legal) {
        if (!kp.attributes->data) {
            *kp.attributes = AnyOwnedObject::create<StatementAttributes>();
        }
        auto traits = kp.attributes->cast<StatementAttributes>();
        traits->visibility_token_idx = kp.kw_token.token_idx;
        traits->is_public = (kp.kw_token.label == LABEL_public);
        return biscuit::KeywordResult::Attribute;
    }

    return biscuit::KeywordResult::Illegal;
}

void handle_plyfile_function(ExtendedParser* ep, Owned<biscuit::Statement>&& stmt,
                             const biscuit::ExpandedToken& name_token) {
    bool accept = true;
    auto fn_def = stmt->function_definition();
    Repository::Function** prev_target = g_repository->global_scope.find(fn_def->name);
    if (prev_target) {
        StringView type = "function";
        if (auto prev_cb = (*prev_target)->stmt->custom_block()) {
            type = g_labelStorage.view(prev_cb->type);
        }
        error_at_token(
            ep->parser, name_token, biscuit::ErrorTokenAction::DoNothing,
            String::format("'{}' was already defined as {}", name_token.text, type));
        ep->parser->recovery.mute_errors = false;
        ep->parser->error(
            String::format("{}: ... see previous definition\n",
                           (*prev_target)
                               ->plyfile->tkr.file_location_map.format_file_location(
                                   (*prev_target)->stmt->file_offset)));
        accept = false;
    }

    if (!parse_parameter_list(ep->parser, fn_def.get()))
        return;

    // Parse function body.
    biscuit::Parser::Filter filter;
    filter.keyword_handler = {handle_keyword_inside_target_or_function, ep};
    filter.allow_instructions = true;
    PLY_SET_IN_SCOPE(ep->parser->filter, filter);
    fn_def->body = parse_statement_block(ep->parser, {"function", "parameter list"});

    // Create library
    if (accept) {
        auto target = Owned<Repository::Function>::create();
        target->plyfile = ep->current_plyfile;
        target->stmt = std::move(stmt);

        g_repository->global_scope.assign(fn_def->name, target);
        g_repository->functions.append(std::move(target));
    }
}

bool parse_plyfile(StringView path) {
    String src = FileSystem.load_text_autodetect(path);
    if (FileSystem.last_result() != FSResult::OK) {
        PLY_FORCE_CRASH();
    }

    // Create Plyfile, tokenizer and parser.
    Repository::Plyfile* plyfile =
        g_repository->plyfiles.append(Owned<Repository::Plyfile>::create());
    plyfile->src = std::move(src);
    plyfile->tkr.set_source_input(path, plyfile->src);
    biscuit::Parser parser;

    // Add parser keywords.
    parser.keywords.assign(LABEL_library, true);
    parser.keywords.assign(LABEL_executable, true);
    parser.keywords.assign(LABEL_source_files, true);
    parser.keywords.assign(LABEL_include_directories, true);
    parser.keywords.assign(LABEL_preprocessor_definitions, true);
    parser.keywords.assign(LABEL_dependencies, true);
    parser.keywords.assign(LABEL_link_libraries, true);
    parser.keywords.assign(LABEL_prebuild_step, true);
    parser.keywords.assign(LABEL_config_options, true);
    parser.keywords.assign(LABEL_config_list, true);
    parser.keywords.assign(LABEL_config, true);
    parser.keywords.assign(LABEL_compile_options, true);
    parser.keywords.assign(LABEL_public, true);
    parser.keywords.assign(LABEL_private, true);
    parser.keywords.assign(LABEL_generate, true);

    // Extend the parser
    ExtendedParser ep;
    ep.parser = &parser;
    ep.current_plyfile = plyfile;
    parser.tkr = &plyfile->tkr;
    parser.error = [](StringView message) { Console.error() << message; };
    biscuit::Parser::Filter filter;
    filter.keyword_handler = {handle_keyword_at_file_scope, &ep};
    filter.allow_functions = true;
    parser.filter = filter;
    parser.function_handler = {handle_plyfile_function, &ep};

    // Parse the script and check for errors.
    biscuit::StatementBlockProperties props{"file"};
    Owned<biscuit::StatementBlock> file =
        parse_statement_block_inner(&parser, props, true);
    if (parser.error_count > 0) {
        return false;
    }

    // Success.
    plyfile->contents = std::move(file);
    return true;
}

} // namespace build
} // namespace ply
