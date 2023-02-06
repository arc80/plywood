﻿PLY_STRUCT_BEGIN(ply::cpp::ParseError)
PLY_STRUCT_MEMBER(type)
PLY_STRUCT_MEMBER(error_token)
PLY_STRUCT_MEMBER(expected)
PLY_STRUCT_MEMBER(preceding_token)
PLY_STRUCT_END()

PLY_ENUM_BEGIN(ply::cpp::, ExpectedToken)
PLY_ENUM_IDENTIFIER(None)
PLY_ENUM_IDENTIFIER(Identifier)
PLY_ENUM_IDENTIFIER(NestedNamePrefix)
PLY_ENUM_IDENTIFIER(OpenParen)
PLY_ENUM_IDENTIFIER(OpenCurly)
PLY_ENUM_IDENTIFIER(OpenAngle)
PLY_ENUM_IDENTIFIER(OpenCurlyOrParen)
PLY_ENUM_IDENTIFIER(CloseParen)
PLY_ENUM_IDENTIFIER(CloseSquare)
PLY_ENUM_IDENTIFIER(DestructorClassName)
PLY_ENUM_IDENTIFIER(OperatorToken)
PLY_ENUM_IDENTIFIER(Colon)
PLY_ENUM_IDENTIFIER(Equal)
PLY_ENUM_IDENTIFIER(QualifiedID)
PLY_ENUM_IDENTIFIER(UnqualifiedID)
PLY_ENUM_IDENTIFIER(Semicolon)
PLY_ENUM_IDENTIFIER(Comma)
PLY_ENUM_IDENTIFIER(CommaOrCloseParen)
PLY_ENUM_IDENTIFIER(CommaOrCloseCurly)
PLY_ENUM_IDENTIFIER(CommaOrOpenCurly)
PLY_ENUM_IDENTIFIER(Declaration)
PLY_ENUM_IDENTIFIER(EnumeratorOrCloseCurly)
PLY_ENUM_IDENTIFIER(CommaOrCloseAngle)
PLY_ENUM_IDENTIFIER(TrailingReturnType)
PLY_ENUM_IDENTIFIER(BaseOrMember)
PLY_ENUM_IDENTIFIER(ParameterType)
PLY_ENUM_IDENTIFIER(TemplateParameterDecl)
PLY_ENUM_IDENTIFIER(TypeSpecifier)
PLY_ENUM_IDENTIFIER(ClassKeyword)
PLY_ENUM_END()

PLY_ENUM_BEGIN(ply::cpp::, ParseError::Type)
PLY_ENUM_IDENTIFIER(Invalid)
PLY_ENUM_IDENTIFIER(Expected)
PLY_ENUM_IDENTIFIER(UnexpectedEOF)
PLY_ENUM_IDENTIFIER(UnclosedToken)
PLY_ENUM_IDENTIFIER(MissingCommaAfterEnumerator)
PLY_ENUM_IDENTIFIER(UnmatchedCloseToken)
PLY_ENUM_IDENTIFIER(QualifierNotAllowedHere)
PLY_ENUM_IDENTIFIER(TypeIDCannotHaveName)
PLY_ENUM_IDENTIFIER(NestedNameNotAllowedHere)
PLY_ENUM_IDENTIFIER(TooManyTypeSpecifiers)
PLY_ENUM_IDENTIFIER(ExpectedFunctionBodyAfterMemberInitList)
PLY_ENUM_IDENTIFIER(CantMixFunctionDefAndDecl)
PLY_ENUM_IDENTIFIER(ScopedEnumRequiresName)
PLY_ENUM_IDENTIFIER(MissingDeclaration)
PLY_ENUM_IDENTIFIER(DuplicateVirtSpecifier)
PLY_ENUM_END()
