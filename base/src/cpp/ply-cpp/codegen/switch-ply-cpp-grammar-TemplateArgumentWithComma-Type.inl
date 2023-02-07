enum class ID : u16 {
    Unknown,
    TypeID,
    Count,
};
union Storage_ {
    Unknown unknown;
    TypeID type_id;
    Storage_() {
    }
    ~Storage_() {
    }
};
SWITCH_FOOTER(Type, Unknown)
SWITCH_ACCESSOR(Unknown, unknown)
SWITCH_ACCESSOR(TypeID, type_id)
PLY_SWITCH_REFLECT()
