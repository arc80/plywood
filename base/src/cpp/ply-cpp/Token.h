/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
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

    LinearLocation linear_loc = -1;
    StringView identifier; // FIXME: Rename to text

    PLY_REFLECT()
    Type type = Invalid;
    // ply reflect off

    PLY_INLINE bool is_valid() const {
        return this->type != Invalid;
    }
    Token() = default;
    Token(LinearLocation linear_loc, Type type, StringView identifier = {})
        : linear_loc{linear_loc}, identifier{identifier}, type{type} {
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
    HybridString to_string() const;
};

StringView get_punctuation_string(Token::Type tok);

} // namespace cpp

PLY_DECLARE_TYPE_DESCRIPTOR(cpp::Token::Type)

} // namespace ply
