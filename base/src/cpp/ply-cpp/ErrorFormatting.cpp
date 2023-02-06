/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

ExpandedFileLocation expand_file_location(const PPVisitedFiles* visited_files,
                                          LinearLocation linear_loc) {
    auto iter = visited_files->location_map.find_last_less_than(linear_loc + 1);
    const cpp::PPVisitedFiles::IncludeChain& chain =
        visited_files->include_chains[iter.get_item().include_chain_idx];
    PLY_ASSERT(!chain.is_macro_expansion); // FIXME handle macros
    const cpp::PPVisitedFiles::SourceFile* src_file =
        &visited_files->source_files[chain.file_or_exp_idx];
    FileLocation file_loc =
        src_file->file_location_map.get_file_location(safe_demote<u32>(
            linear_loc - iter.get_item().linear_loc + iter.get_item().offset));
    return {src_file, file_loc};
}

StringView get_expected_token_desc(ExpectedToken expected) {
    switch (expected) {
        case ExpectedToken::None:
            PLY_ASSERT(0); // shouldn't get here
            return "(None)";
        case ExpectedToken::Identifier:
            return "identifier";
        case ExpectedToken::NestedNamePrefix:
            return "nested name prefix";
        case ExpectedToken::OpenParen:
            return "'('";
        case ExpectedToken::OpenCurly:
            return "'{'";
        case ExpectedToken::OpenAngle:
            return "'<'";
        case ExpectedToken::OpenCurlyOrParen:
            return "'{' or '('";
        case ExpectedToken::CloseParen:
            return "')'";
        case ExpectedToken::CloseSquare:
            return "']'";
        case ExpectedToken::DestructorClassName:
            return "destructor name";
        case ExpectedToken::OperatorToken:
            return "operator token";
        case ExpectedToken::Colon:
            return "':'";
        case ExpectedToken::Equal:
            return "'='";
        case ExpectedToken::QualifiedID:
            return "qualified-id";
        case ExpectedToken::UnqualifiedID:
            return "unqualified-id";
        case ExpectedToken::Semicolon:
            return "';'";
        case ExpectedToken::Comma:
            return "','";
        case ExpectedToken::CommaOrCloseParen:
            return "',' or ')'";
        case ExpectedToken::CommaOrCloseCurly:
            return "',' or '}'";
        case ExpectedToken::CommaOrOpenCurly:
            return "',' or '{'";
        case ExpectedToken::Declaration:
            return "declaration";
        case ExpectedToken::EnumeratorOrCloseCurly:
            return "enumerator or '}'";
        case ExpectedToken::CommaOrCloseAngle:
            return "',' or '>'";
        case ExpectedToken::TrailingReturnType:
            return "trailing return type";
        case ExpectedToken::BaseOrMember:
            return "class member or base class name";
        case ExpectedToken::ParameterType:
            return "parameter type";
        case ExpectedToken::TemplateParameterDecl:
            return "template parameter";
        case ExpectedToken::TypeSpecifier:
            return "type specifier";
        default:
            PLY_ASSERT(0); // shouldn't get here
            return "???";
    }
}

String ExpandedFileLocation::to_string() const {
    return String::format("{}({}, {})", this->src_file->file_location_map.path,
                          this->file_loc.line_number, this->file_loc.column_number);
}

void ParseError::write_message(OutStream& out,
                               const PPVisitedFiles* visited_files) const {
    out.format(
        "{}: error: ",
        expand_file_location(visited_files, this->error_token.linear_loc).to_string());
    switch (this->type) {
        case ParseError::UnexpectedEOF: {
            PLY_ASSERT(this->error_token.type == Token::EndOfFile);
            out << "unexpected end-of-file\n";
            break;
        }
        case ParseError::Expected: {
            out.format("expected {} before '{}'\n",
                       get_expected_token_desc(this->expected),
                       this->error_token.to_string());
            break;
        }
        case ParseError::UnclosedToken: {
            out.format("expected '{}'\n", get_punctuation_string((Token::Type)(
                                              (u32) this->preceding_token.type + 1)));
            PLY_ASSERT(this->preceding_token.is_valid());
            out.format(
                "{}: note: to match this '{}'\n",
                expand_file_location(visited_files, this->preceding_token.linear_loc)
                    .to_string(),
                this->preceding_token.to_string());
            break;
        }
        case ParseError::MissingCommaAfterEnumerator: {
            out << "missing ',' between enumerators\n";
            break;
        }
        case ParseError::QualifierNotAllowedHere: {
            out.format("'{}' qualifier not allowed here\n",
                       this->error_token.identifier);
            break;
        }
        case ParseError::TypeIDCannotHaveName: {
            // FIXME: It would be nice to somehow store the entire qualified-id in the
            // error somehow, and write it in the error message. (Currently, we only
            // store the first token.)
            out << "type-id cannot have a name\n";
            break;
        }
        case ParseError::NestedNameNotAllowedHere: {
            // FIXME: It would be nice to somehow store the entire nested-name-prefix in
            // the error somehow, and write it in the error message. (Currently, we only
            // store the first token.)
            out.format("'{}' cannot have a nested name prefix\n",
                       this->error_token.to_string());
            break;
        }
        case ParseError::TooManyTypeSpecifiers: {
            out << "too many type specifiers\n";
            break;
        }
        case ParseError::ExpectedFunctionBodyAfterMemberInitList: {
            out << "expected function body after member initializer list\n";
            break;
        }
        case ParseError::CantMixFunctionDefAndDecl: {
            out << "can't mix function definitions with other declarations\n";
            break;
        }
        case ParseError::MissingDeclaration: {
            // FIXME: This should only be a warning, but we don't have a warning system
            // yet
            out << "declaration does not declare anything\n";
            break;
        }
        case ParseError::DuplicateVirtSpecifier: {
            out.format("'{}' used more than once\n", this->error_token.identifier);
            break;
        }
        default: {
            out << "error message not implemented!\n";
            break;
        }
    }
}

} // namespace cpp
} // namespace ply
