/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
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
        PLY_REFLECT_ENUM(friend, Type)

        PLY_REFLECT()
        Type type = Unknown;
        LinearLocation linearLoc = -1;
        LinearLocation otherLoc = -1;
        // ply reflect off

        PLY_INLINE Error(Type type, LinearLocation linearLoc, LinearLocation otherLoc = -1)
            : type{type}, linearLoc{linearLoc}, otherLoc{otherLoc} {
        }
        virtual void writeMessage(StringWriter* sw,
                                  const PPVisitedFiles* visitedFiles) const override;
    };

    Functor<void(Owned<BaseError>&&)> errorHandler;
    PPVisitedFiles* visitedFiles = nullptr;

    struct StackItem {
        u32 includeChainIdx = 0;
        StringViewReader strViewReader;
    };
    Array<StackItem> stack;
    LinearLocation linearLocAtEndOfStackTop = -1;

    struct MacrosTraits {
        using Key = StringView;
        struct Item {
            String identifier;
            u32 expansionIdx = 0;
        };
        static Key comparand(const Item& item) {
            return item.identifier;
        }
    };
    HashMap<MacrosTraits> macros;

    bool tokenizeCloseAnglesOnly = false;
    bool atStartOfLine = true;

    HiddenArgFunctor<void(StringView directive)> includeCallback;

    PLY_INLINE void error(Error&& err) {
        this->errorHandler.call(new Error{std::move(err)});
    }
};
PLY_REFLECT_ENUM(, Preprocessor::Error::Type)

Token readToken(Preprocessor* pp);

} // namespace cpp
} // namespace ply
