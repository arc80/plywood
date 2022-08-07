PLY_STRUCT_BEGIN(ply::tool::Command::Dependency)
PLY_STRUCT_MEMBER(repoName)
PLY_STRUCT_MEMBER(depType)
PLY_STRUCT_MEMBER(depName)
PLY_STRUCT_END()

PLY_STRUCT_BEGIN(ply::tool::Command::Type::Run)
PLY_STRUCT_MEMBER(sourceFiles)
PLY_STRUCT_MEMBER(dependencies)
PLY_STRUCT_END()

PLY_STRUCT_BEGIN(ply::tool::Command)
PLY_STRUCT_MEMBER(type)
PLY_STRUCT_END()

PLY_ENUM_BEGIN(ply::tool::, Command::Dependency::Type)
PLY_ENUM_IDENTIFIER(Target)
PLY_ENUM_IDENTIFIER(External)
PLY_ENUM_END()

SWITCH_TABLE_BEGIN(ply::tool::Command::Type)
SWITCH_TABLE_STATE(ply::tool::Command::Type, Run)
SWITCH_TABLE_END(ply::tool::Command::Type)

PLY_SWITCH_BEGIN(ply::tool::Command::Type)
PLY_SWITCH_MEMBER(Run)
PLY_SWITCH_END()

