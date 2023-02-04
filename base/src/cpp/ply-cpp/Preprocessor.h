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
        LinearLocation linearLoc = -1;
        LinearLocation otherLoc = -1;
        // ply reflect off

        PLY_INLINE Error(Type type, LinearLocation linearLoc, LinearLocation otherLoc = -1)
            : type{type}, linearLoc{linearLoc}, otherLoc{otherLoc} {
        }
        virtual void writeMessage(OutStream& out,
                                  const PPVisitedFiles* visitedFiles) const override;
    };

    Func<void(Owned<BaseError>&&)> errorHandler;
    PPVisitedFiles* visitedFiles = nullptr;

    struct StackItem {
        u32 includeChainIdx = 0;
        ViewInStream in;
    };
    Array<StackItem> stack;
    LinearLocation linearLocAtEndOfStackTop = -1;
    Map<StringView, u32> macros;

    bool tokenizeCloseAnglesOnly = false;
    bool atStartOfLine = true;

    // This member is only valid when a token type of Macro is returned.
    // It'll remain valid until the next call to readToken.
    Array<Token> macroArgs;

    Func<void(StringView directive)> includeCallback;

    PLY_INLINE void error(Error&& err) {
        this->errorHandler(new Error{std::move(err)});
    }
};

Token readToken(Preprocessor* pp);
void addPPDef(Preprocessor* pp, StringView identifier, StringView expansion,
              bool takesArgs = false);

} // namespace cpp

PLY_DECLARE_TYPE_DESCRIPTOR(cpp::Preprocessor::Error::Type)

} // namespace ply
