/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/LinearLocation.h>

namespace ply {
namespace cpp {

struct Token {
    enum Type {
        // ply reflect enum
        Invalid,
        LineComment,
        CStyleComment,
        Macro,
        MacroArgument,
        Directive,
        OpenCurly,
        CloseCurly,
        OpenParen,
        CloseParen,
        OpenAngle,
        CloseAngle,
        LessThanOrEqual,
        GreaterThanOrEqual,
        OpenSquare,
        CloseSquare,
        Semicolon,
        SingleColon,
        DoubleColon,
        SingleEqual,
        DoubleEqual,
        NotEqual,
        PlusEqual,
        MinusEqual,
        Arrow,
        StarEqual,
        SlashEqual,
        Comma,
        QuestionMark,
        ForwardSlash,
        Star,
        Percent,
        SingleAmpersand,
        DoubleAmpersand,
        SingleVerticalBar,
        DoubleVerticalBar,
        SinglePlus,
        DoublePlus,
        SingleMinus,
        DoubleMinus,
        LeftShift,
        RightShift,
        Dot,
        Tilde,
        Caret,
        Bang,
        Ellipsis,
        Identifier,
        StringLiteral,
        NumericLiteral,
        EndOfFile,
    };

    LinearLocation linearLoc = -1;
    StringView identifier; // FIXME: Rename to text

    PLY_REFLECT()
    Type type = Invalid;
    // ply reflect off

    PLY_INLINE bool isValid() const {
        return this->type != Invalid;
    }
    Token() = default;
    Token(LinearLocation linearLoc, Type type, StringView identifier = {})
        : linearLoc{linearLoc}, identifier{identifier}, type{type} {
    }
    bool operator==(const Token& other) {
        if (this->type != other.type)
            return false;
        if (this->type == Identifier || this->type == NumericLiteral) {
            return this->identifier == other.identifier;
        } else {
            return true;
        }
    }
    HybridString toString() const;
};
PLY_DECLARE_TYPE_DESCRIPTOR(Token::Type)

StringView getPunctuationString(Token::Type tok);

} // namespace cpp
} // namespace ply
