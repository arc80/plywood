enum class ID : u16 {
    If_,
    While_,
    Assignment,
    Evaluate,
    Return_,
    FunctionDefinition,
    CustomBlock,
    Count,
};
union Storage_ {
    If_ if_;
    While_ while_;
    Assignment assignment;
    Evaluate evaluate;
    Return_ return_;
    FunctionDefinition function_definition;
    CustomBlock custom_block;
    PLY_INLINE Storage_() {
    }
    PLY_INLINE ~Storage_() {
    }
};
SWITCH_FOOTER(Statement, If_)
SWITCH_ACCESSOR(If_, if_)
SWITCH_ACCESSOR(While_, while_)
SWITCH_ACCESSOR(Assignment, assignment)
SWITCH_ACCESSOR(Evaluate, evaluate)
SWITCH_ACCESSOR(Return_, return_)
SWITCH_ACCESSOR(FunctionDefinition, function_definition)
SWITCH_ACCESSOR(CustomBlock, custom_block)
