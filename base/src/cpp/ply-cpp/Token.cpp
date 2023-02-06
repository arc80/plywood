﻿/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/Token.h>

namespace ply {
namespace cpp {

PLY_NO_INLINE StringView get_punctuation_string(Token::Type tok) {
    switch (tok) {
        case Token::OpenCurly:
            return "{";
        case Token::CloseCurly:
            return "}";
        case Token::OpenParen:
            return "(";
        case Token::CloseParen:
            return ")";
        case Token::OpenAngle:
            return "<";
        case Token::CloseAngle:
            return ">";
        case Token::OpenSquare:
            return "[";
        case Token::CloseSquare:
            return "]";
        case Token::Semicolon:
            return ";";
        case Token::SingleColon:
            return ":";
        case Token::DoubleColon:
            return "::";
        case Token::SingleEqual:
            return "=";
        case Token::DoubleEqual:
            return "==";
        case Token::NotEqual:
            return "!=";
        case Token::PlusEqual:
            return "+=";
        case Token::MinusEqual:
            return "-=";
        case Token::Comma:
            return ",";
        case Token::QuestionMark:
            return "?";
        case Token::ForwardSlash:
            return "/";
        case Token::Star:
            return "*";
        case Token::Percent:
            return "%";
        case Token::SingleAmpersand:
            return "&";
        case Token::DoubleAmpersand:
            return "&&";
        case Token::SingleVerticalBar:
            return "|";
        case Token::DoubleVerticalBar:
            return "||";
        case Token::SinglePlus:
            return "+";
        case Token::DoublePlus:
            return "++";
        case Token::SingleMinus:
            return "-";
        case Token::DoubleMinus:
            return "--";
        case Token::LeftShift:
            return "<<";
        case Token::RightShift:
            return ">>";
        case Token::Dot:
            return ".";
        case Token::Tilde:
            return "~";
        case Token::Bang:
            return "!";
        case Token::Ellipsis:
            return "...";
        case Token::LineComment:
            return "//";
        case Token::CStyleComment:
            return "/*";
        case Token::LessThanOrEqual:
            return "<=";
        case Token::GreaterThanOrEqual:
            return ">=";
        case Token::Arrow:
            return "->";
        case Token::StarEqual:
            return "*=";
        case Token::SlashEqual:
            return "/=";
        case Token::Caret:
            return "^";
        default: {
            PLY_ASSERT(0);
            return "???";
        }
    }
}

HybridString Token::to_string() const {
    switch (this->type) {
        case Token::Identifier:
        case Token::StringLiteral:
        case Token::NumericLiteral: {
            return this->identifier;
        }
        case Token::EndOfFile: {
            return StringView{"end-of-file"};
        }
        default: {
            return get_punctuation_string(this->type);
        }
    }
}

} // namespace cpp
} // namespace ply

#include "codegen/Token.inl" //%%
