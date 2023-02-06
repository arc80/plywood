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
    FunctionBody function_body;
    BitField bit_field;
    PLY_INLINE Storage_() {
    }
    PLY_INLINE ~Storage_() {
    }
};
SWITCH_FOOTER(Initializer, None)
SWITCH_ACCESSOR(None, none)
SWITCH_ACCESSOR(Assignment, assignment)
SWITCH_ACCESSOR(FunctionBody, function_body)
SWITCH_ACCESSOR(BitField, bit_field)
PLY_SWITCH_REFLECT()
