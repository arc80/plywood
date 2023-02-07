/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Token.h>
#include <ply-cpp/LinearLocation.h>
#include <ply-cpp/Error.h>

namespace ply {
namespace cpp {

struct PPVisitedFiles;

struct Preprocessor {
    struct Error : BaseError {
        enum Type {
            // ply reflect enum
            Unknown,
            InvalidDirective,
            EOFInMacro,
            EOFInComment,
            EOFInStringLiteral,
            EOFInRawStringDelimiter,
            InvalidCharInRawStringDelimiter,
            InvalidStringLiteralPrefix,
            DirectiveNotAtStartOfLine,
            GarbageCharacters,
        };

        PLY_REFLECT()
        Type type = Unknown;
        LinearLocation linear_loc = -1;
        LinearLocation other_loc = -1;
        // ply reflect off

        Error(Type type, LinearLocation linear_loc, LinearLocation other_loc = -1)
            : type{type}, linear_loc{linear_loc}, other_loc{other_loc} {
        }
        virtual void write_message(OutStream& out,
                                   const PPVisitedFiles* visited_files) const override;
    };

    Func<void(Owned<BaseError>&&)> error_handler;
    PPVisitedFiles* visited_files = nullptr;

    struct StackItem {
        u32 include_chain_idx = 0;
        ViewInStream in;
    };
    Array<StackItem> stack;
    LinearLocation linear_loc_at_end_of_stack_top = -1;
    Map<StringView, u32> macros;

    bool tokenize_close_angles_only = false;
    bool at_start_of_line = true;

    // This member is only valid when a token type of Macro is returned.
    // It'll remain valid until the next call to read_token.
    Array<Token> macro_args;

    Func<void(StringView directive)> include_callback;

    void error(Error&& err) {
        this->error_handler(new Error{std::move(err)});
    }
};

Token read_token(Preprocessor* pp);
void add_ppdef(Preprocessor* pp, StringView identifier, StringView expansion,
               bool takes_args = false);

} // namespace cpp

PLY_DECLARE_TYPE_DESCRIPTOR(cpp::Preprocessor::Error::Type)

} // namespace ply
