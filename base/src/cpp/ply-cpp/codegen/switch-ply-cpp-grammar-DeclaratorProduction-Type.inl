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
    PointerTo pointer_to;
    ArrayOf array_of;
    Function function;
    Qualifier qualifier;
    PLY_INLINE Storage_() {
    }
    PLY_INLINE ~Storage_() {
    }
};
SWITCH_FOOTER(Type, Parenthesized)
SWITCH_ACCESSOR(Parenthesized, parenthesized)
SWITCH_ACCESSOR(PointerTo, pointer_to)
SWITCH_ACCESSOR(ArrayOf, array_of)
SWITCH_ACCESSOR(Function, function)
SWITCH_ACCESSOR(Qualifier, qualifier)
PLY_SWITCH_REFLECT()
