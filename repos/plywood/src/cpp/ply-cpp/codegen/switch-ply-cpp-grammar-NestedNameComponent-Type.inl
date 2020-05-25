enum class ID : u16 {
    IdentifierOrTemplated,
    DeclType,
    Count,
};
union Storage_ {
    IdentifierOrTemplated identifierOrTemplated;
    DeclType declType;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, IdentifierOrTemplated)
SWITCH_ACCESSOR(IdentifierOrTemplated, identifierOrTemplated)
SWITCH_ACCESSOR(DeclType, declType)
PLY_SWITCH_REFLECT()
