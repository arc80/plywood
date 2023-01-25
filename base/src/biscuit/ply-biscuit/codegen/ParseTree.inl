PLY_STRUCT_BEGIN(ply::biscuit::Statement::FunctionDefinition)
PLY_STRUCT_END()

SWITCH_TABLE_BEGIN(ply::biscuit::Expression)
SWITCH_TABLE_STATE(ply::biscuit::Expression, NameLookup)
SWITCH_TABLE_STATE(ply::biscuit::Expression, IntegerLiteral)
SWITCH_TABLE_STATE(ply::biscuit::Expression, InterpolatedString)
SWITCH_TABLE_STATE(ply::biscuit::Expression, PropertyLookup)
SWITCH_TABLE_STATE(ply::biscuit::Expression, BinaryOp)
SWITCH_TABLE_STATE(ply::biscuit::Expression, UnaryOp)
SWITCH_TABLE_STATE(ply::biscuit::Expression, Call)
SWITCH_TABLE_END(ply::biscuit::Expression)

SWITCH_TABLE_BEGIN(ply::biscuit::Statement)
SWITCH_TABLE_STATE(ply::biscuit::Statement, If_)
SWITCH_TABLE_STATE(ply::biscuit::Statement, While_)
SWITCH_TABLE_STATE(ply::biscuit::Statement, Assignment)
SWITCH_TABLE_STATE(ply::biscuit::Statement, Evaluate)
SWITCH_TABLE_STATE(ply::biscuit::Statement, Return_)
SWITCH_TABLE_STATE(ply::biscuit::Statement, FunctionDefinition)
SWITCH_TABLE_STATE(ply::biscuit::Statement, CustomBlock)
SWITCH_TABLE_END(ply::biscuit::Statement)
