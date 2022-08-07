enum class ID : u16 {
    None,
    Assignment,
    FunctionBody,
    BitField,
    Count,
};
union Storage_ {
    None none;
    Assignment assignment;
    FunctionBody functionBody;
    BitField bitField;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Initializer, None)
SWITCH_ACCESSOR(None, none)
SWITCH_ACCESSOR(Assignment, assignment)
SWITCH_ACCESSOR(FunctionBody, functionBody)
SWITCH_ACCESSOR(BitField, bitField)
PLY_SWITCH_REFLECT()
