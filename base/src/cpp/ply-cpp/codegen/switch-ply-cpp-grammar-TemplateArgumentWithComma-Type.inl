enum class ID : u16 {
    Unknown,
    TypeID,
    Count,
};
union Storage_ {
    Unknown unknown;
    TypeID typeID;
    PLY_INLINE Storage_() {}
    PLY_INLINE ~Storage_() {}
};
SWITCH_FOOTER(Type, Unknown)
SWITCH_ACCESSOR(Unknown, unknown)
SWITCH_ACCESSOR(TypeID, typeID)
PLY_SWITCH_REFLECT()
