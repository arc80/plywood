enum class ID : u16 {
    NameLookup,
    IntegerLiteral,
    InterpolatedString,
    PropertyLookup,
    BinaryOp,
    UnaryOp,
    Call,
    Count,
};
union Storage_ {
    NameLookup name_lookup;
    IntegerLiteral integer_literal;
    InterpolatedString interpolated_string;
    PropertyLookup property_lookup;
    BinaryOp binary_op;
    UnaryOp unary_op;
    Call call;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(Expression, NameLookup)
SWITCH_ACCESSOR(NameLookup, name_lookup)
SWITCH_ACCESSOR(IntegerLiteral, integer_literal)
SWITCH_ACCESSOR(InterpolatedString, interpolated_string)
SWITCH_ACCESSOR(PropertyLookup, property_lookup)
SWITCH_ACCESSOR(BinaryOp, binary_op)
SWITCH_ACCESSOR(UnaryOp, unary_op)
SWITCH_ACCESSOR(Call, call)
