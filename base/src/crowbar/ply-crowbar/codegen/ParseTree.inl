PLY_STRUCT_BEGIN(ply::crowbar::Statement::FunctionDefinition)
PLY_STRUCT_END()

SWITCH_TABLE_BEGIN(ply::crowbar::Expression)
SWITCH_TABLE_STATE(ply::crowbar::Expression, NameLookup)
SWITCH_TABLE_STATE(ply::crowbar::Expression, IntegerLiteral)
SWITCH_TABLE_STATE(ply::crowbar::Expression, InterpolatedString)
SWITCH_TABLE_STATE(ply::crowbar::Expression, PropertyLookup)
SWITCH_TABLE_STATE(ply::crowbar::Expression, BinaryOp)
SWITCH_TABLE_STATE(ply::crowbar::Expression, UnaryOp)
SWITCH_TABLE_STATE(ply::crowbar::Expression, Call)
SWITCH_TABLE_END(ply::crowbar::Expression)

SWITCH_TABLE_BEGIN(ply::crowbar::Statement)
SWITCH_TABLE_STATE(ply::crowbar::Statement, If_)
SWITCH_TABLE_STATE(ply::crowbar::Statement, While_)
SWITCH_TABLE_STATE(ply::crowbar::Statement, Assignment)
SWITCH_TABLE_STATE(ply::crowbar::Statement, Evaluate)
SWITCH_TABLE_STATE(ply::crowbar::Statement, Return_)
SWITCH_TABLE_STATE(ply::crowbar::Statement, FunctionDefinition)
SWITCH_TABLE_STATE(ply::crowbar::Statement, CustomBlock)
SWITCH_TABLE_END(ply::crowbar::Statement)

