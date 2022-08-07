/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ErrorFormatting.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

ExpandedFileLocation expandFileLocation(const PPVisitedFiles* visitedFiles,
                                        LinearLocation linearLoc) {
    auto iter = visitedFiles->locationMap.findLastLessThan(linearLoc + 1);
    const cpp::PPVisitedFiles::IncludeChain& chain =
        visitedFiles->includeChains[iter.getItem().includeChainIdx];
    PLY_ASSERT(!chain.isMacroExpansion); // FIXME handle macros
    const cpp::PPVisitedFiles::SourceFile* srcFile = &visitedFiles->sourceFiles[chain.fileOrExpIdx];
    FileLocation fileLoc = srcFile->fileLocMap.getFileLocation(
        safeDemote<u32>(linearLoc - iter.getItem().linearLoc + iter.getItem().offset));
    return {srcFile, fileLoc};
}

StringView getExpectedTokenDesc(ExpectedToken expected) {
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

String ExpandedFileLocation::toString() const {
    return String::format("{}({}, {})", this->srcFile->absPath, this->fileLoc.lineNumber,
                          this->fileLoc.columnNumber);
}

void ParseError::writeMessage(OutStream* outs, const PPVisitedFiles* visitedFiles) const {
    outs->format("{}: error: ",
               expandFileLocation(visitedFiles, this->errorToken.linearLoc).toString());
    switch (this->type) {
        case ParseError::UnexpectedEOF: {
            PLY_ASSERT(this->errorToken.type == Token::EndOfFile);
            *outs << "unexpected end-of-file\n";
            break;
        }
        case ParseError::Expected: {
            outs->format("expected {} before '{}'\n", getExpectedTokenDesc(this->expected),
                       this->errorToken.toString());
            break;
        }
        case ParseError::UnclosedToken: {
            outs->format("expected '{}'\n",
                       getPunctuationString((Token::Type)((u32) this->precedingToken.type + 1)));
            PLY_ASSERT(this->precedingToken.isValid());
            outs->format("{}: note: to match this '{}'\n",
                       expandFileLocation(visitedFiles, this->precedingToken.linearLoc).toString(),
                       this->precedingToken.toString());
            break;
        }
        case ParseError::MissingCommaAfterEnumerator: {
            *outs << "missing ',' between enumerators\n";
            break;
        }
        case ParseError::QualifierNotAllowedHere: {
            outs->format("'{}' qualifier not allowed here\n", this->errorToken.identifier);
            break;
        }
        case ParseError::TypeIDCannotHaveName: {
            // FIXME: It would be nice to somehow store the entire qualified-id in the error
            // somehow, and write it in the error message. (Currently, we only store the first
            // token.)
            *outs << "type-id cannot have a name\n";
            break;
        }
        case ParseError::NestedNameNotAllowedHere: {
            // FIXME: It would be nice to somehow store the entire nested-name-prefix in the error
            // somehow, and write it in the error message. (Currently, we only store the first
            // token.)
            outs->format("'{}' cannot have a nested name prefix\n", this->errorToken.toString());
            break;
        }
        case ParseError::TooManyTypeSpecifiers: {
            *outs << "too many type specifiers\n";
            break;
        }
        case ParseError::ExpectedFunctionBodyAfterMemberInitList: {
            *outs << "expected function body after member initializer list\n";
            break;
        }
        case ParseError::CantMixFunctionDefAndDecl: {
            *outs << "can't mix function definitions with other declarations\n";
            break;
        }
        case ParseError::MissingDeclaration: {
            // FIXME: This should only be a warning, but we don't have a warning system yet
            *outs << "declaration does not declare anything\n";
            break;
        }
        case ParseError::DuplicateVirtSpecifier: {
            outs->format("'{}' used more than once\n", this->errorToken.identifier);
            break;
        }
        default: {
            *outs << "error message not implemented!\n";
            break;
        }
    }
}

} // namespace cpp
} // namespace ply
