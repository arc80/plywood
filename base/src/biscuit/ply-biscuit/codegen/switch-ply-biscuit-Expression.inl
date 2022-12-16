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
    NameLookup nameLookup;
    IntegerLiteral integerLiteral;
    InterpolatedString interpolatedString;
    PropertyLookup propertyLookup;
    BinaryOp binaryOp;
    UnaryOp unaryOp;
    Call call;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Expression, NameLookup)
SWITCH_ACCESSOR(NameLookup, nameLookup)
SWITCH_ACCESSOR(IntegerLiteral, integerLiteral)
SWITCH_ACCESSOR(InterpolatedString, interpolatedString)
SWITCH_ACCESSOR(PropertyLookup, propertyLookup)
SWITCH_ACCESSOR(BinaryOp, binaryOp)
SWITCH_ACCESSOR(UnaryOp, unaryOp)
SWITCH_ACCESSOR(Call, call)
