enum class ID : u16 {
    IdentifierOrTemplated,
    DeclType,
    Count,
};
union Storage_ {
    IdentifierOrTemplated identifier_or_templated;
    DeclType decl_type;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(Type, IdentifierOrTemplated)
SWITCH_ACCESSOR(IdentifierOrTemplated, identifier_or_templated)
SWITCH_ACCESSOR(DeclType, decl_type)
PLY_SWITCH_REFLECT()
