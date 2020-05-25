enum class ID : u16 {
    Parenthesized,
    PointerTo,
    ArrayOf,
    Function,
    Qualifier,
    Count,
};
union Storage_ {
    Parenthesized parenthesized;
    PointerTo pointerTo;
    ArrayOf arrayOf;
    Function function;
    Qualifier qualifier;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Parenthesized)
SWITCH_ACCESSOR(Parenthesized, parenthesized)
SWITCH_ACCESSOR(PointerTo, pointerTo)
SWITCH_ACCESSOR(ArrayOf, arrayOf)
SWITCH_ACCESSOR(Function, function)
SWITCH_ACCESSOR(Qualifier, qualifier)
PLY_SWITCH_REFLECT()
