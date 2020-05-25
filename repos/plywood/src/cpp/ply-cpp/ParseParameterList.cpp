/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-cpp/Core.h>
#include <ply-cpp/ParseDeclarations.h>
#include <ply-cpp/Preprocessor.h>

namespace ply {
namespace cpp {

struct ParseParams {
    Token::Type openPunc = Token::OpenParen;
    Token::Type closePunc = Token::CloseParen;
    SpecDcorMode specDcorMode = SpecDcorMode::Param;

    static ParseParams Func;
    static ParseParams Template;
};

ParseParams ParseParams::Func = {};
ParseParams ParseParams::Template = {
    Token::OpenAngle,
    Token::CloseAngle,
    SpecDcorMode::TemplateParam,
};

PLY_NO_INLINE void parseParameterDeclarationList(Parser* parser,
                                                 grammar::ParamDeclarationList& params,
                                                 bool forTemplate) {
    const ParseParams* pp = forTemplate ? &ParseParams::Template : &ParseParams::Func;

    // Get open punctuator
    // Caller is responsible for ensuring the expected punctuator is next!
    Token token = readToken(parser);
    // FIXME: Maybe we should log an error here instead of asserting. This could let us
    // remove some of the checks performed in some callers, potentially cleaning up code.
    PLY_ASSERT(token.type == pp->openPunc); // Guaranteed by caller
    params.openPunc = token;
    parser->stopMutingErrors();

    token = readToken(parser);
    if (token.type == pp->closePunc) {
        // Empty parameter declaration list
        params.closePunc = token;
        return;
    } else {
        pushBackToken(parser, token);
    }

    SetAcceptFlagsInScope acceptScope{parser, pp->openPunc};

    for (;;) {
        // A parameter declaration is expected here.
        grammar::ParamDeclarationWithComma* pdc = nullptr;
        bool anyTokensConsumed = false;

        Token expectedLoc = readToken(parser);
        if (expectedLoc.type == Token::Ellipsis && !forTemplate) {
            // FIXME: Check somewhere that this is the last parameter
            pdc = &params.params.append();
            grammar::DeclSpecifier* declSpec = new grammar::DeclSpecifier;
            auto ellipsis = declSpec->ellipsis().switchTo();
            ellipsis->ellipsisToken = expectedLoc;
            pdc->declSpecifierSeq.append(declSpec);
            anyTokensConsumed = true;
        } else {
            pushBackToken(parser, expectedLoc);
            grammar::Declaration::Simple simple;
            ParseActivity pa{parser};
            parseSpecifiersAndDeclarators(parser, simple, pp->specDcorMode);
            if (!pa.errorOccurred()) {
                // We successfully parsed a parameter declaration.
                parser->stopMutingErrors();
            }
            if (!simple.initDeclarators.isEmpty()) {
                // If there are no parse errors, parseSpecifiersAndDeclarators is guaranteed to
                // return exactly one initDeclarator when specDcorMode == TemplateParam or Param,
                // even if it's empty, as is the case for an abstract declarator.
                PLY_ASSERT(simple.initDeclarators.numItems() == 1);
                pdc = &params.params.append();
                pdc->declSpecifierSeq = std::move(simple.declSpecifierSeq);
                pdc->dcor = std::move(simple.initDeclarators[0].dcor);
                pdc->init = std::move(simple.initDeclarators[0].init);
            }
            anyTokensConsumed = pa.anyTokensConsumed();
        }

        token = readToken(parser);
        if (token.type == pp->closePunc) {
            // End of parameter declaration list
            params.closePunc = token;
            break;
        } else if (token.type == Token::Comma) {
            // Comma
            if (pdc) {
                pdc->comma = token;
            }
        } else {
            // Unexpected token
            parser->error(true, {ParseError::Expected, token,
                                 forTemplate ? ExpectedToken::CommaOrCloseAngle
                                             : ExpectedToken::CommaOrCloseParen});
            if (anyTokensConsumed) {
                if (!handleUnexpectedToken(parser, nullptr, token))
                    break;
            } else {
                if (!okToStayInScope(parser, token))
                    break;
                pushBackToken(parser, token);
            }
        }
    }
}

grammar::FunctionQualifierSeq parseFunctionQualifierSeq(Parser* parser) {
    grammar::FunctionQualifierSeq qualifiers;

    // Read trailing qualifiers
    for (;;) {
        Token token = readToken(parser);
        if (token.type == Token::Identifier &&
            (token.identifier == "const" || token.identifier == "override")) {
            qualifiers.tokens.append(token);
        } else if (token.type == Token::SingleAmpersand || token.type == Token::DoubleAmpersand) {
            qualifiers.tokens.append(token);
        } else {
            pushBackToken(parser, token);
            break;
        }
    }

    return qualifiers;
}

PLY_NO_INLINE grammar::DeclaratorProduction*
parseParameterList(Parser* parser, Owned<grammar::DeclaratorProduction>** prodToModify) {
    Token openParen = readToken(parser);
    if (openParen.type != Token::OpenParen) {
        // Currently, we only hit this case when optimistically trying to parse a constructor
        PLY_ASSERT(parser->restorePointEnabled); // Just a sanity check
        parser->error(true, {ParseError::Expected, openParen, ExpectedToken::OpenParen});
        pushBackToken(parser, openParen);
        return nullptr;
    }

    parser->stopMutingErrors();

    auto* prod = new grammar::DeclaratorProduction;
    auto func = prod->type.function().switchTo();
    prod->target = std::move(**prodToModify);
    **prodToModify = prod;
    *prodToModify = &prod->target;

    pushBackToken(parser, openParen);
    parseParameterDeclarationList(parser, func->params, false);
    func->qualifiers = parseFunctionQualifierSeq(parser);
    return prod;
}

} // namespace cpp
} // namespace ply
